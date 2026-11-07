
static const float PI = 3.14159265359;

// ============================================================
// EXISTING INPUTS AND DESCRIPTOR BINDINGS
// ============================================================

struct VSInput
{
    float3 Pos;
    float3 Normal;
    float2 UV;
    float3 Tangent;
};

struct PBRMaterial
{
    float3 baseColor;
    float metallic;
    float roughness;
    uint16_t textureID;
    uint16_t useTexture;
};

//globaly available resources
[[vk::binding(0, 0)]]
Sampler2D textures[];
[[vk::binding(1, 0)]]
StructuredBuffer<PBRMaterial> pbr_materials;

[[vk::binding(0, 1)]]
StructuredBuffer<uint32_t> localMaterialIDs;
[[vk::binding(1, 1)]]
StructuredBuffer<uint16_t> primitiveMaterialIDs;

[[vk::binding(0, 2)]]
StructuredBuffer<uint32_t> materialOverrides;

struct ShaderData
{
    float4x4 projection;
    float4x4 view;
    float4x4 model[3];
    float4 lightDir;
    uint32_t selected;
};

struct ObjectData
{
    float4x4 model;
    uint32_t materialID;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;

    float3 Normal;
    float3 WorldNormal;
    float3 WorldViewDir;
    float3 WorldLightDir;

    float2 UV;
    float3 WorldPos;
    float3 ViewPos;

    float3 LightDir;
};

// ============================================================
// LIGHTING CONFIGURATION
// ============================================================

// These colors are LINEAR RGB, not sRGB values.
// Adjust these to match your desired outdoor appearance.

static const float3 SKY_COLOR =
    float3(0.32, 0.42, 0.60);

static const float3 GROUND_COLOR =
    float3(0.18, 0.14, 0.10);

// Multiplier for indirect sky/ground illumination.
static const float AMBIENT_ENERGY = 1.0;

// Directional light intensity, in arbitrary linear units.
static const float DIRECT_LIGHT_ENERGY = 3.0;

// ============================================================
// PBR BRDF
// ============================================================

// GGX / Trowbridge-Reitz normal distribution.
float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH2 = NdotH * NdotH;

    float denominator =
        NdotH2 * (a2 - 1.0) + 1.0;

    return a2 /
        max(PI * denominator * denominator, 0.000001);
}

// Schlick-GGX visibility approximation.
// k uses the direct-lighting convention.
float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotX /
        max(NdotX * (1.0 - k) + k, 0.000001);
}

float GeometrySmith(
    float NdotV,
    float NdotL,
    float roughness)
{
    float Gv = GeometrySchlickGGX(NdotV, roughness);
    float Gl = GeometrySchlickGGX(NdotL, roughness);

    return Gv * Gl;
}

// Schlick Fresnel approximation.
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    float x = 1.0 - saturate(cosTheta);
    float x2 = x * x;
    float x5 = x2 * x2 * x;

    return F0 + (1.0 - F0) * x5;
}

// Roughness-aware Fresnel approximation for ambient reflections.
float3 FresnelSchlickRoughness(
    float NdotV,
    float3 F0,
    float roughness)
{
    float3 grazing =
        max(float3(1.0 - roughness, 1.0 - roughness,
                   1.0 - roughness), F0);

    float x = 1.0 - saturate(NdotV);
    float x2 = x * x;
    float x5 = x2 * x2 * x;

    return F0 + (grazing - F0) * x5;
}

// ============================================================
// VERTEX SHADER
// ============================================================

[shader("vertex")]
VSOutput main(
    VSInput input,
    uniform ShaderData *shaderData
    //uniform ObjectData *objectData,
    )
{
    VSOutput o;

    float4x4 modelMat = shaderData->model[0];

    float4 worldPos =
        mul(modelMat, float4(input.Pos, 1.0));

    float4 viewPos =
        mul(shaderData->view, worldPos);

    o.Pos = mul(shaderData->projection, viewPos);

    // Preserve interpolated coordinates and instance information.
    o.UV = input.UV;

    o.WorldPos = worldPos.xyz;
    o.ViewPos = viewPos.xyz;

    // Transform the normal into world space.
    // This is correct for rotation and uniform scale.
    // For non-uniform scaling, use an inverse-transpose normal matrix.
    float3 worldNormal =
        normalize(mul((float3x3)modelMat, input.Normal));

    o.WorldNormal = worldNormal;

    // Direct-light calculations use view-space normals and vectors.
    // This keeps the normal, light direction and view direction
    // in the same coordinate space.
    o.Normal = normalize(
        mul((float3x3)shaderData->view, worldNormal));

    // Assumes lightDir represents the direction light rays travel.
    // Therefore, negate it to obtain the surface-to-light vector.
    float3 worldLightDir =
        normalize(shaderData->lightDir.xyz);

    o.WorldLightDir = worldLightDir;

    o.LightDir = normalize(
        mul((float3x3)shaderData->view, worldLightDir));

    // Retained for compatibility with the existing output structure.
    // The fragment shader calculates its view direction from ViewPos.
    o.WorldViewDir = float3(0.0, 0.0, 0.0);

    return o;
}

// ============================================================
// FRAGMENT SHADER
// ============================================================

[shader("fragment")]
float4 main(
    VSOutput input,
    uint triID : SV_PrimitiveID)
{
    // --------------------------------------------------------
    // MATERIAL
    // --------------------------------------------------------

    uint materialID = primitiveMaterialIDs[triID];
    PBRMaterial mat = pbr_materials[materialID];

    float3 baseColor = mat.baseColor;

    if (mat.useTexture != 0)
    {
        // Assumes the texture is sampled in linear RGB.
        // Prefer an sRGB Vulkan image format for color textures.
        float3 texColor =
            textures[mat.textureID].Sample(input.UV).rgb;

        baseColor *= texColor;
    }

    float metallic = saturate(mat.metallic);
    float roughness = clamp(mat.roughness, 0.045, 1.0);

    // --------------------------------------------------------
    // VECTORS
    // --------------------------------------------------------

    float3 N = normalize(input.Normal);

    // View-space camera direction.
    float3 V = normalize(-input.ViewPos);

    // lightDir is the direction rays travel, so reverse it.
    float3 L = normalize(-input.LightDir);

    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = max(saturate(dot(N, V)), 0.0001);
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    // --------------------------------------------------------
    // F0 AND ENERGY CONSERVATION
    // --------------------------------------------------------

    // Dielectrics reflect approximately 4% at normal incidence.
    // Metals use their base color as the specular reflectance.
    float3 F0 = lerp(
        float3(0.04, 0.04, 0.04),
        baseColor,
        metallic);

    // --------------------------------------------------------
    // DIRECT LIGHTING
    // --------------------------------------------------------

    float3 directLighting = float3(0.0, 0.0, 0.0);

    if (NdotL > 0.0)
    {
        float D = DistributionGGX(NdotH, roughness);

        float G = GeometrySmith(
            NdotV,
            NdotL,
            roughness);

        float3 F = FresnelSchlick(HdotV, F0);

        // Cook-Torrance microfacet specular BRDF.
        float3 specular =
            (D * G * F) /
            max(4.0 * NdotV * NdotL, 0.0001);

        // Remaining energy is available for diffuse reflection.
        float3 kS = F;
        float3 kD = (1.0 - kS) * (1.0 - metallic);

        // Lambertian diffuse BRDF.
        float3 diffuse = kD * baseColor / PI;

        // Single directional light: no distance attenuation.
        float3 radiance =
            float3(1.0, 1.0, 1.0) * DIRECT_LIGHT_ENERGY;

        directLighting =
            (diffuse + specular) * radiance * NdotL;
    }

    // --------------------------------------------------------
    // PROCEDURAL SKY / GROUND AMBIENT
    // --------------------------------------------------------

    // Use the WORLD normal, not the view-space normal.
    // This ensures ambient illumination does not rotate with
    // the camera.
    float3 worldN = normalize(input.WorldNormal);

    // Blend ground illumination on downward-facing surfaces
    // into sky illumination on upward-facing surfaces.
    float skyFactor = saturate(worldN.y * 0.5 + 0.5);

    float3 ambientEnvironment = lerp(
        GROUND_COLOR,
        SKY_COLOR,
        skyFactor);

    // Ambient diffuse: metallic pbr_materials have no diffuse lobe.
    float3 ambientDiffuse =
        ambientEnvironment *
        baseColor *
        (1.0 - metallic);

    // Approximate ambient specular energy using Fresnel.
    // This is not full image-based lighting: no cubemap or
    // roughness-filtered environment is available in the inputs.
    float3 ambientF =
        FresnelSchlickRoughness(NdotV, F0, roughness);

    float3 ambientSpecular =
        ambientEnvironment * ambientF;

    float3 ambient =
        (ambientDiffuse + ambientSpecular) *
        AMBIENT_ENERGY;

    // --------------------------------------------------------
    // COMBINE LIGHTING
    // --------------------------------------------------------

    float3 color =
        directLighting + ambient;

    // Keep the result in linear HDR.
    // No tone mapping, gamma correction, or clamping to [0, 1].
    return float4(max(color, float3(0.0, 0.0, 0.0)), 1.0);

    return float4(color, 1.0);
}
