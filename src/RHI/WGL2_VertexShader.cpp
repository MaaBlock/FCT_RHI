#include "./WGL2_VertexShader.h"
#include "../Context/WGL2_Context.h"
#include <iostream>

namespace FCT::RHI {

WGL2_VertexShader::WGL2_VertexShader(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_VertexShader::~WGL2_VertexShader()
{
    if (m_shaderHandle != 0) {
        glDeleteShader(m_shaderHandle);
        m_shaderHandle = 0;
    }
}

void WGL2_VertexShader::create()
{
    if (m_shaderHandle != 0 || m_code.empty()) {
        return;
    }

    m_shaderHandle = glCreateShader(GL_VERTEX_SHADER);
    const char* src = m_code.data();
    GLint length = static_cast<GLint>(m_code.size());
    glShaderSource(m_shaderHandle, 1, &src, &length);
    glCompileShader(m_shaderHandle);

    GLint success = 0;
    glGetShaderiv(m_shaderHandle, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024] = {0};
        glGetShaderInfoLog(m_shaderHandle, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "[WGL2_VertexShader] Compile failed:\n" << infoLog << "\nSource:\n" << std::string(m_code.begin(), m_code.end()) << std::endl;
        glDeleteShader(m_shaderHandle);
        m_shaderHandle = 0;
    }
}

} // namespace FCT::RHI
