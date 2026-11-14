#include "./WGL2_ContextCommon.h"

#if !defined(__EMSCRIPTEN__)
#include <GLFW/glfw3.h>

#define X(type, name) type name = nullptr;
WGL2_GL_FUNCTION_LIST(X)
#undef X

#endif

namespace FCT {

#if !defined(__EMSCRIPTEN__)
static void stub_glEnable(GLenum) {}
static void stub_glDisable(GLenum) {}
static void stub_glCullFace(GLenum) {}
static void stub_glFrontFace(GLenum) {}
static void stub_glDepthMask(GLboolean) {}
static void stub_glDepthFunc(GLenum) {}
static void stub_glStencilMask(GLuint) {}
static void stub_glStencilFunc(GLenum, GLint, GLuint) {}
static void stub_glStencilOp(GLenum, GLenum, GLenum) {}
static void stub_glBlendFuncSeparate(GLenum, GLenum, GLenum, GLenum) {}
static void stub_glBlendEquationSeparate(GLenum, GLenum) {}
static void stub_glColorMask(GLboolean, GLboolean, GLboolean, GLboolean) {}
static void stub_glViewport(GLint, GLint, GLsizei, GLsizei) {}
static void stub_glScissor(GLint, GLint, GLsizei, GLsizei) {}
static void stub_glClear(GLbitfield) {}
static void stub_glClearColor(GLfloat, GLfloat, GLfloat, GLfloat) {}
static void stub_glClearDepthf(GLfloat) {}
static void stub_glClearStencil(GLint) {}
static void stub_glGenBuffers(GLsizei, GLuint*) {}
static void stub_glDeleteBuffers(GLsizei, const GLuint*) {}
static void stub_glBindBuffer(GLenum, GLuint) {}
static void stub_glBufferData(GLenum, GLsizeiptr, const void*, GLenum) {}
static void stub_glBufferSubData(GLenum, GLintptr, GLsizeiptr, const void*) {}
static void stub_glBindBufferRange(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr) {}
static void stub_glBindBufferBase(GLenum, GLuint, GLuint) {}
static void stub_glGenTextures(GLsizei, GLuint*) {}
static void stub_glDeleteTextures(GLsizei, const GLuint*) {}
static void stub_glBindTexture(GLenum, GLuint) {}
static void stub_glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
static void stub_glTexImage3D(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
static void stub_glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*) {}
static void stub_glTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void*) {}
static void stub_glTexStorage2D(GLenum, GLsizei, GLenum, GLsizei, GLsizei) {}
static void stub_glTexStorage3D(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei) {}
static void stub_glTexParameteri(GLenum, GLenum, GLint) {}
static void stub_glActiveTexture(GLenum) {}
static void stub_glPixelStorei(GLenum, GLint) {}
static void stub_glGenVertexArrays(GLsizei, GLuint*) {}
static void stub_glDeleteVertexArrays(GLsizei, const GLuint*) {}
static void stub_glBindVertexArray(GLuint) {}
static void stub_glEnableVertexAttribArray(GLuint) {}
static void stub_glDisableVertexAttribArray(GLuint) {}
static void stub_glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) {}
static void stub_glVertexAttribIPointer(GLuint, GLint, GLenum, GLsizei, const void*) {}
static void stub_glVertexAttribDivisor(GLuint, GLuint) {}
static void stub_glGenFramebuffers(GLsizei, GLuint*) {}
static void stub_glDeleteFramebuffers(GLsizei, const GLuint*) {}
static void stub_glBindFramebuffer(GLenum, GLuint) {}
static void stub_glFramebufferTexture2D(GLenum, GLenum, GLenum, GLuint, GLint) {}
static void stub_glFramebufferRenderbuffer(GLenum, GLenum, GLenum, GLuint) {}
static GLenum stub_glCheckFramebufferStatus(GLenum) { return GL_FRAMEBUFFER_COMPLETE; }
static void stub_glDrawBuffers(GLsizei, const GLenum*) {}
static void stub_glGenRenderbuffers(GLsizei, GLuint*) {}
static void stub_glDeleteRenderbuffers(GLsizei, const GLuint*) {}
static void stub_glBindRenderbuffer(GLenum, GLuint) {}
static void stub_glRenderbufferStorage(GLenum, GLenum, GLsizei, GLsizei) {}
static void stub_glRenderbufferStorageMultisample(GLenum, GLsizei, GLenum, GLsizei, GLsizei) {}
static void stub_glGenSamplers(GLsizei, GLuint*) {}
static void stub_glDeleteSamplers(GLsizei, const GLuint*) {}
static void stub_glBindSampler(GLuint, GLuint) {}
static void stub_glSamplerParameteri(GLuint, GLenum, GLint) {}
static void stub_glSamplerParameterf(GLuint, GLenum, GLfloat) {}
static GLuint stub_glCreateShader(GLenum) { return 1; }
static void stub_glDeleteShader(GLuint) {}
static void stub_glShaderSource(GLuint, GLsizei, const GLchar* const*, const GLint*) {}
static void stub_glCompileShader(GLuint) {}
static void stub_glGetShaderiv(GLuint, GLenum, GLint* params) { if (params) *params = GL_TRUE; }
static void stub_glGetShaderInfoLog(GLuint, GLsizei, GLsizei*, GLchar*) {}
static GLuint stub_glCreateProgram() { return 1; }
static void stub_glDeleteProgram(GLuint) {}
static void stub_glAttachShader(GLuint, GLuint) {}
static void stub_glDetachShader(GLuint, GLuint) {}
static void stub_glLinkProgram(GLuint) {}
static void stub_glUseProgram(GLuint) {}
static void stub_glGetProgramiv(GLuint, GLenum, GLint* params) { if (params) *params = GL_TRUE; }
static void stub_glGetProgramInfoLog(GLuint, GLsizei, GLsizei*, GLchar*) {}
static GLint stub_glGetUniformLocation(GLuint, const GLchar*) { return 0; }
static GLuint stub_glGetUniformBlockIndex(GLuint, const GLchar*) { return 0; }
static void stub_glUniformBlockBinding(GLuint, GLuint, GLuint) {}
static void stub_glUniform1i(GLint, GLint) {}
static void stub_glDrawArrays(GLenum, GLint, GLsizei) {}
static void stub_glDrawArraysInstanced(GLenum, GLint, GLsizei, GLsizei) {}
static void stub_glDrawElements(GLenum, GLsizei, GLenum, const void*) {}
static void stub_glDrawElementsInstanced(GLenum, GLsizei, GLenum, const void*, GLsizei) {}
static void stub_glInvalidateFramebuffer(GLenum, GLsizei, const GLenum*) {}
static void stub_glGetIntegerv(GLenum pname, GLint* data) {
    if (data) {
        if (pname == GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT) *data = 256;
        else if (pname == GL_MAX_UNIFORM_BLOCK_SIZE) *data = 65536;
        else *data = 0;
    }
}
static GLenum stub_glGetError() { return GL_NO_ERROR; }
static GLsync stub_glFenceSync(GLenum, GLbitfield) { return reinterpret_cast<GLsync>(1); }
static GLenum stub_glClientWaitSync(GLsync, GLbitfield, GLuint64) { return GL_ALREADY_SIGNALED; }
static void stub_glDeleteSync(GLsync) {}
static void stub_glGenTransformFeedbacks(GLsizei n, GLuint* ids) {
    static GLuint s_nextXfb = 100;
    if (ids) {
        for (GLsizei i = 0; i < n; ++i) ids[i] = s_nextXfb++;
    }
}
static void stub_glDeleteTransformFeedbacks(GLsizei, const GLuint*) {}
static void stub_glBindTransformFeedback(GLenum, GLuint) {}
static void stub_glBeginTransformFeedback(GLenum) {}
static void stub_glEndTransformFeedback() {}
static void stub_glPauseTransformFeedback() {}
static void stub_glResumeTransformFeedback() {}
static void stub_glTransformFeedbackVaryings(GLuint, GLsizei, const GLchar* const*, GLenum) {}
static void stub_glGetTransformFeedbackVarying(GLuint, GLuint, GLsizei, GLsizei*, GLsizei*, GLenum*, GLchar*) {}

static void assignDefaultStubs() {
    if (!glEnable) glEnable = stub_glEnable;
    if (!glDisable) glDisable = stub_glDisable;
    if (!glCullFace) glCullFace = stub_glCullFace;
    if (!glFrontFace) glFrontFace = stub_glFrontFace;
    if (!glDepthMask) glDepthMask = stub_glDepthMask;
    if (!glDepthFunc) glDepthFunc = stub_glDepthFunc;
    if (!glStencilMask) glStencilMask = stub_glStencilMask;
    if (!glStencilFunc) glStencilFunc = stub_glStencilFunc;
    if (!glStencilOp) glStencilOp = stub_glStencilOp;
    if (!glBlendFuncSeparate) glBlendFuncSeparate = stub_glBlendFuncSeparate;
    if (!glBlendEquationSeparate) glBlendEquationSeparate = stub_glBlendEquationSeparate;
    if (!glColorMask) glColorMask = stub_glColorMask;
    if (!glViewport) glViewport = stub_glViewport;
    if (!glScissor) glScissor = stub_glScissor;
    if (!glClear) glClear = stub_glClear;
    if (!glClearColor) glClearColor = stub_glClearColor;
    if (!glClearDepthf) glClearDepthf = stub_glClearDepthf;
    if (!glClearStencil) glClearStencil = stub_glClearStencil;
    if (!glGenBuffers) glGenBuffers = stub_glGenBuffers;
    if (!glDeleteBuffers) glDeleteBuffers = stub_glDeleteBuffers;
    if (!glBindBuffer) glBindBuffer = stub_glBindBuffer;
    if (!glBufferData) glBufferData = stub_glBufferData;
    if (!glBufferSubData) glBufferSubData = stub_glBufferSubData;
    if (!glBindBufferRange) glBindBufferRange = stub_glBindBufferRange;
    if (!glBindBufferBase) glBindBufferBase = stub_glBindBufferBase;
    if (!glGenTextures) glGenTextures = stub_glGenTextures;
    if (!glDeleteTextures) glDeleteTextures = stub_glDeleteTextures;
    if (!glBindTexture) glBindTexture = stub_glBindTexture;
    if (!glTexImage2D) glTexImage2D = stub_glTexImage2D;
    if (!glTexImage3D) glTexImage3D = stub_glTexImage3D;
    if (!glTexSubImage2D) glTexSubImage2D = stub_glTexSubImage2D;
    if (!glTexSubImage3D) glTexSubImage3D = stub_glTexSubImage3D;
    if (!glTexStorage2D) glTexStorage2D = stub_glTexStorage2D;
    if (!glTexStorage3D) glTexStorage3D = stub_glTexStorage3D;
    if (!glTexParameteri) glTexParameteri = stub_glTexParameteri;
    if (!glActiveTexture) glActiveTexture = stub_glActiveTexture;
    if (!glPixelStorei) glPixelStorei = stub_glPixelStorei;
    if (!glGenVertexArrays) glGenVertexArrays = stub_glGenVertexArrays;
    if (!glDeleteVertexArrays) glDeleteVertexArrays = stub_glDeleteVertexArrays;
    if (!glBindVertexArray) glBindVertexArray = stub_glBindVertexArray;
    if (!glEnableVertexAttribArray) glEnableVertexAttribArray = stub_glEnableVertexAttribArray;
    if (!glDisableVertexAttribArray) glDisableVertexAttribArray = stub_glDisableVertexAttribArray;
    if (!glVertexAttribPointer) glVertexAttribPointer = stub_glVertexAttribPointer;
    if (!glVertexAttribIPointer) glVertexAttribIPointer = stub_glVertexAttribIPointer;
    if (!glVertexAttribDivisor) glVertexAttribDivisor = stub_glVertexAttribDivisor;
    if (!glGenFramebuffers) glGenFramebuffers = stub_glGenFramebuffers;
    if (!glDeleteFramebuffers) glDeleteFramebuffers = stub_glDeleteFramebuffers;
    if (!glBindFramebuffer) glBindFramebuffer = stub_glBindFramebuffer;
    if (!glFramebufferTexture2D) glFramebufferTexture2D = stub_glFramebufferTexture2D;
    if (!glFramebufferRenderbuffer) glFramebufferRenderbuffer = stub_glFramebufferRenderbuffer;
    if (!glCheckFramebufferStatus) glCheckFramebufferStatus = stub_glCheckFramebufferStatus;
    if (!glDrawBuffers) glDrawBuffers = stub_glDrawBuffers;
    if (!glGenRenderbuffers) glGenRenderbuffers = stub_glGenRenderbuffers;
    if (!glDeleteRenderbuffers) glDeleteRenderbuffers = stub_glDeleteRenderbuffers;
    if (!glBindRenderbuffer) glBindRenderbuffer = stub_glBindRenderbuffer;
    if (!glRenderbufferStorage) glRenderbufferStorage = stub_glRenderbufferStorage;
    if (!glRenderbufferStorageMultisample) glRenderbufferStorageMultisample = stub_glRenderbufferStorageMultisample;
    if (!glGenSamplers) glGenSamplers = stub_glGenSamplers;
    if (!glDeleteSamplers) glDeleteSamplers = stub_glDeleteSamplers;
    if (!glBindSampler) glBindSampler = stub_glBindSampler;
    if (!glSamplerParameteri) glSamplerParameteri = stub_glSamplerParameteri;
    if (!glSamplerParameterf) glSamplerParameterf = stub_glSamplerParameterf;
    if (!glCreateShader) glCreateShader = stub_glCreateShader;
    if (!glDeleteShader) glDeleteShader = stub_glDeleteShader;
    if (!glShaderSource) glShaderSource = stub_glShaderSource;
    if (!glCompileShader) glCompileShader = stub_glCompileShader;
    if (!glGetShaderiv) glGetShaderiv = stub_glGetShaderiv;
    if (!glGetShaderInfoLog) glGetShaderInfoLog = stub_glGetShaderInfoLog;
    if (!glCreateProgram) glCreateProgram = stub_glCreateProgram;
    if (!glDeleteProgram) glDeleteProgram = stub_glDeleteProgram;
    if (!glAttachShader) glAttachShader = stub_glAttachShader;
    if (!glDetachShader) glDetachShader = stub_glDetachShader;
    if (!glLinkProgram) glLinkProgram = stub_glLinkProgram;
    if (!glUseProgram) glUseProgram = stub_glUseProgram;
    if (!glGetProgramiv) glGetProgramiv = stub_glGetProgramiv;
    if (!glGetProgramInfoLog) glGetProgramInfoLog = stub_glGetProgramInfoLog;
    if (!glGetUniformLocation) glGetUniformLocation = stub_glGetUniformLocation;
    if (!glGetUniformBlockIndex) glGetUniformBlockIndex = stub_glGetUniformBlockIndex;
    if (!glUniformBlockBinding) glUniformBlockBinding = stub_glUniformBlockBinding;
    if (!glUniform1i) glUniform1i = stub_glUniform1i;
    if (!glDrawArrays) glDrawArrays = stub_glDrawArrays;
    if (!glDrawArraysInstanced) glDrawArraysInstanced = stub_glDrawArraysInstanced;
    if (!glDrawElements) glDrawElements = stub_glDrawElements;
    if (!glDrawElementsInstanced) glDrawElementsInstanced = stub_glDrawElementsInstanced;
    if (!glInvalidateFramebuffer) glInvalidateFramebuffer = stub_glInvalidateFramebuffer;
    if (!glGetIntegerv) glGetIntegerv = stub_glGetIntegerv;
    if (!glGetError) glGetError = stub_glGetError;
    if (!glFenceSync) glFenceSync = stub_glFenceSync;
    if (!glClientWaitSync) glClientWaitSync = stub_glClientWaitSync;
    if (!glDeleteSync) glDeleteSync = stub_glDeleteSync;
    if (!glGenTransformFeedbacks) glGenTransformFeedbacks = stub_glGenTransformFeedbacks;
    if (!glDeleteTransformFeedbacks) glDeleteTransformFeedbacks = stub_glDeleteTransformFeedbacks;
    if (!glBindTransformFeedback) glBindTransformFeedback = stub_glBindTransformFeedback;
    if (!glBeginTransformFeedback) glBeginTransformFeedback = stub_glBeginTransformFeedback;
    if (!glEndTransformFeedback) glEndTransformFeedback = stub_glEndTransformFeedback;
    if (!glPauseTransformFeedback) glPauseTransformFeedback = stub_glPauseTransformFeedback;
    if (!glResumeTransformFeedback) glResumeTransformFeedback = stub_glResumeTransformFeedback;
    if (!glTransformFeedbackVaryings) glTransformFeedbackVaryings = stub_glTransformFeedbackVaryings;
    if (!glGetTransformFeedbackVarying) glGetTransformFeedbackVarying = stub_glGetTransformFeedbackVarying;
}
#endif

void initWGL2Functions()
{
#if !defined(__EMSCRIPTEN__)
    assignDefaultStubs();
    // 尝试从活动上下文加载实际函数
#define LOAD_PROC(type, name) \
    if (auto p = reinterpret_cast<type>(glfwGetProcAddress(#name))) name = p;
    WGL2_GL_FUNCTION_LIST(LOAD_PROC)
#undef LOAD_PROC
#endif
}

GLenum WGL2_TranslateInternalFormat(Format format)
{
    switch (format) {
    case Format::R8_UNORM: return GL_R8;
    case Format::R8G8_UNORM: return GL_RG8;
    case Format::R8G8B8A8_UNORM: return GL_RGBA8;
    case Format::R8G8B8A8_SRGB: return GL_SRGB8_ALPHA8;
    case Format::B8G8R8A8_UNORM: return GL_RGBA8;
    case Format::R16_SFLOAT: return GL_R16F;
    case Format::R16G16_SFLOAT: return GL_RG16F;
    case Format::R16G16B16A16_SFLOAT: return GL_RGBA16F;
    case Format::R32_SFLOAT: return GL_R32F;
    case Format::R32G32_SFLOAT: return GL_RG32F;
    case Format::R32G32B32A32_SFLOAT: return GL_RGBA32F;
    case Format::R32_UINT: return GL_R32UI;
    case Format::R32G32_UINT: return GL_RG32UI;
    case Format::R32G32B32A32_UINT: return GL_RGBA32UI;
    case Format::D16_UNORM: return GL_DEPTH_COMPONENT16;
    case Format::D24_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8;
    case Format::D32_SFLOAT: return GL_DEPTH_COMPONENT32F;
    case Format::D32_SFLOAT_S8_UINT: return GL_DEPTH32F_STENCIL8;
    default: return GL_RGBA8;
    }
}

GLenum WGL2_TranslateFormat(Format format)
{
    switch (format) {
    case Format::R8_UNORM:
    case Format::R16_SFLOAT:
    case Format::R32_SFLOAT:
        return GL_RED;
    case Format::R8G8_UNORM:
    case Format::R16G16_SFLOAT:
    case Format::R32G32_SFLOAT:
        return GL_RG;
    case Format::R32_UINT:
        return GL_RED_INTEGER;
    case Format::R32G32_UINT:
        return GL_RG_INTEGER;
    case Format::R32G32B32A32_UINT:
        return GL_RGBA_INTEGER;
    case Format::D16_UNORM:
    case Format::D32_SFLOAT:
        return GL_DEPTH_COMPONENT;
    case Format::D24_UNORM_S8_UINT:
    case Format::D32_SFLOAT_S8_UINT:
        return GL_DEPTH_STENCIL;
    default:
        return GL_RGBA;
    }
}

GLenum WGL2_TranslateType(Format format)
{
    switch (format) {
    case Format::R8_UNORM:
    case Format::R8G8_UNORM:
    case Format::R8G8B8A8_UNORM:
    case Format::R8G8B8A8_SRGB:
    case Format::B8G8R8A8_UNORM:
        return GL_UNSIGNED_BYTE;
    case Format::R16_SFLOAT:
    case Format::R16G16_SFLOAT:
    case Format::R16G16B16A16_SFLOAT:
        return GL_HALF_FLOAT;
    case Format::R32_SFLOAT:
    case Format::R32G32_SFLOAT:
    case Format::R32G32B32A32_SFLOAT:
    case Format::D32_SFLOAT:
        return GL_FLOAT;
    case Format::R32_UINT:
    case Format::R32G32_UINT:
    case Format::R32G32B32A32_UINT:
        return GL_UNSIGNED_INT;
    case Format::D16_UNORM:
        return GL_UNSIGNED_SHORT;
    case Format::D24_UNORM_S8_UINT:
        return GL_UNSIGNED_INT_24_8;
    case Format::D32_SFLOAT_S8_UINT:
        return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    default:
        return GL_UNSIGNED_BYTE;
    }
}

uint32_t WGL2_GetFormatBytesPerPixel(Format format)
{
    switch (format) {
    case Format::R8_UNORM: return 1;
    case Format::R8G8_UNORM: return 2;
    case Format::R8G8B8A8_UNORM:
    case Format::R8G8B8A8_SRGB:
    case Format::B8G8R8A8_UNORM:
    case Format::R32_SFLOAT:
    case Format::R32_UINT:
    case Format::D24_UNORM_S8_UINT:
    case Format::D32_SFLOAT:
        return 4;
    case Format::R16G16B16A16_SFLOAT:
    case Format::R32G32_SFLOAT:
    case Format::R32G32_UINT:
    case Format::D32_SFLOAT_S8_UINT:
        return 8;
    case Format::R32G32B32A32_SFLOAT:
    case Format::R32G32B32A32_UINT:
        return 16;
    default: return 4;
    }
}

WGL2_ContextCommon::WGL2_ContextCommon()
{
    initWGL2Functions();
}

} // namespace FCT
