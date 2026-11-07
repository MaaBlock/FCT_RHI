#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstring>
#include "DataTypes.h"
#include "../Base/Flags.h"
#include "../Base/string.h"
#include "./ShaderStage.h"

namespace FCT
{
    // Uniform类型枚举
    enum class ConstType {
        // 矩阵类型
        ModelMatrix,
        ViewMatrix,
        ProjectionMatrix,
        MVPMatrix,
        Mat4,
        Mat3,

        // 向量类型
        Vec4,
        Vec3,
        Vec2,

        // 标量类型
        Float,
        Int,
        Bool,

        // 纹理类型
        Texture2D,
        TextureCube,

        // 自定义类型
        Custom
    };

    using UniformType = ConstType;

#ifdef FCT_USE_VULKAN
#include <vulkan/vulkan.hpp>

    inline vk::ShaderStageFlags ConvertToVkShaderStageFlags(ShaderStages stages)
    {
        vk::ShaderStageFlags flags;

        if (stages & ShaderStage::Vertex)
            flags |= vk::ShaderStageFlagBits::eVertex;

        if (stages & ShaderStage::Fragment)
            flags |= vk::ShaderStageFlagBits::eFragment;

        if (stages & ShaderStage::Compute)
            flags |= vk::ShaderStageFlagBits::eCompute;

        if (stages & ShaderStage::Geometry)
            flags |= vk::ShaderStageFlagBits::eGeometry;

        if (stages & ShaderStage::TessControl)
            flags |= vk::ShaderStageFlagBits::eTessellationControl;

        if (stages & ShaderStage::TessEvaluation)
            flags |= vk::ShaderStageFlagBits::eTessellationEvaluation;

        if (stages & ShaderStage::Task)
            flags |= vk::ShaderStageFlagBits::eTaskEXT;

        if (stages & ShaderStage::Mesh)
            flags |= vk::ShaderStageFlagBits::eMeshEXT;

        if (stages & ShaderStage::RayGen)
            flags |= vk::ShaderStageFlagBits::eRaygenKHR;

        if (stages & ShaderStage::AnyHit)
            flags |= vk::ShaderStageFlagBits::eAnyHitKHR;

        if (stages & ShaderStage::ClosestHit)
            flags |= vk::ShaderStageFlagBits::eClosestHitKHR;

        if (stages & ShaderStage::Miss)
            flags |= vk::ShaderStageFlagBits::eMissKHR;

        if (stages & ShaderStage::Intersection)
            flags |= vk::ShaderStageFlagBits::eIntersectionKHR;

        if (stages & ShaderStage::Callable)
            flags |= vk::ShaderStageFlagBits::eCallableKHR;

        if (!flags)
            flags = vk::ShaderStageFlagBits::eAll;

        return flags;
    }
#endif

    // 获取Uniform类型的大小
    constexpr size_t GetUniformSize(ConstType type) {
        switch (type) {
        case ConstType::ModelMatrix:
        case ConstType::ViewMatrix:
        case ConstType::ProjectionMatrix:
        case ConstType::MVPMatrix:
        case ConstType::Mat4:
            return sizeof(Mat4);
        case ConstType::Mat3:
            return sizeof(Mat3);
        case ConstType::Vec4:
            return sizeof(Vec4);
        case ConstType::Vec3:
            return sizeof(Vec3<>);
        case ConstType::Vec2:
            return sizeof(Vec2);
        case ConstType::Float:
            return sizeof(float);
        case ConstType::Int:
        case ConstType::Bool:
            return sizeof(int);
        case ConstType::Texture2D:
        case ConstType::TextureCube:
            return sizeof(int); // 纹理句柄
        case ConstType::Custom:
        default:
            return 0;
        }
    }

    // 获取Uniform类型的对齐要求
    constexpr size_t GetUniformAlignment(ConstType type)
    {
        switch (type) {
        case ConstType::ModelMatrix:
        case ConstType::ViewMatrix:
        case ConstType::ProjectionMatrix:
        case ConstType::MVPMatrix:
        case ConstType::Mat4:
        case ConstType::Mat3:
            return 16; // 矩阵对齐到16字节
        case ConstType::Vec4:
        case ConstType::Vec3:
            return 16; // 向量对齐到16字节
        case ConstType::Vec2:
            return 8;  // Vec2对齐到8字节
        case ConstType::Float:
        case ConstType::Int:
        case ConstType::Bool:
        case ConstType::Texture2D:
        case ConstType::TextureCube:
            return 4;  // 标量和纹理句柄对齐到4字节
        case ConstType::Custom:
        default:
            return 16; // 默认对齐到16字节
        }
    }

    constexpr size_t GetHLSLArrayElementSize(ConstType type) {
        switch (type) {
        case ConstType::Float:
        case ConstType::Int:
        case ConstType::Bool:
        case ConstType::Vec2:
        case ConstType::Vec3:
            return 16; // 16字节

        case ConstType::Vec4:
            return 16;

        case ConstType::Mat3:
            return 48; // 3x3矩阵：3个寄存器，每行按 float4 对齐

        case ConstType::Mat4:
        case ConstType::ModelMatrix:
        case ConstType::ViewMatrix:
        case ConstType::ProjectionMatrix:
        case ConstType::MVPMatrix:
            return 64; // 4x4矩阵：4个寄存器

        default:
            return 16;
        }
    }

    inline constexpr const char* GetUniformDefaultName(ConstType type)
    {
        switch (type)
        {
            // 标准类型 - 不生成警告
        case ConstType::ModelMatrix:
            return "modelMatrix";
        case ConstType::ViewMatrix:
            return "viewMatrix";
        case ConstType::ProjectionMatrix:
            return "projectionMatrix";
        case ConstType::MVPMatrix:
            return "mvpMatrix";
        case ConstType::Mat4:
            return "u_Mat4";
        case ConstType::Mat3:
            return "u_Mat3";
        case ConstType::Vec4:
            return "u_Vec4";
        case ConstType::Vec3:
            return "u_Vec3";
        case ConstType::Vec2:
            return "u_Vec2";
        case ConstType::Float:
            return "u_Float";

        case ConstType::Int:
            return "u_Int";

        case ConstType::Bool:
            return "u_Bool";

        case ConstType::Texture2D:
            return "u_Texture2D";

        case ConstType::TextureCube:
            return "u_TextureCube";

        case ConstType::Custom:
        default:
            return "u_Custom";
        }
    }

    inline constexpr const char* GetUniformTypeName(ConstType type)
    {
        switch (type) {
        case ConstType::ModelMatrix:
        case ConstType::ViewMatrix:
        case ConstType::ProjectionMatrix:
        case ConstType::MVPMatrix:
        case ConstType::Mat4:
            return "float4x4";
        case ConstType::Mat3:
            return "float3x3";
        case ConstType::Vec4:
            return "float4";
        case ConstType::Vec3:
            return "float3";
        case ConstType::Vec2:
            return "float2";
        case ConstType::Float:
            return "float";
        case ConstType::Int:
            return "int";
        case ConstType::Bool:
            return "bool";
        case ConstType::Texture2D:
            return "Texture2D";
        case ConstType::TextureCube:
            return "TextureCube";
        case ConstType::Custom:
        default:
            return "float4";
        }
    }


    class ConstElement {
    public:
        constexpr ConstElement() noexcept
            : m_type(ConstType::Custom), m_name("") {}

        constexpr ConstElement(ConstType type) noexcept
            : m_type(type), m_name(GetUniformDefaultName(type)) {}

        constexpr ConstElement(ConstType type, const char* name) noexcept
            : m_type(type), m_name(name) {}

        constexpr ConstElement(ConstType type, const char* name, size_t arraySize) noexcept
            : m_type(type), m_name(name), m_arraySize(arraySize) {}

        constexpr size_t getElementCount() const noexcept {
            return isArray() ? m_arraySize : 1;
        }
        constexpr bool isArray() const noexcept { return m_arraySize > 0; }

        constexpr ConstType getType() const noexcept { return m_type; }
        constexpr const char* getName() const noexcept { return m_name; }
        constexpr size_t getSize() const noexcept {
            if (isArray()) {
                return GetHLSLArrayElementSize(m_type) * getElementCount();
            }
            return GetUniformSize(m_type);
        }
        constexpr size_t getAlignment() const noexcept { return GetUniformAlignment(m_type); }
        constexpr explicit operator bool() const noexcept {
            return (m_name && m_name[0] != '\0');
        }
    private:
        ConstType m_type;
        const char* m_name;
        size_t m_arraySize = 0;
    };

    class ConstLayout {
    public:
        constexpr ConstLayout() noexcept
        : m_name(""), m_shaderStages(ShaderStage::All), m_updateFrequency(UpdateFrequency::PerFrame),
          m_elementCount(0), m_size(0)
        {
            for (size_t i = 0; i < MaxElements; ++i) {
                m_elements[i] = ConstElement();
                m_offsets[i] = 0;
            }
        }
        template<typename... Args>
        constexpr ConstLayout(const char* layoutName, Args&&... args) noexcept
        : m_name(layoutName), m_updateFrequency(UpdateFrequency::PerFrame)
        {
            processArgs(std::forward<Args>(args)...);
        }

        constexpr const char* getName() const noexcept { return m_name; }

        constexpr UpdateFrequency getUpdateFrequency() const noexcept { return m_updateFrequency; }

        constexpr ShaderStages getShaderStages() const noexcept { return m_shaderStages; }
#ifdef FCT_UNUSE

        constexpr void addElement(const ConstElement& element) noexcept {
            if (m_elementCount < MaxElements) {
                size_t alignment = element.getAlignment();
                m_size = (m_size + alignment - 1) & ~(alignment - 1);

                m_offsets[m_elementCount] = m_size;
                m_elements[m_elementCount++] = element;
                m_size += element.getSize();
            }
        }
#endif
        constexpr void addElement(const ConstElement& element) noexcept {
            if (m_elementCount < MaxElements) {
                size_t size = element.getSize();
                m_size = (m_size + size <= ((m_size + 15) & ~15)) ?
                    m_size : (m_size + 15) & ~15;
                m_offsets[m_elementCount] = m_size;
                m_elements[m_elementCount++] = element;
                m_size += element.getSize();
            }
        }
        bool operator==(const ConstLayout& other) const noexcept {

            if (!StringEquals(m_name, other.m_name)) {
                return false;
            }
            if (m_elementCount != other.m_elementCount) {
                return false;
            }

            for (size_t i = 0; i < m_elementCount; ++i) {
                if (m_elements[i].getType() != other.m_elements[i].getType())
                    return false;
                if (m_elements[i].getElementCount() != other.m_elements[i].getElementCount())
                    return false;
                if (!StringEquals(m_elements[i].getName(), other.m_elements[i].getName()))
                    return false;
            }
            return true;
        }

        size_t getHash() const noexcept {
            size_t seed = 0;
            boost::hash_combine(seed, std::string(m_name));
            for (size_t i = 0; i < m_elementCount; ++i) {
                boost::hash_combine(seed, m_elements[i].getType());
                boost::hash_combine(seed, m_elements[i].getElementCount());
                boost::hash_combine(seed, std::string(m_elements[i].getName()));
            }
            return seed;
        }

        constexpr size_t getElementCount() const noexcept { return m_elementCount; }

        constexpr const ConstElement& getElement(size_t index) const noexcept {
            return (index < m_elementCount) ? m_elements[index] : m_elements[0];
        }

        constexpr const ConstElement& getElement(ConstType type) const noexcept {
            auto index = findElementIndex(type);
            return (index >= 0) ? m_elements[index] : ConstElement();
        }

        constexpr size_t getTotalSize() const noexcept {
            //return (m_size + 15) & ~15;
            return m_size;
        }

        constexpr int findElementIndex(const char* name) const noexcept {
            for (size_t i = 0; i < m_elementCount; ++i) {
                if (strcmp(m_elements[i].getName(), name) == 0) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        constexpr int findElementIndex(ConstType type) const noexcept {
            for (size_t i = 0; i < m_elementCount; ++i) {
                if (m_elements[i].getType() == type) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        constexpr size_t getElementOffset(size_t index) const noexcept {
            return (index < m_elementCount) ? m_offsets[index] : 0;
        }

        constexpr size_t getElementOffset(const char* name) const noexcept {
            int index = findElementIndex(name);
            return (index >= 0) ? m_offsets[index] : 0;
        }

        constexpr size_t getElementOffset(ConstType type) const noexcept {
            int index = findElementIndex(type);
            return (index >= 0) ? m_offsets[index] : 0;
        }

    private:
        constexpr void processArgs() noexcept {

        }

        template<typename... Rest>
        constexpr void processArgs(UpdateFrequency frequency, Rest&&... rest) noexcept {
            m_updateFrequency = frequency;
            processArgs(std::forward<Rest>(rest)...);
        }

        template<typename... Rest>
        constexpr void processArgs(const ConstElement& element, Rest&&... rest) noexcept {
            addElement(element);
            processArgs(std::forward<Rest>(rest)...);
        }

        template<typename... Rest>
        constexpr void processArgs(ShaderStages stages, Rest&&... rest) noexcept {
            m_shaderStages = stages;
            processArgs(std::forward<Rest>(rest)...);
        }

        static constexpr size_t MaxElements = 16;
        const char* m_name = "";
        ShaderStages m_shaderStages = ShaderStage::All;
        UpdateFrequency m_updateFrequency = UpdateFrequency::PerFrame;
        ConstElement m_elements[MaxElements]{};
        size_t m_offsets[MaxElements]{};
        size_t m_elementCount = 0;
        size_t m_size = 0;
    };

    namespace PredefinedUniforms {
        static constexpr ConstLayout MVP = {
            "transform",
            UpdateFrequency::PerFrame,
            ConstElement(ConstType::ModelMatrix),
            ConstElement(ConstType::ViewMatrix),
            ConstElement(ConstType::ProjectionMatrix)
        };

        static constexpr ConstLayout SingleMVP = {
            "mvpTransform",
            ConstElement(ConstType::MVPMatrix),
            UpdateFrequency::PerFrame
        };

        static constexpr ConstLayout ModelViewProj = {
            "combinedTransform",
            ConstElement(ConstType::ModelMatrix),
            UpdateFrequency::PerObject,
            ConstElement(ConstType::MVPMatrix)
        };

        static constexpr ConstLayout BasicPBRMaterial = {
            "pbrMaterial",
            UpdateFrequency::Static,
            ConstElement(ConstType::Vec4, "baseColorFactor"),
            ConstElement(ConstType::Float, "metallicFactor"),
            ConstElement(ConstType::Float, "roughnessFactor"),
            ConstElement(ConstType::Vec3, "emissiveFactor")
        };
    }

    class UniformBuffer {
    public:
        explicit UniformBuffer(const ConstLayout& layout)
            : m_layout(layout), m_size(layout.getTotalSize()), m_dirty(true) {
            m_data.resize(m_size, 0);
        }

        UpdateFrequency getUpdateFrequency() const { return m_layout.getUpdateFrequency(); }

        template<typename T>
        void setValue(const char* name, const T& value) {
            int index = m_layout.findElementIndex(name);
            if (index >= 0) {
                const auto& element = m_layout.getElement(index);
                size_t offset = m_layout.getElementOffset(index);
                if (element.isArray())
                {
                    setArrayValueDispatch(name, value, element, offset);
                }
                else
                {
                    setSingleValue(name, value, element, offset);
                }
            }
        }

        template<typename T>
        void setValue(ConstType type, const T& value) {
            int index = m_layout.findElementIndex(type);
            if (index >= 0) {
                size_t offset = m_layout.getElementOffset(index);
                size_t elementSize = m_layout.getElement(index).getSize();

                if (sizeof(T) <= elementSize) {
                    std::memcpy(m_data.data() + offset, &value, sizeof(T));
                    m_dirty = true;
                }
            }
        }

        template<typename T>
        T getValue(const char* name) const {
            T result{};
            int index = m_layout.findElementIndex(name);
            if (index >= 0) {
                size_t offset = m_layout.getElementOffset(index);
                size_t elementSize = m_layout.getElement(index).getSize();

                if (sizeof(T) <= elementSize) {
                    std::memcpy(&result, m_data.data() + offset, sizeof(T));
                }
            }
            return result;
        }

        const void* getData() const { return m_data.data(); }

        size_t getSize() const { return m_size; }

        const ConstLayout& getLayout() const { return m_layout; }

        bool isDirty() const { return m_dirty; }

        void clearDirty() { m_dirty = false; }
    private:
        template<typename T>
   void setArrayValueDispatch(const char* name, const T& value, const ConstElement& element, size_t offset) {
            if constexpr (std::is_pointer_v<T>) {
                setArrayFromPointer(value, element, offset);
            } else if constexpr (std::ranges::range<T>) {
                setArrayFromContainer(value, element, offset);
            } else
            {

            }
        }

        template<typename T>
        void setArrayFromPointer(T value, const ConstElement& element, size_t offset) {
            static_assert(std::is_pointer_v<T>, "Expected pointer type");

            size_t baseElementSize = GetUniformSize(element.getType());
            size_t elementCount = element.getElementCount();

            for (uint32_t i = 0; i < elementCount; ++i) {
                size_t currentOffset = offset + i * GetHLSLArrayElementSize(element.getType());
                if (currentOffset + baseElementSize <= m_data.size()) {
                    std::memcpy(m_data.data() + currentOffset, &value[i], baseElementSize);
                }
            }
            m_dirty = true;
        }

        template<typename T>
        void setArrayFromContainer(const T& value, const ConstElement& element, size_t offset) {
            size_t baseElementSize = GetUniformSize(element.getType());
            size_t maxElements = element.getElementCount();
            uint32_t arrayIndex = 0;

            for (const auto& it : value) {
                if (arrayIndex >= maxElements) break;

                size_t currentOffset = offset + arrayIndex * GetHLSLArrayElementSize(element.getType());
                if (currentOffset + baseElementSize <= m_data.size()) {
                    std::memcpy(m_data.data() + currentOffset, &it, baseElementSize);
                }
                arrayIndex++;
            }
            m_dirty = true;
        }
        template<typename T>
        void setSingleValue(const char* name, const T& value, const ConstElement& element, size_t offset) {
            size_t elementSize = element.getSize();
            if (sizeof(T) <= elementSize) {
                std::memcpy(m_data.data() + offset, &value, sizeof(T));
                m_dirty = true;
            }
        }
        ConstLayout m_layout;
        std::vector<uint8_t> m_data;
        size_t m_size;
        bool m_dirty;
    };
}