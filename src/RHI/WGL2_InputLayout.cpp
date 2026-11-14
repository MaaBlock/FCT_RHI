#include "./WGL2_InputLayout.h"
#include "../Context/WGL2_Context.h"
#include <boost/functional/hash.hpp>

namespace FCT::RHI {

WGL2_InputLayout::WGL2_InputLayout(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

void WGL2_InputLayout::create()
{
}

void WGL2_InputLayout::applyBinding(uint32_t slot, GLuint vbo)
{
    auto itLayout = m_vertexLayouts.find(slot);
    if (itLayout == m_vertexLayouts.end() || vbo == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    bool isPerInstance = false;
    auto itRate = m_inputRates.find(slot);
    if (itRate != m_inputRates.end()) {
        isPerInstance = (itRate->second == InputRate::PerInstance);
    }

    const VertexLayout& layout = itLayout->second;
    size_t stride = layout.getStride();
    size_t elemCount = layout.getElementCount();

    for (size_t i = 0; i < elemCount; ++i) {
        const VertexElement& elem = layout.getElement(i);
        GLuint loc = static_cast<GLuint>(elem.getType());
        size_t offset = layout.getElementOffset(i);
        Format fmt = elem.getFormat();

        glEnableVertexAttribArray(loc);

        // FEAT-27: 严格区分整型与浮点型属性
        if (fmt == Format::R32_UINT || fmt == Format::R32_SINT) {
            GLenum glType = (fmt == Format::R32_UINT) ? GL_UNSIGNED_INT : GL_INT;
            glVertexAttribIPointer(loc, 1, glType, static_cast<GLsizei>(stride),
                                   reinterpret_cast<const void*>(offset));
        } else if (fmt == Format::R32G32_UINT || fmt == Format::R32G32_SINT) {
            GLenum glType = (fmt == Format::R32G32_UINT) ? GL_UNSIGNED_INT : GL_INT;
            glVertexAttribIPointer(loc, 2, glType, static_cast<GLsizei>(stride),
                                   reinterpret_cast<const void*>(offset));
        } else if (fmt == Format::R32G32B32A32_UINT || fmt == Format::R32G32B32A32_SINT) {
            GLenum glType = (fmt == Format::R32G32B32A32_UINT) ? GL_UNSIGNED_INT : GL_INT;
            glVertexAttribIPointer(loc, 4, glType, static_cast<GLsizei>(stride),
                                   reinterpret_cast<const void*>(offset));
        } else {
            GLint compCount = 4;
            if (fmt == Format::R32_SFLOAT) compCount = 1;
            else if (fmt == Format::R32G32_SFLOAT) compCount = 2;
            else if (fmt == Format::R32G32B32_SFLOAT) compCount = 3;
            else if (fmt == Format::R32G32B32A32_SFLOAT) compCount = 4;
            else if (fmt == Format::R8G8B8A8_UNORM) compCount = 4;

            GLenum glType = WGL2_TranslateType(fmt);
            GLboolean norm = (fmt == Format::R8G8B8A8_UNORM) ? GL_TRUE : GL_FALSE;

            glVertexAttribPointer(loc, compCount, glType, norm, static_cast<GLsizei>(stride),
                                  reinterpret_cast<const void*>(offset));
        }

        glVertexAttribDivisor(loc, isPerInstance ? 1 : 0);
    }
}

size_t WGL2_InputLayout::getHash() const noexcept
{
    size_t seed = 0;
    for (const auto& [slot, layout] : m_vertexLayouts) {
        boost::hash_combine(seed, slot);
        boost::hash_combine(seed, layout.getStride());
        boost::hash_combine(seed, layout.getElementCount());
        for (size_t i = 0; i < layout.getElementCount(); ++i) {
            const auto& elem = layout.getElement(i);
            boost::hash_combine(seed, static_cast<uint32_t>(elem.getType()));
            boost::hash_combine(seed, static_cast<uint32_t>(elem.getFormat()));
            boost::hash_combine(seed, layout.getElementOffset(i));
        }
        auto itRate = m_inputRates.find(slot);
        if (itRate != m_inputRates.end()) {
            boost::hash_combine(seed, static_cast<uint32_t>(itRate->second));
        }
    }
    return seed;
}

} // namespace FCT::RHI
