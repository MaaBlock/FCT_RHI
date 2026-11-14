#pragma once

#include "./InputLayout.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

/**
 * @brief WebGL 2.0 / GLES 3.0 输入布局与整型实例化属性分发 (FEAT-27)
 */
class WGL2_InputLayout : public InputLayout {
public:
    explicit WGL2_InputLayout(WGL2_Context* ctx);
    ~WGL2_InputLayout() override = default;

    void create() override;

    void applyBinding(uint32_t slot, GLuint vbo);
    size_t getHash() const noexcept;

    const std::map<uint32_t, VertexLayout>& getVertexLayouts() const noexcept { return m_vertexLayouts; }
    const std::map<uint32_t, InputRate>& getInputRates() const noexcept { return m_inputRates; }

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT
