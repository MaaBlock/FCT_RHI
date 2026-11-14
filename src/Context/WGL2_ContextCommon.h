#pragma once

#ifndef __gl_h_
#define __gl_h_ 1
#endif
#ifndef __GL_H__
#define __GL_H__ 1
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include "../ThirdParty.h"
#include "./Format.h"

#if defined(__EMSCRIPTEN__)
#define GL_GLES_PROTOTYPES 1
#include <GLES3/gl3.h>
#include <GLES3/gl2ext.h>
#else
#define GL_GLES_PROTOTYPES 0
#include <GLES3/gl3.h>

#define WGL2_GL_FUNCTION_LIST(X) \
    X(PFNGLENABLEPROC, glEnable) \
    X(PFNGLDISABLEPROC, glDisable) \
    X(PFNGLCULLFACEPROC, glCullFace) \
    X(PFNGLFRONTFACEPROC, glFrontFace) \
    X(PFNGLDEPTHMASKPROC, glDepthMask) \
    X(PFNGLDEPTHFUNCPROC, glDepthFunc) \
    X(PFNGLSTENCILMASKPROC, glStencilMask) \
    X(PFNGLSTENCILFUNCPROC, glStencilFunc) \
    X(PFNGLSTENCILOPPROC, glStencilOp) \
    X(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate) \
    X(PFNGLBLENDEQUATIONSEPARATEPROC, glBlendEquationSeparate) \
    X(PFNGLCOLORMASKPROC, glColorMask) \
    X(PFNGLVIEWPORTPROC, glViewport) \
    X(PFNGLSCISSORPROC, glScissor) \
    X(PFNGLCLEARPROC, glClear) \
    X(PFNGLCLEARCOLORPROC, glClearColor) \
    X(PFNGLCLEARDEPTHFPROC, glClearDepthf) \
    X(PFNGLCLEARSTENCILPROC, glClearStencil) \
    X(PFNGLGENBUFFERSPROC, glGenBuffers) \
    X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers) \
    X(PFNGLBINDBUFFERPROC, glBindBuffer) \
    X(PFNGLBUFFERDATAPROC, glBufferData) \
    X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
    X(PFNGLBINDBUFFERRANGEPROC, glBindBufferRange) \
    X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \
    X(PFNGLGENTEXTURESPROC, glGenTextures) \
    X(PFNGLDELETETEXTURESPROC, glDeleteTextures) \
    X(PFNGLBINDTEXTUREPROC, glBindTexture) \
    X(PFNGLTEXIMAGE2DPROC, glTexImage2D) \
    X(PFNGLTEXIMAGE3DPROC, glTexImage3D) \
    X(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D) \
    X(PFNGLTEXSUBIMAGE3DPROC, glTexSubImage3D) \
    X(PFNGLTEXSTORAGE2DPROC, glTexStorage2D) \
    X(PFNGLTEXSTORAGE3DPROC, glTexStorage3D) \
    X(PFNGLTEXPARAMETERIPROC, glTexParameteri) \
    X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
    X(PFNGLPIXELSTOREIPROC, glPixelStorei) \
    X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
    X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays) \
    X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
    X(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray) \
    X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
    X(PFNGLVERTEXATTRIBIPOINTERPROC, glVertexAttribIPointer) \
    X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
    X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
    X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers) \
    X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
    X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
    X(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer) \
    X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
    X(PFNGLDRAWBUFFERSPROC, glDrawBuffers) \
    X(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers) \
    X(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers) \
    X(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer) \
    X(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage) \
    X(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glRenderbufferStorageMultisample) \
    X(PFNGLGENSAMPLERSPROC, glGenSamplers) \
    X(PFNGLDELETESAMPLERSPROC, glDeleteSamplers) \
    X(PFNGLBINDSAMPLERPROC, glBindSampler) \
    X(PFNGLSAMPLERPARAMETERIPROC, glSamplerParameteri) \
    X(PFNGLSAMPLERPARAMETERFPROC, glSamplerParameterf) \
    X(PFNGLCREATESHADERPROC, glCreateShader) \
    X(PFNGLDELETESHADERPROC, glDeleteShader) \
    X(PFNGLSHADERSOURCEPROC, glShaderSource) \
    X(PFNGLCOMPILESHADERPROC, glCompileShader) \
    X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
    X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
    X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
    X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
    X(PFNGLATTACHSHADERPROC, glAttachShader) \
    X(PFNGLDETACHSHADERPROC, glDetachShader) \
    X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
    X(PFNGLUSEPROGRAMPROC, glUseProgram) \
    X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
    X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
    X(PFNGLGETUNIFORMBLOCKINDEXPROC, glGetUniformBlockIndex) \
    X(PFNGLUNIFORMBLOCKBINDINGPROC, glUniformBlockBinding) \
    X(PFNGLUNIFORM1IPROC, glUniform1i) \
    X(PFNGLDRAWARRAYSPROC, glDrawArrays) \
    X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
    X(PFNGLDRAWELEMENTSPROC, glDrawElements) \
    X(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced) \
    X(PFNGLINVALIDATEFRAMEBUFFERPROC, glInvalidateFramebuffer) \
    X(PFNGLGETINTEGERVPROC, glGetIntegerv) \
    X(PFNGLGETERRORPROC, glGetError) \
    X(PFNGLFENCESYNCPROC, glFenceSync) \
    X(PFNGLCLIENTWAITSYNCPROC, glClientWaitSync) \
    X(PFNGLDELETESYNCPROC, glDeleteSync) \
    X(PFNGLGENTRANSFORMFEEDBACKSPROC, glGenTransformFeedbacks) \
    X(PFNGLDELETETRANSFORMFEEDBACKSPROC, glDeleteTransformFeedbacks) \
    X(PFNGLBINDTRANSFORMFEEDBACKPROC, glBindTransformFeedback) \
    X(PFNGLBEGINTRANSFORMFEEDBACKPROC, glBeginTransformFeedback) \
    X(PFNGLENDTRANSFORMFEEDBACKPROC, glEndTransformFeedback) \
    X(PFNGLPAUSETRANSFORMFEEDBACKPROC, glPauseTransformFeedback) \
    X(PFNGLRESUMETRANSFORMFEEDBACKPROC, glResumeTransformFeedback) \
    X(PFNGLTRANSFORMFEEDBACKVARYINGSPROC, glTransformFeedbackVaryings) \
    X(PFNGLGETTRANSFORMFEEDBACKVARYINGPROC, glGetTransformFeedbackVarying)

#define X(type, name) extern type name;
WGL2_GL_FUNCTION_LIST(X)
#undef X

#endif

namespace FCT {

/**
 * @brief 初始化 WGL2 / GLES 3.0 函数表
 */
void initWGL2Functions();

/**
 * @brief 格式枚举向 OpenGL ES 3.0 内部格式转换
 */
GLenum WGL2_TranslateInternalFormat(Format format);

/**
 * @brief 格式枚举向 OpenGL ES 3.0 外部格式转换
 */
GLenum WGL2_TranslateFormat(Format format);

/**
 * @brief 格式枚举向 OpenGL ES 3.0 数据类型转换
 */
GLenum WGL2_TranslateType(Format format);

/**
 * @brief 获取格式单像素字节大小
 */
uint32_t WGL2_GetFormatBytesPerPixel(Format format);

class WGL2_ContextCommon {
public:
    WGL2_ContextCommon();
    ~WGL2_ContextCommon() = default;
};

} // namespace FCT
