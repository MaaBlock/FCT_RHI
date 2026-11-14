# FCT_RHI

[English](README.md) | [MQEngine](https://github.com/MaaBlock/MQEngine) | [FCT 模块集](https://github.com/MaaBlock/FCT)

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![Backends](https://img.shields.io/badge/RHI-Vulkan%20%7C%20DirectX%2012%20%7C%20WebGL2-AC162C)](https://github.com/MaaBlock/FCT_RHI)
[![Platforms](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20Android%20%7C%20Wasm-2ea44f)](https://github.com/MaaBlock/FCT_RHI)

`FCT_RHI` 是基于 C++20 的跨平台**渲染硬件接口（RHI）**与**渲染图（RenderGraph）**图形库，支持 **Vulkan**、**DirectX 12** 与 **WebGL2 / WebAssembly** 后端，围绕**数据驱动渲染**、**声明式数据配置流程**与**微内核插件制**提供统一的图形能力，既作为 [MQEngine](https://github.com/MaaBlock/MQEngine) 的图形底座使用，也可单独接入其他图形工程。

## 功能特性

### 数据驱动渲染
- 只需声明渲染通道及其输入输出附件，库内自动推导目标分辨率、合并渲染通道、复用瞬态显存并生成同步屏障，免去手写底层同步代码。
- 跨后端统一支持传统光栅化、通用计算着色器、硬件光线追踪、网格着色器（Mesh Shader）、Bindless 无绑定纹理与 GPU 间接绘制。

### 数据配置流程
- 通过声明式数据结构直接配置硬件能力开关、顶点与像素格式、常量缓冲及纹理槽位，无需逐条编写繁琐的图形 API 绑定调用。
- 支持通过 `.pgraph` 配置文件读写渲染图结构，方便配合可视化节点编辑器调整渲染管线。
- 根据着色器反射信息自动完成资源绑定；桌面端支持实时编译着色器，面向 WebAssembly 与小游戏发布时可切换至预编译离线字节码以缩减包体。

### 微内核与可插拔模块
- 图形上下文保持精简内核，渲染后端（Vulkan、DirectX 12、WebGL2）与扩展功能模块（[ImGui 界面](https://github.com/MaaBlock/FCT_IMGUI)、[Node.js 脚本](https://github.com/MaaBlock/FCT_Node)、[物理模拟](https://github.com/MaaBlock/FCT_Phys)、[通用计算](https://github.com/MaaBlock/FCT_Compute)、性能分析）均按需插拔挂载。
- 在同一套窗口接口下覆盖无窗口 Headless GPU 离屏渲染（适用于 CI 自动化测试与服务端渲染）、桌面单窗口与多窗口、Android 移动端以及 Web / 小游戏宿主。

## 快速上手

### 环境要求
- C++20 编译器（Visual Studio 2022 17.10+、Clang 或 GCC）
- CMake 3.22.1+ 与 [vcpkg](https://github.com/microsoft/vcpkg)
- Vulkan SDK 1.2+（Windows / Linux / Android）、Windows SDK（DirectX 12）或 Emscripten（WebGL2 / Wasm）

### 克隆与接入
```bash
git clone --recursive https://github.com/MaaBlock/FCT_RHI.git
```

```cmake
add_subdirectory(FCT_RHI)
target_link_fct_libraries(YourTarget)
```

## 关联仓库
- [MQEngine](https://github.com/MaaBlock/MQEngine)：基于 `FCT_RHI` 的数据驱动与微内核插件制 C++20 三维游戏引擎。
- [FCT](https://github.com/MaaBlock/FCT)：包含 `FCT_Base`、`FCT_RHI`、`FCT_IMGUI`、`FCT_Node`、`FCT_Phys` 与 `FCT_Compute` 的模块化运行库集合。