/*
 * Copyright (C) 2009, 2014-2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(GRAPHICS_CONTEXT_GL)

#include "GraphicsContextGL.h"
#include <memory>
#include <wtf/HashCountedSet.h>
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/RetainPtr.h>
#include <wtf/UniqueArray.h>

#if PLATFORM(COCOA)
#include "IOSurface.h"
#include "WebGLLayerClient.h"
#endif

#if USE(CA)
#include "PlatformCALayer.h"
#endif

#if !USE(ANGLE)
#include "ANGLEWebKitBridge.h"
#endif

// FIXME: Find a better way to avoid the name confliction for NO_ERROR.
#if PLATFORM(WIN)
#undef NO_ERROR
#elif PLATFORM(GTK)
// This define is from the X11 headers, but it's used below, so we must undefine it.
#undef VERSION
#endif

#if PLATFORM(COCOA)
OBJC_CLASS CALayer;
OBJC_CLASS WebGLLayer;
#endif // PLATFORM(COCOA)

#if USE(NICOSIA)
namespace Nicosia {
class GCGLLayer;
}
#endif

namespace WebCore {
class ExtensionsGL;
#if USE(ANGLE)
class ExtensionsGLANGLE;
#elif USE(OPENGL_ES)
class ExtensionsGLOpenGLES;
#elif USE(OPENGL)
class ExtensionsGLOpenGL;
#endif
class HostWindow;
class ImageBuffer;
class ImageData;
#if USE(TEXTURE_MAPPER)
class TextureMapperGCGLPlatformLayer;
#endif

typedef WTF::HashMap<CString, uint64_t> ShaderNameHash;

class GraphicsContextGLOpenGLPrivate;

class GraphicsContextGLOpenGL final : public GraphicsContextGL
#if PLATFORM(COCOA)
    , private WebGLLayerClient
#endif
{
public:
    static RefPtr<GraphicsContextGLOpenGL> create(GraphicsContextGLAttributes, HostWindow*, Destination = Destination::Offscreen);
    virtual ~GraphicsContextGLOpenGL();

#if PLATFORM(COCOA)
    static Ref<GraphicsContextGLOpenGL> createShared(GraphicsContextGLOpenGL& sharedContext);
    PlatformGraphicsContextGL platformGraphicsContextGL() const final { return m_contextObj; }
    PlatformGLObject platformTexture() const final { return m_texture; }
    CALayer* platformLayer() const final { return reinterpret_cast<CALayer*>(m_webGLLayer.get()); }
    PlatformGraphicsContextGLDisplay platformDisplay() const { return m_displayObj; }
    PlatformGraphicsContextGLConfig platformConfig() const { return m_configObj; }
    static GCGLenum IOSurfaceTextureTarget();
    static GCGLenum IOSurfaceTextureTargetQuery();
    static GCGLint EGLIOSurfaceTextureTarget();
#else
    PlatformGraphicsContextGL platformGraphicsContextGL() const final;
    PlatformGLObject platformTexture() const final;
    PlatformLayer* platformLayer() const final;
#endif

    bool makeContextCurrent();
#if PLATFORM(IOS_FAMILY)
    enum class ReleaseBehavior {
        PreserveThreadResources,
        ReleaseThreadResources
    };
    static bool releaseCurrentContext(ReleaseBehavior);
#endif

    // With multisampling on, blit from multisampleFBO to regular FBO.
    void prepareTexture();

    // Equivalent to ::glTexImage2D(). Allows pixels==0 with no allocation.
    void texImage2DDirect(GCGLenum target, GCGLint level, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, GCGLint border, GCGLenum format, GCGLenum type, const void* pixels);

    // Get an attribute location without checking the name -> mangledname mapping.
    int getAttribLocationDirect(PlatformGLObject program, const String& name);

    // Compile a shader without going through ANGLE.
    void compileShaderDirect(PlatformGLObject);

#if !USE(ANGLE)
    bool texImage2DResourceSafe(GCGLenum target, GCGLint level, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, GCGLint border, GCGLenum format, GCGLenum type, GCGLint alignment = 4) final;
#endif

    bool isGLES2Compliant() const final;

    //----------------------------------------------------------------------
    // Entry points for WebGL.
    //

    void activeTexture(GCGLenum texture) final;
    void attachShader(PlatformGLObject program, PlatformGLObject shader) final;
    void bindAttribLocation(PlatformGLObject, GCGLuint index, const String& name) final;
    void bindBuffer(GCGLenum target, PlatformGLObject) final;
    void bindFramebuffer(GCGLenum target, PlatformGLObject) final;
    void bindRenderbuffer(GCGLenum target, PlatformGLObject) final;
    void bindTexture(GCGLenum target, PlatformGLObject) final;
    void blendColor(GCGLclampf red, GCGLclampf green, GCGLclampf blue, GCGLclampf alpha) final;
    void blendEquation(GCGLenum mode) final;
    void blendEquationSeparate(GCGLenum modeRGB, GCGLenum modeAlpha) final;
    void blendFunc(GCGLenum sfactor, GCGLenum dfactor) final;
    void blendFuncSeparate(GCGLenum srcRGB, GCGLenum dstRGB, GCGLenum srcAlpha, GCGLenum dstAlpha) final;

    void bufferData(GCGLenum target, GCGLsizeiptr size, GCGLenum usage) final;
    void bufferData(GCGLenum target, GCGLsizeiptr size, const void* data, GCGLenum usage) final;
    void bufferSubData(GCGLenum target, GCGLintptr offset, GCGLsizeiptr size, const void* data) final;

    GCGLenum checkFramebufferStatus(GCGLenum target) final;
    void clear(GCGLbitfield mask) final;
    void clearColor(GCGLclampf red, GCGLclampf green, GCGLclampf blue, GCGLclampf alpha) final;
    void clearDepth(GCGLclampf depth) final;
    void clearStencil(GCGLint s) final;
    void colorMask(GCGLboolean red, GCGLboolean green, GCGLboolean blue, GCGLboolean alpha) final;
    void compileShader(PlatformGLObject) final;

    void compressedTexImage2D(GCGLenum target, GCGLint level, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, GCGLint border, GCGLsizei imageSize, const void* data) final;
    void compressedTexSubImage2D(GCGLenum target, GCGLint level, GCGLint xoffset, GCGLint yoffset, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLsizei imageSize, const void* data) final;
    void copyTexImage2D(GCGLenum target, GCGLint level, GCGLenum internalformat, GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height, GCGLint border) final;
    void copyTexSubImage2D(GCGLenum target, GCGLint level, GCGLint xoffset, GCGLint yoffset, GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height) final;
    void cullFace(GCGLenum mode) final;
    void depthFunc(GCGLenum func) final;
    void depthMask(GCGLboolean flag) final;
    void depthRange(GCGLclampf zNear, GCGLclampf zFar) final;
    void detachShader(PlatformGLObject, PlatformGLObject) final;
    void disable(GCGLenum cap) final;
    void disableVertexAttribArray(GCGLuint index) final;
    void drawArrays(GCGLenum mode, GCGLint first, GCGLsizei count) final;
    void drawElements(GCGLenum mode, GCGLsizei count, GCGLenum type, GCGLintptr offset) final;

    void enable(GCGLenum cap) final;
    void enableVertexAttribArray(GCGLuint index) final;
    void finish() final;
    void flush() final;
    void framebufferRenderbuffer(GCGLenum target, GCGLenum attachment, GCGLenum renderbuffertarget, PlatformGLObject) final;
    void framebufferTexture2D(GCGLenum target, GCGLenum attachment, GCGLenum textarget, PlatformGLObject, GCGLint level) final;
    void frontFace(GCGLenum mode) final;
    void generateMipmap(GCGLenum target) final;

    bool getActiveAttrib(PlatformGLObject program, GCGLuint index, ActiveInfo&) final;
    bool getActiveAttribImpl(PlatformGLObject program, GCGLuint index, ActiveInfo&);
    bool getActiveUniform(PlatformGLObject program, GCGLuint index, ActiveInfo&) final;
    bool getActiveUniformImpl(PlatformGLObject program, GCGLuint index, ActiveInfo&);
    void getAttachedShaders(PlatformGLObject program, GCGLsizei maxCount, GCGLsizei* count, PlatformGLObject* shaders) final;
    GCGLint getAttribLocation(PlatformGLObject, const String& name) final;
    void getBooleanv(GCGLenum pname, GCGLboolean* value) final;
    void getBufferParameteriv(GCGLenum target, GCGLenum pname, GCGLint* value) final;
    GCGLenum getError() final;
    void getFloatv(GCGLenum pname, GCGLfloat* value) final;
    void getFramebufferAttachmentParameteriv(GCGLenum target, GCGLenum attachment, GCGLenum pname, GCGLint* value) final;
    void getIntegerv(GCGLenum pname, GCGLint* value) final;
    void getIntegeri_v(GCGLenum pname, GCGLuint index, GCGLint* value) final;
    void getInteger64v(GCGLenum pname, GCGLint64* value) final;
    void getInteger64i_v(GCGLenum pname, GCGLuint index, GCGLint64* value) final;
    void getProgramiv(PlatformGLObject program, GCGLenum pname, GCGLint* value) final;
#if !USE(ANGLE)
    void getNonBuiltInActiveSymbolCount(PlatformGLObject program, GCGLenum pname, GCGLint* value);
#endif // !USE(ANGLE)
    String getProgramInfoLog(PlatformGLObject) final;
    String getUnmangledInfoLog(PlatformGLObject[2], GCGLsizei, const String&);
    void getRenderbufferParameteriv(GCGLenum target, GCGLenum pname, GCGLint* value) final;
    void getShaderiv(PlatformGLObject, GCGLenum pname, GCGLint* value) final;
    String getShaderInfoLog(PlatformGLObject) final;
    void getShaderPrecisionFormat(GCGLenum shaderType, GCGLenum precisionType, GCGLint* range, GCGLint* precision) final;
    String getShaderSource(PlatformGLObject) final;
    String getString(GCGLenum name) final;
    void getTexParameterfv(GCGLenum target, GCGLenum pname, GCGLfloat* value) final;
    void getTexParameteriv(GCGLenum target, GCGLenum pname, GCGLint* value) final;
    void getUniformfv(PlatformGLObject program, GCGLint location, GCGLfloat* value) final;
    void getUniformiv(PlatformGLObject program, GCGLint location, GCGLint* value) final;
    void getUniformuiv(PlatformGLObject program, GCGLint location, GCGLuint* value) final;
    GCGLint getUniformLocation(PlatformGLObject, const String& name) final;
    void getVertexAttribfv(GCGLuint index, GCGLenum pname, GCGLfloat* value) final;
    void getVertexAttribiv(GCGLuint index, GCGLenum pname, GCGLint* value) final;
    GCGLsizeiptr getVertexAttribOffset(GCGLuint index, GCGLenum pname) final;

    void hint(GCGLenum target, GCGLenum mode) final;
    GCGLboolean isBuffer(PlatformGLObject) final;
    GCGLboolean isEnabled(GCGLenum cap) final;
    GCGLboolean isFramebuffer(PlatformGLObject) final;
    GCGLboolean isProgram(PlatformGLObject) final;
    GCGLboolean isRenderbuffer(PlatformGLObject) final;
    GCGLboolean isShader(PlatformGLObject) final;
    GCGLboolean isTexture(PlatformGLObject) final;
    void lineWidth(GCGLfloat) final;
    void linkProgram(PlatformGLObject) final;
    void pixelStorei(GCGLenum pname, GCGLint param) final;
    void polygonOffset(GCGLfloat factor, GCGLfloat units) final;

    void readPixels(GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLenum type, void* data) final;

    void renderbufferStorage(GCGLenum target, GCGLenum internalformat, GCGLsizei width, GCGLsizei height) final;
    void sampleCoverage(GCGLclampf value, GCGLboolean invert) final;
    void scissor(GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height) final;
    void shaderSource(PlatformGLObject, const String& string) final;
    void stencilFunc(GCGLenum func, GCGLint ref, GCGLuint mask) final;
    void stencilFuncSeparate(GCGLenum face, GCGLenum func, GCGLint ref, GCGLuint mask) final;
    void stencilMask(GCGLuint mask) final;
    void stencilMaskSeparate(GCGLenum face, GCGLuint mask) final;
    void stencilOp(GCGLenum fail, GCGLenum zfail, GCGLenum zpass) final;
    void stencilOpSeparate(GCGLenum face, GCGLenum fail, GCGLenum zfail, GCGLenum zpass) final;

    bool texImage2D(GCGLenum target, GCGLint level, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, GCGLint border, GCGLenum format, GCGLenum type, const void* pixels) final;
    void texParameterf(GCGLenum target, GCGLenum pname, GCGLfloat param) final;
    void texParameteri(GCGLenum target, GCGLenum pname, GCGLint param) final;
    void texSubImage2D(GCGLenum target, GCGLint level, GCGLint xoffset, GCGLint yoffset, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLenum type, const void* pixels) final;

    void uniform1f(GCGLint location, GCGLfloat x) final;
    void uniform1fv(GCGLint location, GCGLsizei, const GCGLfloat* v) final;
    void uniform1i(GCGLint location, GCGLint x) final;
    void uniform1iv(GCGLint location, GCGLsizei, const GCGLint* v) final;
    void uniform2f(GCGLint location, GCGLfloat x, GCGLfloat y) final;
    void uniform2fv(GCGLint location, GCGLsizei, const GCGLfloat* v) final;
    void uniform2i(GCGLint location, GCGLint x, GCGLint y) final;
    void uniform2iv(GCGLint location, GCGLsizei, const GCGLint* v) final;
    void uniform3f(GCGLint location, GCGLfloat x, GCGLfloat y, GCGLfloat z) final;
    void uniform3fv(GCGLint location, GCGLsizei, const GCGLfloat* v) final;
    void uniform3i(GCGLint location, GCGLint x, GCGLint y, GCGLint z) final;
    void uniform3iv(GCGLint location, GCGLsizei, const GCGLint* v) final;
    void uniform4f(GCGLint location, GCGLfloat x, GCGLfloat y, GCGLfloat z, GCGLfloat w) final;
    void uniform4fv(GCGLint location, GCGLsizei, const GCGLfloat* v) final;
    void uniform4i(GCGLint location, GCGLint x, GCGLint y, GCGLint z, GCGLint w) final;
    void uniform4iv(GCGLint location, GCGLsizei, const GCGLint* v) final;
    void uniformMatrix2fv(GCGLint location, GCGLsizei, GCGLboolean transpose, const GCGLfloat* value) final;
    void uniformMatrix3fv(GCGLint location, GCGLsizei, GCGLboolean transpose, const GCGLfloat* value) final;
    void uniformMatrix4fv(GCGLint location, GCGLsizei, GCGLboolean transpose, const GCGLfloat* value) final;

    void useProgram(PlatformGLObject) final;
    void validateProgram(PlatformGLObject) final;
#if !USE(ANGLE)
    bool checkVaryingsPacking(PlatformGLObject vertexShader, PlatformGLObject fragmentShader) const;
    bool precisionsMatch(PlatformGLObject vertexShader, PlatformGLObject fragmentShader) const;
#endif

    void vertexAttrib1f(GCGLuint index, GCGLfloat x) final;
    void vertexAttrib1fv(GCGLuint index, const GCGLfloat* values) final;
    void vertexAttrib2f(GCGLuint index, GCGLfloat x, GCGLfloat y) final;
    void vertexAttrib2fv(GCGLuint index, const GCGLfloat* values) final;
    void vertexAttrib3f(GCGLuint index, GCGLfloat x, GCGLfloat y, GCGLfloat z) final;
    void vertexAttrib3fv(GCGLuint index, const GCGLfloat* values) final;
    void vertexAttrib4f(GCGLuint index, GCGLfloat x, GCGLfloat y, GCGLfloat z, GCGLfloat w) final;
    void vertexAttrib4fv(GCGLuint index, const GCGLfloat* values) final;
    void vertexAttribPointer(GCGLuint index, GCGLint size, GCGLenum type, GCGLboolean normalized, GCGLsizei stride, GCGLintptr offset) final;

    void viewport(GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height) final;

    void reshape(int width, int height);

    void drawArraysInstanced(GCGLenum mode, GCGLint first, GCGLsizei count, GCGLsizei primcount) final;
    void drawElementsInstanced(GCGLenum mode, GCGLsizei count, GCGLenum type, GCGLintptr offset, GCGLsizei primcount) final;
    void vertexAttribDivisor(GCGLuint index, GCGLuint divisor) final;

    // VertexArrayOject calls
    PlatformGLObject createVertexArray() final;
    void deleteVertexArray(PlatformGLObject) final;
    GCGLboolean isVertexArray(PlatformGLObject) final;
    void bindVertexArray(PlatformGLObject) final;

    // ========== WebGL2 entry points.

    void bufferData(GCGLenum target, const void* data, GCGLenum usage, GCGLuint srcOffset, GCGLuint length) final;
    void bufferSubData(GCGLenum target, GCGLintptr dstByteOffset, const void* srcData, GCGLuint srcOffset, GCGLuint length) final;

    void copyBufferSubData(GCGLenum readTarget, GCGLenum writeTarget, GCGLintptr readOffset, GCGLintptr writeOffset, GCGLsizeiptr size) final;
    void getBufferSubData(GCGLenum target, GCGLintptr srcByteOffset, const void* dstData, GCGLuint dstOffset, GCGLuint length) final;
    void* mapBufferRange(GCGLenum target, GCGLintptr offset, GCGLsizeiptr length, GCGLbitfield access) final;
    GCGLboolean unmapBuffer(GCGLenum target) final;

    void blitFramebuffer(GCGLint srcX0, GCGLint srcY0, GCGLint srcX1, GCGLint srcY1, GCGLint dstX0, GCGLint dstY0, GCGLint dstX1, GCGLint dstY1, GCGLbitfield mask, GCGLenum filter) final;
    void framebufferTextureLayer(GCGLenum target, GCGLenum attachment, PlatformGLObject texture, GCGLint level, GCGLint layer) final;
    void invalidateFramebuffer(GCGLenum target, GCGLsizei numAttachments, const GCGLenum* attachments) final;
    void invalidateSubFramebuffer(GCGLenum target, GCGLsizei numAttachments, const GCGLenum* attachments, GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height) final;
    void readBuffer(GCGLenum src) final;

    // getInternalFormatParameter
    void getInternalformativ(GCGLenum target, GCGLenum internalformat, GCGLenum pname, GCGLsizei bufSize, GCGLint* params) final;
    void renderbufferStorageMultisample(GCGLenum target, GCGLsizei samples, GCGLenum internalformat, GCGLsizei width, GCGLsizei height) final;

    void texStorage2D(GCGLenum target, GCGLsizei levels, GCGLenum internalformat, GCGLsizei width, GCGLsizei height) final;
    void texStorage3D(GCGLenum target, GCGLsizei levels, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, GCGLsizei depth) final;

    void copyTexSubImage3D(GCGLenum target, GCGLint level, GCGLint xoffset, GCGLint yoffset, GCGLint zoffset, GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height) final;

    GCGLint getFragDataLocation(PlatformGLObject program, const String& name) final;

    void uniform1ui(GCGLint location, GCGLuint v0) final;
    void uniform2ui(GCGLint location, GCGLuint v0, GCGLuint v1) final;
    void uniform3ui(GCGLint location, GCGLuint v0, GCGLuint v1, GCGLuint v2) final;
    void uniform4ui(GCGLint location, GCGLuint v0, GCGLuint v1, GCGLuint v2, GCGLuint v3) final;
    void uniform1uiv(GCGLint location, const GCGLuint* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform2uiv(GCGLint location, const GCGLuint* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform3uiv(GCGLint location, const GCGLuint* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform4uiv(GCGLint location, const GCGLuint* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniformMatrix2x3fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniformMatrix3x2fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniformMatrix2x4fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniformMatrix4x2fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniformMatrix3x4fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniformMatrix4x3fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void vertexAttribI4i(GCGLuint index, GCGLint x, GCGLint y, GCGLint z, GCGLint w) final;
    void vertexAttribI4iv(GCGLuint index, const GCGLint* values) final;
    void vertexAttribI4ui(GCGLuint index, GCGLuint x, GCGLuint y, GCGLuint z, GCGLuint w) final;
    void vertexAttribI4uiv(GCGLuint index, const GCGLuint* values) final;
    void vertexAttribIPointer(GCGLuint index, GCGLint size, GCGLenum type, GCGLsizei stride, GCGLintptr offset) final;

    void drawRangeElements(GCGLenum mode, GCGLuint start, GCGLuint end, GCGLsizei count, GCGLenum type, GCGLintptr offset) final;

    void drawBuffers(GCGLsizei n, const GCGLenum* bufs) final;
    void clearBufferiv(GCGLenum buffer, GCGLint drawbuffer, const GCGLint* values, GCGLuint srcOffset) final;
    void clearBufferuiv(GCGLenum buffer, GCGLint drawbuffer, const GCGLuint* values, GCGLuint srcOffset) final;
    void clearBufferfv(GCGLenum buffer, GCGLint drawbuffer, const GCGLfloat* values, GCGLuint srcOffset) final;
    void clearBufferfi(GCGLenum buffer, GCGLint drawbuffer, GCGLfloat depth, GCGLint stencil) final;

    PlatformGLObject createQuery() final;
    void deleteQuery(PlatformGLObject query) final;
    GCGLboolean isQuery(PlatformGLObject query) final;
    void beginQuery(GCGLenum target, PlatformGLObject query) final;
    void endQuery(GCGLenum target) final;
    PlatformGLObject getQuery(GCGLenum target, GCGLenum pname) final;
    // getQueryParameter
    void getQueryObjectuiv(PlatformGLObject query, GCGLenum pname, GCGLuint* value) final;

    PlatformGLObject createSampler() final;
    void deleteSampler(PlatformGLObject sampler) final;
    GCGLboolean isSampler(PlatformGLObject sampler) final;
    void bindSampler(GCGLuint unit, PlatformGLObject sampler) final;
    void samplerParameteri(PlatformGLObject sampler, GCGLenum pname, GCGLint param) final;
    void samplerParameterf(PlatformGLObject sampler, GCGLenum pname, GCGLfloat param) final;
    // getSamplerParameter
    void getSamplerParameterfv(PlatformGLObject sampler, GCGLenum pname, GCGLfloat* value) final;
    void getSamplerParameteriv(PlatformGLObject sampler, GCGLenum pname, GCGLint* value) final;

    GCGLsync fenceSync(GCGLenum condition, GCGLbitfield flags) final;
    GCGLboolean isSync(GCGLsync) final;
    void deleteSync(GCGLsync) final;
    GCGLenum clientWaitSync(GCGLsync, GCGLbitfield flags, GCGLuint64 timeout) final;
    void waitSync(GCGLsync, GCGLbitfield flags, GCGLint64 timeout) final;
    // getSyncParameter
    // FIXME - this can be implemented at the WebGL level if we signal the WebGLSync object.
    void getSynciv(GCGLsync, GCGLenum pname, GCGLsizei bufSize, GCGLint *value) final;

    PlatformGLObject createTransformFeedback() final;
    void deleteTransformFeedback(PlatformGLObject id) final;
    GCGLboolean isTransformFeedback(PlatformGLObject id) final;
    void bindTransformFeedback(GCGLenum target, PlatformGLObject id) final;
    void beginTransformFeedback(GCGLenum primitiveMode) final;
    void endTransformFeedback() final;
    void transformFeedbackVaryings(PlatformGLObject program, const Vector<String>& varyings, GCGLenum bufferMode) final;
    void getTransformFeedbackVarying(PlatformGLObject program, GCGLuint index, ActiveInfo&) final;
    void pauseTransformFeedback() final;
    void resumeTransformFeedback() final;

    void bindBufferBase(GCGLenum target, GCGLuint index, PlatformGLObject buffer) final;
    void bindBufferRange(GCGLenum target, GCGLuint index, PlatformGLObject buffer, GCGLintptr offset, GCGLsizeiptr size) final;
    // getIndexedParameter -> use getParameter calls above.
    Vector<GCGLuint> getUniformIndices(PlatformGLObject program, const Vector<String>& uniformNames) final;
    void getActiveUniforms(PlatformGLObject program, const Vector<GCGLuint>& uniformIndices, GCGLenum pname, Vector<GCGLint>& params) final;

    GCGLuint getUniformBlockIndex(PlatformGLObject program, const String& uniformBlockName) final;
    // getActiveUniformBlockParameter
    void getActiveUniformBlockiv(PlatformGLObject program, GCGLuint uniformBlockIndex, GCGLenum pname, GCGLint* params) final;
    String getActiveUniformBlockName(PlatformGLObject program, GCGLuint uniformBlockIndex) final;
    void uniformBlockBinding(PlatformGLObject program, GCGLuint uniformBlockIndex, GCGLuint uniformBlockBinding) final;

    void compressedTexImage2D(GCGLenum target, GCGLint level, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, GCGLint border, GCGLsizei imageSize, GCGLintptr offset) final;
    void compressedTexImage2D(GCGLenum target, GCGLint level, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, GCGLint border, const void* srcData, GCGLuint srcOffset, GCGLuint srcLengthOverride) final;

    void compressedTexSubImage2D(GCGLenum target, GCGLint level, GCGLint xoffset, GCGLint yoffset, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLsizei imageSize, GCGLintptr offset) final;
    void compressedTexSubImage2D(GCGLenum target, GCGLint level, GCGLint xoffset, GCGLint yoffset, GCGLsizei width, GCGLsizei height, GCGLenum format, const void* srcData, GCGLuint srcOffset, GCGLuint srcLengthOverride) final;

    void uniform1fv(GCGLint location, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform2fv(GCGLint location, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform3fv(GCGLint location, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform4fv(GCGLint location, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;

    void uniform1iv(GCGLint location, const GCGLint* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform2iv(GCGLint location, const GCGLint* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform3iv(GCGLint location, const GCGLint* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniform4iv(GCGLint location, const GCGLint* data, GCGLuint srcOffset, GCGLuint srcLength) final;

    void uniformMatrix2fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniformMatrix3fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;
    void uniformMatrix4fv(GCGLint location, GCGLboolean transpose, const GCGLfloat* data, GCGLuint srcOffset, GCGLuint srcLength) final;

    void readPixels(GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLenum type, GCGLintptr offset) final;
    void readPixels(GCGLint x, GCGLint y, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLenum type, const void* dstData, GCGLuint dstOffset) final;

    // Helper methods.

    void paintToCanvas(const unsigned char* imagePixels, const IntSize& imageSize, const IntSize& canvasSize, GraphicsContext&);

    void markContextChanged();
    void markLayerComposited();
    bool layerComposited() const;
    void forceContextLost();
    void recycleContext();

    // Maintenance of auto-clearing of color/depth/stencil buffers. The
    // reset method is present to keep calling code simpler, so it
    // doesn't have to know which buffers were allocated.
    void resetBuffersToAutoClear();
    void setBuffersToAutoClear(GCGLbitfield);
    GCGLbitfield getBuffersToAutoClear() const;
    void enablePreserveDrawingBuffer() final;

    void dispatchContextChangedNotification();
    void simulateContextChanged();

    void paintRenderingResultsToCanvas(ImageBuffer*);
    RefPtr<ImageData> paintRenderingResultsToImageData();
    bool paintCompositedResultsToCanvas(ImageBuffer*);

#if USE(OPENGL) && ENABLE(WEBGL2)
    void primitiveRestartIndex(GCGLuint) final;
#endif

#if PLATFORM(COCOA) && PLATFORM(MAC)
    void updateCGLContext();
#endif

    void setContextVisibility(bool) final;

    GraphicsContextGLPowerPreference powerPreferenceUsedForCreation() const final { return m_powerPreferenceUsedForCreation; }

    // Support for buffer creation and deletion
    PlatformGLObject createBuffer() final;
    PlatformGLObject createFramebuffer() final;
    PlatformGLObject createProgram() final;
    PlatformGLObject createRenderbuffer() final;
    PlatformGLObject createShader(GCGLenum) final;
    PlatformGLObject createTexture() final;

    void deleteBuffer(PlatformGLObject) final;
    void deleteFramebuffer(PlatformGLObject) final;
    void deleteProgram(PlatformGLObject) final;
    void deleteRenderbuffer(PlatformGLObject) final;
    void deleteShader(PlatformGLObject) final;
    void deleteTexture(PlatformGLObject) final;

    void synthesizeGLError(GCGLenum error) final;

    // Read real OpenGL errors, and move them to the synthetic
    // error list. Return true if at least one error is moved.
    bool moveErrorsToSyntheticErrorList();

    // Support for extensions. Returns a non-null object, though not
    // all methods it contains may necessarily be supported on the
    // current hardware. Must call ExtensionsGL::supports() to
    // determine this.
    ExtensionsGL& getExtensions() final;

    void setFailNextGPUStatusCheck() final { m_failNextStatusCheck = true; }

    unsigned textureSeed(GCGLuint texture) { return m_state.textureSeedCount.count(texture); }

#if PLATFORM(MAC)
    using PlatformDisplayID = uint32_t;
    void screenDidChange(PlatformDisplayID);
#endif

    void prepareForDisplay() final;

private:
    GraphicsContextGLOpenGL(GraphicsContextGLAttributes, HostWindow*, Destination = Destination::Offscreen, GraphicsContextGLOpenGL* sharedContext = nullptr);


    // Take into account the user's requested context creation attributes,
    // in particular stencil and antialias, and determine which could or
    // could not be honored based on the capabilities of the OpenGL
    // implementation.
    void validateDepthStencil(const char* packedDepthStencilExtension);
    void validateAttributes();
    
    // Did the most recent drawing operation leave the GPU in an acceptable state?
    void checkGPUStatus();

    // Read rendering results into a pixel array with the same format as the
    // backbuffer.
    void readRenderingResults(unsigned char* pixels, int pixelsSize);
    void readPixelsAndConvertToBGRAIfNecessary(int x, int y, int width, int height, unsigned char* pixels);

#if USE(OPENGL_ES)
    void setRenderbufferStorageFromDrawable(GCGLsizei width, GCGLsizei height);
#endif

    bool reshapeFBOs(const IntSize&);
    void prepareTextureImpl();
    void resolveMultisamplingIfNecessary(const IntRect& = IntRect());
    void attachDepthAndStencilBufferIfNeeded(GCGLuint internalDepthStencilFormat, int width, int height);

#if PLATFORM(COCOA)
    bool allowOfflineRenderers() const;
    bool reshapeDisplayBufferBacking();
    bool bindDisplayBufferBacking(std::unique_ptr<IOSurface> backing, void* pbuffer);
    // WebGLLayerClient overrides.
    void didDisplay() final;
#endif

#if PLATFORM(COCOA)
    RetainPtr<WebGLLayer> m_webGLLayer;
    PlatformGraphicsContextGL m_contextObj { nullptr };
    PlatformGraphicsContextGLDisplay m_displayObj { nullptr };
    PlatformGraphicsContextGLConfig m_configObj { nullptr };
#endif // PLATFORM(COCOA)

#if PLATFORM(WIN) && USE(CA)
    RefPtr<PlatformCALayer> m_webGLLayer;
#endif

#if !USE(ANGLE)
    typedef HashMap<String, sh::ShaderVariable> ShaderSymbolMap;

    struct ShaderSourceEntry {
        GCGLenum type;
        String source;
        String translatedSource;
        String log;
        bool isValid;
        ShaderSymbolMap attributeMap;
        ShaderSymbolMap uniformMap;
        ShaderSymbolMap varyingMap;
        ShaderSourceEntry()
            : type(VERTEX_SHADER)
            , isValid(false)
        {
        }
        
        ShaderSymbolMap& symbolMap(enum ANGLEShaderSymbolType symbolType)
        {
            ASSERT(symbolType == SHADER_SYMBOL_TYPE_ATTRIBUTE || symbolType == SHADER_SYMBOL_TYPE_UNIFORM || symbolType == SHADER_SYMBOL_TYPE_VARYING);
            if (symbolType == SHADER_SYMBOL_TYPE_ATTRIBUTE)
                return attributeMap;
            if (symbolType == SHADER_SYMBOL_TYPE_VARYING)
                return varyingMap;
            return uniformMap;
        }
    };

    // FIXME: Shaders are never removed from this map, even if they and their program are deleted.
    // This is bad, and it also relies on the fact we never reuse PlatformGLObject numbers.
    typedef HashMap<PlatformGLObject, ShaderSourceEntry> ShaderSourceMap;
    ShaderSourceMap m_shaderSourceMap;

    typedef HashMap<PlatformGLObject, std::pair<PlatformGLObject, PlatformGLObject>> LinkedShaderMap;
    LinkedShaderMap m_linkedShaderMap;

    struct ActiveShaderSymbolCounts {
        Vector<GCGLint> filteredToActualAttributeIndexMap;
        Vector<GCGLint> filteredToActualUniformIndexMap;

        ActiveShaderSymbolCounts()
        {
        }

        GCGLint countForType(GCGLenum activeType)
        {
            ASSERT(activeType == ACTIVE_ATTRIBUTES || activeType == ACTIVE_UNIFORMS);
            if (activeType == ACTIVE_ATTRIBUTES)
                return filteredToActualAttributeIndexMap.size();

            return filteredToActualUniformIndexMap.size();
        }
    };
    typedef HashMap<PlatformGLObject, ActiveShaderSymbolCounts> ShaderProgramSymbolCountMap;
    ShaderProgramSymbolCountMap m_shaderProgramSymbolCountMap;

    typedef HashMap<String, String> HashedSymbolMap;
    HashedSymbolMap m_possiblyUnusedAttributeMap;

    String mappedSymbolName(PlatformGLObject program, ANGLEShaderSymbolType, const String& name);
    String mappedSymbolName(PlatformGLObject shaders[2], size_t count, const String& name);
    String originalSymbolName(PlatformGLObject program, ANGLEShaderSymbolType, const String& name);
    Optional<String> mappedSymbolInShaderSourceMap(PlatformGLObject shader, ANGLEShaderSymbolType, const String& name);
    Optional<String> originalSymbolInShaderSourceMap(PlatformGLObject shader, ANGLEShaderSymbolType, const String& name);

    std::unique_ptr<ShaderNameHash> nameHashMapForShaders;
#endif // !USE(ANGLE)

#if USE(ANGLE)
    friend class ExtensionsGLANGLE;
    std::unique_ptr<ExtensionsGLANGLE> m_extensions;
#elif USE(OPENGL_ES)
    friend class ExtensionsGLOpenGLES;
    friend class ExtensionsGLOpenGLCommon;
    std::unique_ptr<ExtensionsGLOpenGLES> m_extensions;
#elif USE(OPENGL)
    friend class ExtensionsGLOpenGL;
    friend class ExtensionsGLOpenGLCommon;
    std::unique_ptr<ExtensionsGLOpenGL> m_extensions;
#endif

    GraphicsContextGLPowerPreference m_powerPreferenceUsedForCreation { GraphicsContextGLPowerPreference::Default };
    Vector<Vector<float>> m_vertexArray;

#if !USE(ANGLE)
    ANGLEWebKitBridge m_compiler;
#endif

    GCGLuint m_texture { 0 };
    GCGLuint m_fbo { 0 };
#if USE(COORDINATED_GRAPHICS)
    GCGLuint m_compositorTexture { 0 };
    GCGLuint m_intermediateTexture { 0 };
#endif

    GCGLuint m_depthBuffer { 0 };
    GCGLuint m_stencilBuffer { 0 };
    GCGLuint m_depthStencilBuffer { 0 };

    bool m_layerComposited { false };
    GCGLuint m_internalColorFormat { 0 };

    struct GraphicsContextGLState {
        GCGLuint boundReadFBO { 0 };
        GCGLuint boundDrawFBO { 0 };
        GCGLenum activeTextureUnit { GraphicsContextGL::TEXTURE0 };

        using BoundTextureMap = HashMap<GCGLenum,
            std::pair<GCGLuint, GCGLenum>,
            WTF::IntHash<GCGLenum>, 
            WTF::UnsignedWithZeroKeyHashTraits<GCGLuint>,
            WTF::PairHashTraits<WTF::UnsignedWithZeroKeyHashTraits<GCGLuint>, WTF::UnsignedWithZeroKeyHashTraits<GCGLuint>>
        >;
        BoundTextureMap boundTextureMap;
        GCGLuint currentBoundTexture() const { return boundTexture(activeTextureUnit); }
        GCGLuint boundTexture(GCGLenum textureUnit) const
        {
            auto iterator = boundTextureMap.find(textureUnit);
            if (iterator != boundTextureMap.end())
                return iterator->value.first;
            return 0;
        }

        GCGLuint currentBoundTarget() const { return boundTarget(activeTextureUnit); }
        GCGLenum boundTarget(GCGLenum textureUnit) const
        {
            auto iterator = boundTextureMap.find(textureUnit);
            if (iterator != boundTextureMap.end())
                return iterator->value.second;
            return 0;
        }

        void setBoundTexture(GCGLenum textureUnit, GCGLuint texture, GCGLenum target)
        {
            boundTextureMap.set(textureUnit, std::make_pair(texture, target));
        }

        using TextureSeedCount = HashCountedSet<GCGLuint, WTF::IntHash<GCGLuint>, WTF::UnsignedWithZeroKeyHashTraits<GCGLuint>>;
        TextureSeedCount textureSeedCount;
    };

    GraphicsContextGLState m_state;

    // For multisampling
    GCGLuint m_multisampleFBO { 0 };
    GCGLuint m_multisampleDepthStencilBuffer { 0 };
    GCGLuint m_multisampleColorBuffer { 0 };

#if USE(ANGLE)
    // For preserveDrawingBuffer:true without multisampling.
    GCGLuint m_preserveDrawingBufferTexture { 0 };
    // Attaches m_texture when m_preserveDrawingBufferTexture is non-zero.
    GCGLuint m_preserveDrawingBufferFBO { 0 };
#endif

    // A bitmask of GL buffer bits (GL_COLOR_BUFFER_BIT,
    // GL_DEPTH_BUFFER_BIT, GL_STENCIL_BUFFER_BIT) which need to be
    // auto-cleared.
    GCGLbitfield m_buffersToAutoClear { 0 };

    // Errors raised by synthesizeGLError().
    ListHashSet<GCGLenum> m_syntheticErrors;

#if USE(NICOSIA) && USE(TEXTURE_MAPPER)
    friend class Nicosia::GCGLLayer;
    std::unique_ptr<Nicosia::GCGLLayer> m_nicosiaLayer;
#elif USE(TEXTURE_MAPPER)
    friend class TextureMapperGCGLPlatformLayer;
    std::unique_ptr<TextureMapperGCGLPlatformLayer> m_texmapLayer;
#elif !PLATFORM(COCOA)
    friend class GraphicsContextGLOpenGLPrivate;
    std::unique_ptr<GraphicsContextGLOpenGLPrivate> m_private;
#endif

    bool m_isForWebGL2 { false };
    bool m_usingCoreProfile { false };

    unsigned m_statusCheckCount { 0 };
    bool m_failNextStatusCheck { false };

#if USE(CAIRO)
    PlatformGLObject m_vao { 0 };
#endif

#if PLATFORM(COCOA)
    // Backing store for the the buffer which is eventually used for display.
    // When preserveDrawingBuffer == false, this is the drawing buffer backing store.
    // When preserveDrawingBuffer == true, this is blitted to during display prepare.
    std::unique_ptr<IOSurface> m_displayBufferBacking;
    void* m_displayBufferPbuffer { nullptr };

    bool m_hasSwitchedToHighPerformanceGPU { false };
#endif
};

} // namespace WebCore

#endif // ENABLE(GRAPHICS_CONTEXT_GL)
