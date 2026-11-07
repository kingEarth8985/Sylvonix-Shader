#include "slangcomp.hpp"
#include <iostream>
#include <array>

const char* getStageName(SlangStage stage) {
    switch (stage) {
        case SLANG_STAGE_VERTEX:        return "vertex";
        case SLANG_STAGE_HULL:          return "hull";
        case SLANG_STAGE_DOMAIN:        return "domain";
        case SLANG_STAGE_GEOMETRY:      return "geometry";
        case SLANG_STAGE_FRAGMENT:      return "fragment";
        case SLANG_STAGE_COMPUTE:       return "compute";
        case SLANG_STAGE_RAY_GENERATION:return "raygeneration";
        case SLANG_STAGE_INTERSECTION:  return "intersection";
        case SLANG_STAGE_ANY_HIT:       return "anyhit";
        case SLANG_STAGE_CLOSEST_HIT:   return "closesthit";
        case SLANG_STAGE_MISS:          return "miss";
        case SLANG_STAGE_CALLABLE:      return "callable";
        case SLANG_STAGE_MESH:          return "mesh";
        case SLANG_STAGE_AMPLIFICATION: return "amplification";
        default:                        return "unknown";
    }
}

void SlangCompiler::Compile()
{
    LoadSlangFile();
    std::cout << "Loaded Slang file and module\n";
    ReflectResources();
    FindEntryPoints();

    std::filesystem::path outputPath =
        sourcePath;

    outputPath.replace_extension(
        ".svshader");

    svFile.Save(
        outputPath.string());
}

void SlangCompiler::LoadSlangFile()
{

    slang::createGlobalSession(
        globalSession.writeRef()
    );

    slang::TargetDesc targetDesc{};
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = globalSession->findProfile("spirv_1_4");

    slang::CompilerOptionEntry compilerOption{
        slang::CompilerOptionName::EmitSpirvDirectly,
        {
            slang::CompilerOptionValueKind::Int, 1
        }
    };

    slang::SessionDesc sessionDesc{};

    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;

    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    sessionDesc.compilerOptionEntries = &compilerOption;
    sessionDesc.compilerOptionEntryCount = 1;
    SlangResult createResult = globalSession->createSession(sessionDesc, session.writeRef());

    if(SLANG_FAILED(createResult))
    {
        throw std::runtime_error(
            "Failed to create Slang session.");
    }

    slangModule =
        session->loadModuleFromSource(
            "Shader",
            sourcePath.c_str(),
            nullptr,
            nullptr);

    if(!slangModule)
    {
        throw std::runtime_error(
            "Failed to load module.");
    }

    programLayout = slangModule->getLayout(0, nullptr);
}

void SlangCompiler::ReflectResources()
{
    if(!programLayout)
    {
        return;
    }

    SlangInt parameterCount =
        programLayout->getParameterCount();

    std::cout
        << "===== RESOURCES =====\n";

    for(
        SlangInt i = 0;
        i < parameterCount;
        i++)
    {
        auto* parameter =
            programLayout->getParameterByIndex(i);

        auto* typeLayout =
            parameter->getTypeLayout();

        auto* type =
            typeLayout->getType();

        SVResource resource{};

        resource.Name =
            parameter->getName();

        if(parameter->getCategoryCount() > 0)
        {
            auto category =
                parameter->getCategoryByIndex(0);

            resource.Binding =
                static_cast<uint32_t>(
                    parameter->getOffset(
                        category));

            resource.Set =
                static_cast<uint32_t>(
                    parameter->getBindingSpace(
                        category));
        }

        std::cout
            << "Resource: "
            << resource.Name
            << "\n";

        std::cout
            << "Set: "
            << resource.Set
            << "\n";

        std::cout
            << "Binding: "
            << resource.Binding
            << "\n";

        svFile.AddResource(
            resource);
    }

    std::cout
        << "=====================\n";
}

std::vector<uint32_t>SlangCompiler::CompileSPIRV(slang::IComponentType* program)
{
    Slang::ComPtr<ISlangBlob>
        spirv;

    SlangResult result =
        program->getTargetCode(
            0,
            spirv.writeRef());

    if(SLANG_FAILED(result))
    {
        throw std::runtime_error(
            "Failed to compile SPIR-V.");
    }

    std::cout
        << "SPIR-V Size: "
        << spirv->getBufferSize()
        << " bytes\n";

    const uint32_t* words =
        static_cast<const uint32_t*>(
            spirv->getBufferPointer());

    size_t wordCount =
        spirv->getBufferSize()
        / sizeof(uint32_t);

    return std::vector<uint32_t>(
        words,
        words + wordCount);
}

void SlangCompiler::FindEntryPoints()
{
    SlangInt32 entryPointCount =
        slangModule->getDefinedEntryPointCount();

    
    for(SlangInt32 i = 0; i < entryPointCount; i++)
    {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;

        slangModule->getDefinedEntryPoint(
            i,
            entryPoint.writeRef());

        entryPoints.push_back(entryPoint);

        std::array<slang::IComponentType*, 2>
            componentTypes =
        {
            slangModule,
            entryPoint
        };

        Slang::ComPtr<slang::IComponentType>
            composedProgram;

        SlangResult result =
            session->createCompositeComponentType(
                componentTypes.data(),
                componentTypes.size(),
                composedProgram.writeRef(),
                nullptr);

        if(SLANG_FAILED(result))
        {
            continue;
        }

        auto* layout =
            composedProgram->getLayout();

        auto* entryPointLayout =
            layout->getEntryPointByIndex(0);

        std::cout
            << "Found entry point: "
            << entryPointLayout->getName()
            << "\n";

        std::cout
            << "Stage: "
            << getStageName(entryPointLayout->getStage())
            << "\n";
        
        SVShaderStage stage{};

        stage.Name =
            entryPointLayout->getName();

        stage.Stage =
            static_cast<uint32_t>(
                entryPointLayout->getStage());
        
        stage.Spirv =
            CompileSPIRV(
                composedProgram);

        svFile.AddStage(stage);
    }
}