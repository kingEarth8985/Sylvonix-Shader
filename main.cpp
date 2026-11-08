#include <iostream>
#include <string>
#include "slangcomp.hpp"

static constexpr const char* VERSION =
    "0.1.0";

struct CompileOptions
{
    std::string type;
    std::string path;
};

void ReadBack(
    const std::string& path)
{
    SVFile file;

    file.Load(path);

    std::cout
        << "=== READBACK ===\n";

    for(const auto& stage :
        file.GetStages())
    {
        std::cout
            << "\nName: "
            << stage.Name << '\n'
            << "Stage: " << stage.Stage
            << '\n';

        std::cout
            << "SPIR-V Size: "
            << stage.Spirv.size() * sizeof(uint32_t)
            << " bytes\n";
    }

    for(const auto& parameter :
        file.GetResources())
    {
        std::cout
            << "\nParameter: "
            << parameter.Name
            << '\n';

        std::cout
            << "  Set: "
            << parameter.Set
            << '\n';

        std::cout
            << "  Binding: "
            << parameter.Binding
            << '\n';
    }
}

int main(
    int argc,
    char** argv)
{
    if(argc < 2)
    {
        std::cout
            << "SVShaderCompiler\n"
            << "Use --help for usage\n";

        return 0;
    }

    std::string command =
        argv[1];

    if(command == "--version")
    {
        std::cout
            << "SVShaderCompiler "
            << VERSION
            << '\n';

        return 0;
    }

    if(command == "--readback")
    {
        if(argc < 3)
        {
            std::cerr
                << "Usage: --readback <file.hsvs>\n";

            return 1;
        }

        ReadBack(
            argv[2]);

        return 0;
    }

    CompileOptions options;

    for(int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if(arg == "-t" || arg == "--type")
        {
            if(i + 1 >= argc)
            {
                std::cerr
                    << "Missing shader type\n";

                return 1;
            }

            options.type =
                argv[++i];
        }
        else
        {
            options.path =
                arg;
        }
    }

    std::cout
        << "Type: "
        << options.type
        << '\n';

    std::cout
        << "Path: "
        << options.path
        << '\n';
        
    SlangCompiler compiler(
        options.path);
    
    compiler.Compile();

    return 1;
}