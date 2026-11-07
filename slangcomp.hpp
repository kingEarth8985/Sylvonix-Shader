#pragma once
#include <string>
#include <vector>
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include "svshaderFile.hpp"

class SlangCompiler
{
public:
    SlangCompiler(
        const std::string& path)
        : sourcePath(path)
    {
    }

    void Compile();

private:

    void LoadSlangFile();

    void FindEntryPoints();

    std::vector<uint32_t> CompileSPIRV(slang::IComponentType* program);

    void AddStageToManifest();

    void ReflectResources();

private:

    SVFile svFile;

    std::string sourcePath;

    Slang::ComPtr<slang::IGlobalSession>
        globalSession;

    Slang::ComPtr<slang::ISession>
        session;

    Slang::ComPtr<slang::IModule>
        slangModule;

    std::vector<
        Slang::ComPtr<slang::IEntryPoint>>
        entryPoints;
    
    slang::ProgramLayout* programLayout;
};