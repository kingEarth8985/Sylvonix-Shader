#include "svshaderFile.hpp"

#include <fstream>

void SVFile::Save(
    const std::string& path)
{
    std::ofstream file(
        path,
        std::ios::binary);

    if(!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open SVShader file.");
    }

    SVHeader header{};

    header.Magic =
        'SVSH';

    header.Version =
        1;

    header.StageCount =
        static_cast<uint32_t>(
            shaderStages.size());

    header.ResourceCount =
        static_cast<uint32_t>(
            resources.size());

    file.write(
        reinterpret_cast<const char*>(
            &header),
        sizeof(header));

    for(const auto& stage :
        shaderStages)
    {
        SVStageHeader stageHeader{};

        stageHeader.Stage =
            stage.Stage;

        stageHeader.NameLength =
            static_cast<uint32_t>(
                stage.Name.size());

        stageHeader.SpirvWordCount =
            static_cast<uint32_t>(
                stage.Spirv.size());

        file.write(
            reinterpret_cast<const char*>(
                &stageHeader),
            sizeof(stageHeader));

        file.write(
            stage.Name.data(),
            stage.Name.size());

        file.write(
            reinterpret_cast<const char*>(
                stage.Spirv.data()),
            stage.Spirv.size()
                * sizeof(uint32_t));
    }

    for(const auto& resource :
        resources)
    {
        SVResourceHeader header{};

        header.NameLength =
            static_cast<uint32_t>(
                resource.Name.size());

        header.Set =
            resource.Set;

        header.Binding =
            resource.Binding;

        file.write(
            reinterpret_cast<const char*>(
                &header),
            sizeof(header));

        file.write(
            resource.Name.data(),
            resource.Name.size());
    }
}

void SVFile::Load(
    const std::string& path)
{
    std::ifstream file(
        path,
        std::ios::binary);

    if(!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open SVShader file.");
    }

    shaderStages.clear();
    resources.clear();

    SVHeader header{};

    file.read(
        reinterpret_cast<char*>(
            &header),
        sizeof(header));

    if(header.Magic != 'SVSH')
    {
        throw std::runtime_error(
            "Invalid SVShader file.");
    }

    if(header.Version != 1)
    {
        throw std::runtime_error(
            "Unsupported SVShader version.");
    }

    for(uint32_t i = 0;
        i < header.StageCount;
        i++)
    {
        SVStageHeader stageHeader{};

        file.read(
            reinterpret_cast<char*>(
                &stageHeader),
            sizeof(stageHeader));

        SVShaderStage stage{};

        stage.Stage =
            stageHeader.Stage;

        stage.Name.resize(
            stageHeader.NameLength);

        file.read(
            stage.Name.data(),
            stageHeader.NameLength);

        stage.Spirv.resize(
            stageHeader.SpirvWordCount);

        file.read(
            reinterpret_cast<char*>(
                stage.Spirv.data()),
            stageHeader.SpirvWordCount
                * sizeof(uint32_t));

        shaderStages.push_back(
            std::move(stage));
    }

    // Resources later
    for(uint32_t i = 0;
        i < header.ResourceCount;
        i++)
    {
        SVResourceHeader resourceHeader{};

        file.read(
            reinterpret_cast<char*>(
                &resourceHeader),
            sizeof(resourceHeader));

        SVResource resource{};

        resource.Name.resize(
            resourceHeader.NameLength);

        file.read(
            resource.Name.data(),
            resourceHeader.NameLength);

        resource.Set =
            resourceHeader.Set;

        resource.Binding =
            resourceHeader.Binding;

        resources.push_back(
            std::move(resource));
    }
}