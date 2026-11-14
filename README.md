# FCT_RHI

[简体中文 (数据驱动 · 数据配置流程 · 微内核插件制)](README_zh.md) | [MQEngine](https://github.com/MaaBlock/MQEngine) | [FCT Modules](https://github.com/MaaBlock/FCT)

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![Backends](https://img.shields.io/badge/RHI-Vulkan%20%7C%20DirectX%2012%20%7C%20WebGL2-AC162C)](https://github.com/MaaBlock/FCT_RHI)
[![Platforms](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20Android%20%7C%20Wasm-2ea44f)](https://github.com/MaaBlock/FCT_RHI)

**FCT_RHI** is a cross-platform C++20 **Render Hardware Interface (RHI)** and **RenderGraph** library supporting **Vulkan**, **DirectX 12**, and **WebGL2 / WebAssembly**. Built around **data-driven rendering**, a **declarative data configuration workflow**, and a **microkernel plugin architecture**, it serves as the graphics foundation of [MQEngine](https://github.com/MaaBlock/MQEngine) and can also be integrated standalone.

## Features

### Data-Driven Rendering
- **Declarative RenderGraph**: Describe render passes and resource inputs/outputs without manual synchronization boilerplate. Render target sizing, pass merging, transient VRAM reuse, and pipeline barriers are handled automatically.
- **Modern GPU Capabilities**: Unified support for rasterization, compute shaders, hardware ray tracing, mesh shaders, Bindless textures, and GPU indirect drawing across backends.

### Data Configuration Workflow
- **Schema-Based Pipeline Setup**: Configure device capabilities, vertex/pixel formats, uniform blocks, and texture slots as declarative data descriptions rather than imperative API calls.
- **RenderGraph Config Assets**: Load and save render graph topologies via `.pgraph` configuration files for visual graph authoring and rapid iteration.
- **Automatic Shader Reflection & Offline Packaging**: Automatically binds shader resources through reflection; supports live shader compilation on desktop and precompiled shader bytecode for lightweight WebAssembly and MiniGame builds.

### Microkernel & Pluggable Modules
- **On-Demand Backend & Module Mounting**: Keep the core graphics context minimal while plugging in graphics backends (Vulkan, DirectX 12, WebGL2) and extension modules ([ImGui](https://github.com/MaaBlock/FCT_IMGUI), [Node.js / JS](https://github.com/MaaBlock/FCT_Node), [Physics](https://github.com/MaaBlock/FCT_Phys), [Compute](https://github.com/MaaBlock/FCT_Compute), Profiling) as needed.
- **Unified Windowing & Headless Mode**: Run zero-window Headless GPU offscreen rendering (for CI testing and server rendering), single-window games, multi-window desktop editors, Android apps, and Web/MiniGame targets under one unified interface.

## Quick Start

### Requirements
- C++20 compiler (Visual Studio 2022 17.10+, Clang, or GCC)
- CMake 3.22.1+ & [vcpkg](https://github.com/microsoft/vcpkg)
- Vulkan SDK 1.2+ (Windows / Linux / Android), Windows SDK (DirectX 12), or Emscripten (WebGL2 / Wasm)

### Clone & Integrate
```bash
git clone --recursive https://github.com/MaaBlock/FCT_RHI.git
```

```cmake
add_subdirectory(FCT_RHI)
target_link_fct_libraries(YourTarget)
```

## Related Repositories
- **[MQEngine](https://github.com/MaaBlock/MQEngine)** — Data-driven C++20 3D game engine with a microkernel plugin system built on `FCT_RHI`.
- **[FCT](https://github.com/MaaBlock/FCT)** — Modular C++ runtime suite (`FCT_Base`, `FCT_RHI`, `FCT_IMGUI`, `FCT_Node`, `FCT_Phys`, `FCT_Compute`).
