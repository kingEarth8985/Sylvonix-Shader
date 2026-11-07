# Sylvonix Shader

A lightweight C++ shader compiler wrapper built around the Slang compiler from the Vulkan SDK.

## Overview

This project provides a small CLI for compiling shader sources using Slang. It is designed to be easy to build with CMake and to integrate with a Vulkan SDK installation on the machine.

## Features

- CMake-based build setup
- Vulkan SDK integration
- Slang compiler linkage
- Simple CLI entry point for shader compilation
- Basic shader file handling utilities

## Project structure

- `main.cpp` - command-line entry point
- `slangcomp.cpp` / `slangcomp.hpp` - Slang compiler wrapper
- `svshaderFile.cpp` / `svshaderFile.hpp` - shader file logic
- `CMakeLists.txt` - build configuration

## Requirements

- CMake 3.10 or newer
- C++20 support
- Vulkan SDK installed and available in the environment
- Slang library available in `$VULKAN_SDK/lib`

## Build

```bash
export VULKAN_SDK=/path/to/your/vulkan/sdk
cmake -S . -B build
cmake --build build
```

## Usage

The current CLI supports a version flag and a shader type/path pair:

```bash
./build/SylvonixShader --version
./build/SylvonixShader -t <shader-type> <shader-file>
./build/SylvonixShader --type <shader-type> <shader-file>
```

Example:

```bash
./build/SylvonixShader -t fragment ./shaders/example.slang
```

## Notes

This repository is a small experimental shader compiler project. The current implementation prints the selected options and invokes the Slang compiler wrapper for the target shader file.
