#ifndef RENDERGRAPH_PASSDESC_H
#define RENDERGRAPH_PASSDESC_H

#include "./ComputePass.h"
#include <string>
#include <vector>
#include <utility>

namespace FCT
{
    enum class PassType {
        Rasterization,
        Compute,
        ExternalInterop
    };

    struct PassDesc
    {
        std::string name;
        PassType type = PassType::Rasterization;
        EnablePassClear clear = EnablePassClear();
        std::vector<Texture> textures;
        std::vector<Target> targets;
        std::vector<DepthStencil> depthStencils;
        IComputeDispatcher* computeDispatcher = nullptr;

        PassDesc() = default;
        PassDesc(std::string name) : name(std::move(name))
        {
        }

        template<typename... Rest>
        void processArgs(PassType passType, Rest&&... rest)
        {
            type = passType;
            processArgs(std::forward<Rest>(rest)...);
        }

        template<typename... Rest>
        void processArgs(IComputeDispatcher* dispatcher, Rest&&... rest)
        {
            computeDispatcher = dispatcher;
            if (type == PassType::Rasterization) {
                type = PassType::Compute;
            }
            processArgs(std::forward<Rest>(rest)...);
        }

        template<typename... Rest>
        void processArgs(const EnablePassClear& other, Rest&&... rest)
        {
            clear = other;
            processArgs(std::forward<Rest>(rest)...);
        }

        template<typename... Rest>
        void processArgs(const Texture& other, Rest&&... rest)
        {
            textures.push_back(other);
            processArgs(std::forward<Rest>(rest)...);
        }

        template<typename... Rest>
        void processArgs(const Target& other, Rest&&... rest)
        {
            targets.push_back(other);
            processArgs(std::forward<Rest>(rest)...);
        }

        template<typename... Rest>
        void processArgs(const DepthStencil& other, Rest&&... rest)
        {
            depthStencils.push_back(other);
            processArgs(std::forward<Rest>(rest)...);
        }

        void processArgs() noexcept {}
    };
}
#endif //RENDERGRAPH_PASSDESC_H
