#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct SVHeader
{
    uint32_t Magic;
    uint32_t Version;

    uint32_t StageCount;
    uint32_t ResourceCount;
};

struct SVStageHeader
{
    uint32_t Stage;
    uint32_t NameLength;
    uint32_t SpirvWordCount;
};

struct SVShaderStage
{
    std::string Name;

    uint32_t Stage;

    std::vector<uint32_t>
        Spirv;
};

struct SVResourceHeader
{
    uint32_t NameLength;

    uint32_t Set;
    uint32_t Binding;
};

struct SVResource
{
    std::string Name;

    uint32_t Set;
    uint32_t Binding;
};

class SVFile
{
public:

    void AddStage(
        const SVShaderStage& stage)
    {
        shaderStages.push_back(
            stage);
    }

    void AddResource(
        const SVResource& resource)
    {
        resources.push_back(
            resource);
    }

    void Save(
        const std::string& path);
    
    void Load(
        const std::string& path);

    std::vector<SVShaderStage> GetStages() const
    {
        return shaderStages;
    }

    std::vector<SVResource> GetResources() const
    {
        return resources;
    }

private:

    std::vector<SVShaderStage>
        shaderStages;

    std::vector<SVResource>
        resources;
};