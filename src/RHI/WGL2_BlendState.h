#pragma once

#include "./BlendState.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_BlendState : public BlendState {
public:
    explicit WGL2_BlendState(WGL2_Context* ctx) : m_ctx(ctx) {}
    ~WGL2_BlendState() override = default;

    void create() override {}

    using BlendTarget = Target;
    const std::vector<Target>& getTargets() const noexcept { return m_targets; }
    const Target& getTarget(size_t i = 0) const noexcept { return m_targets.at(i); }

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT
