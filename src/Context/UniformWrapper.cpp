#include "../RHI/ConstBuffer.h"
#include "UniformWrapper.h"
#include "Context.h"
#include "Context.hpp"
namespace FCT {
    Uniform::Uniform(): m_uniformBuffer(nullptr), m_constBuffer(nullptr)
    {
    }

    Uniform::Uniform(Uniform&& other) noexcept: m_uniformBuffer(other.m_uniformBuffer), m_constBuffer(other.m_constBuffer)
    {
        other.m_uniformBuffer = nullptr;
        other.m_constBuffer = nullptr;
    }

    Uniform& Uniform::operator=(Uniform&& other) noexcept
    {
        if (this != &other) {
            if (m_uniformBuffer)
            {
                delete m_uniformBuffer;
            }
            if (m_constBuffer)
            {
                delete m_constBuffer;
            }

            m_uniformBuffer = other.m_uniformBuffer;
            m_constBuffer = other.m_constBuffer;

            other.m_uniformBuffer = nullptr;
            other.m_constBuffer = nullptr;
        }
        return *this;
    }

    Uniform::Uniform(FCT::Context* ctx, const FCT::ConstLayout& layout)
    {
        m_uniformBuffer = new FCT::UniformBuffer(layout);
        m_constBuffer = ctx->createResource<FCT::RHI::ConstBuffer>();
        m_constBuffer->layout(layout);
        m_constBuffer->buffer(m_uniformBuffer);
        m_constBuffer->create();
    }

    Uniform::~Uniform()
    {
        if (m_uniformBuffer)
        {
            delete m_uniformBuffer;
        }
        if (m_constBuffer)
        {
            delete m_constBuffer;
        }
    }

    void Uniform::update()
    {
        if (m_uniformBuffer && m_constBuffer) { m_constBuffer->upload(m_uniformBuffer->getData(), m_uniformBuffer->getSize()); }
        m_constBuffer->updataData();
    }

    Uniform::operator RHI::ConstBuffer*() const
    {
        return m_constBuffer;
    }

    FCT::RHI::ConstBuffer* Uniform::getConstBuffer() const
    {
        return m_constBuffer;
    }
}
