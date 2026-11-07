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
}