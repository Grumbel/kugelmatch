// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "gpu_raytracer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

// Minimal GLES2 / GL type aliases (avoid requiring GL headers at compile time).
using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLboolean = unsigned char;
using GLfloat = float;
using GLchar = char;
using GLsizeiptr = long;

#ifndef GL_FALSE
#define GL_FALSE 0
#endif
#ifndef GL_TRUE
#define GL_TRUE 1
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_PACK_ALIGNMENT
#define GL_PACK_ALIGNMENT 0x0D05
#endif

namespace {

using PFNGLCREATESHADERPROC = GLuint (*)(GLenum);
using PFNGLSHADERSOURCEPROC = void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
using PFNGLCOMPILESHADERPROC = void (*)(GLuint);
using PFNGLGETSHADERIVPROC = void (*)(GLuint, GLenum, GLint*);
using PFNGLGETSHADERINFOLOGPROC = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFNGLCREATEPROGRAMPROC = GLuint (*)();
using PFNGLATTACHSHADERPROC = void (*)(GLuint, GLuint);
using PFNGLLINKPROGRAMPROC = void (*)(GLuint);
using PFNGLGETPROGRAMIVPROC = void (*)(GLuint, GLenum, GLint*);
using PFNGLGETPROGRAMINFOLOGPROC = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFNGLDELETESHADERPROC = void (*)(GLuint);
using PFNGLDELETEPROGRAMPROC = void (*)(GLuint);
using PFNGLUSEPROGRAMPROC = void (*)(GLuint);
using PFNGLGETUNIFORMLOCATIONPROC = GLint (*)(GLuint, const GLchar*);
using PFNGLGETATTRIBLOCATIONPROC = GLint (*)(GLuint, const GLchar*);
using PFNGLUNIFORM1IPROC = void (*)(GLint, GLint);
using PFNGLUNIFORM1FPROC = void (*)(GLint, GLfloat);
using PFNGLUNIFORM2FPROC = void (*)(GLint, GLfloat, GLfloat);
using PFNGLFINISHPROC = void (*)();
using PFNGLUNIFORM3FPROC = void (*)(GLint, GLfloat, GLfloat, GLfloat);
using PFNGLUNIFORM1IVPROC = void (*)(GLint, GLsizei, const GLint*);
using PFNGLUNIFORM1FVPROC = void (*)(GLint, GLsizei, const GLfloat*);
using PFNGLUNIFORM3FVPROC = void (*)(GLint, GLsizei, const GLfloat*);
using PFNGLGENBUFFERSPROC = void (*)(GLsizei, GLuint*);
using PFNGLBINDBUFFERPROC = void (*)(GLenum, GLuint);
using PFNGLBUFFERDATAPROC = void (*)(GLenum, GLsizeiptr, const void*, GLenum);
using PFNGLDELETEBUFFERSPROC = void (*)(GLsizei, const GLuint*);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void (*)(GLuint);
using PFNGLDISABLEVERTEXATTRIBARRAYPROC = void (*)(GLuint);
using PFNGLVERTEXATTRIBPOINTERPROC = void (*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using PFNGLDRAWARRAYSPROC = void (*)(GLenum, GLint, GLsizei);
using PFNGLVIEWPORTPROC = void (*)(GLint, GLint, GLsizei, GLsizei);
using PFNGLCLEARPROC = void (*)(GLenum);
using PFNGLCLEARCOLORPROC = void (*)(GLfloat, GLfloat, GLfloat, GLfloat);
using PFNGLGENFRAMEBUFFERSPROC = void (*)(GLsizei, GLuint*);
using PFNGLBINDFRAMEBUFFERPROC = void (*)(GLenum, GLuint);
using PFNGLDELETEFRAMEBUFFERSPROC = void (*)(GLsizei, const GLuint*);
using PFNGLFRAMEBUFFERTEXTURE2DPROC = void (*)(GLenum, GLenum, GLenum, GLuint, GLint);
using PFNGLCHECKFRAMEBUFFERSTATUSPROC = GLenum (*)(GLenum);
using PFNGLGENTEXTURESPROC = void (*)(GLsizei, GLuint*);
using PFNGLBINDTEXTUREPROC = void (*)(GLenum, GLuint);
using PFNGLDELETETEXTURESPROC = void (*)(GLsizei, const GLuint*);
using PFNGLTEXIMAGE2DPROC = void (*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
using PFNGLTEXPARAMETERIPROC = void (*)(GLenum, GLenum, GLint);
using PFNGLACTIVETEXTUREPROC = void (*)(GLenum);
using PFNGLREADPIXELSPROC = void (*)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);
using PFNGLPIXELSTOREIPROC = void (*)(GLenum, GLint);

PFNGLCREATESHADERPROC glCreateShader_ = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource_ = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader_ = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv_ = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_ = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram_ = nullptr;
PFNGLATTACHSHADERPROC glAttachShader_ = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram_ = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv_ = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_ = nullptr;
PFNGLDELETESHADERPROC glDeleteShader_ = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram_ = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram_ = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_ = nullptr;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation_ = nullptr;
PFNGLUNIFORM1IPROC glUniform1i_ = nullptr;
PFNGLUNIFORM1FPROC glUniform1f_ = nullptr;
PFNGLUNIFORM2FPROC glUniform2f_ = nullptr;
PFNGLFINISHPROC glFinish_ = nullptr;
PFNGLUNIFORM3FPROC glUniform3f_ = nullptr;
PFNGLUNIFORM1IVPROC glUniform1iv_ = nullptr;
PFNGLUNIFORM1FVPROC glUniform1fv_ = nullptr;
PFNGLUNIFORM3FVPROC glUniform3fv_ = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers_ = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer_ = nullptr;
PFNGLBUFFERDATAPROC glBufferData_ = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers_ = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray_ = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray_ = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer_ = nullptr;
PFNGLDRAWARRAYSPROC glDrawArrays_ = nullptr;
PFNGLVIEWPORTPROC glViewport_ = nullptr;
PFNGLCLEARPROC glClear_ = nullptr;
PFNGLCLEARCOLORPROC glClearColor_ = nullptr;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers_ = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer_ = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers_ = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D_ = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus_ = nullptr;
PFNGLGENTEXTURESPROC glGenTextures_ = nullptr;
PFNGLBINDTEXTUREPROC glBindTexture_ = nullptr;
PFNGLDELETETEXTURESPROC glDeleteTextures_ = nullptr;
PFNGLTEXIMAGE2DPROC glTexImage2D_ = nullptr;
PFNGLTEXPARAMETERIPROC glTexParameteri_ = nullptr;
PFNGLACTIVETEXTUREPROC glActiveTexture_ = nullptr;
PFNGLREADPIXELSPROC glReadPixels_ = nullptr;
PFNGLPIXELSTOREIPROC glPixelStorei_ = nullptr;

template <typename T>
bool loadProc(T& fn, const char* name) {
    fn = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!fn) {
        std::fprintf(stderr, "Missing GL proc: %s\n", name);
        return false;
    }
    return true;
}

bool loadAllProcs() {
    bool ok = true;
    ok &= loadProc(glCreateShader_, "glCreateShader");
    ok &= loadProc(glShaderSource_, "glShaderSource");
    ok &= loadProc(glCompileShader_, "glCompileShader");
    ok &= loadProc(glGetShaderiv_, "glGetShaderiv");
    ok &= loadProc(glGetShaderInfoLog_, "glGetShaderInfoLog");
    ok &= loadProc(glCreateProgram_, "glCreateProgram");
    ok &= loadProc(glAttachShader_, "glAttachShader");
    ok &= loadProc(glLinkProgram_, "glLinkProgram");
    ok &= loadProc(glGetProgramiv_, "glGetProgramiv");
    ok &= loadProc(glGetProgramInfoLog_, "glGetProgramInfoLog");
    ok &= loadProc(glDeleteShader_, "glDeleteShader");
    ok &= loadProc(glDeleteProgram_, "glDeleteProgram");
    ok &= loadProc(glUseProgram_, "glUseProgram");
    ok &= loadProc(glGetUniformLocation_, "glGetUniformLocation");
    ok &= loadProc(glGetAttribLocation_, "glGetAttribLocation");
    ok &= loadProc(glUniform1i_, "glUniform1i");
    ok &= loadProc(glUniform1f_, "glUniform1f");
    ok &= loadProc(glUniform2f_, "glUniform2f");
    loadProc(glFinish_, "glFinish"); // optional, used for auto-scale timing
    ok &= loadProc(glUniform3f_, "glUniform3f");
    ok &= loadProc(glUniform1iv_, "glUniform1iv");
    ok &= loadProc(glUniform1fv_, "glUniform1fv");
    ok &= loadProc(glUniform3fv_, "glUniform3fv");
    ok &= loadProc(glGenBuffers_, "glGenBuffers");
    ok &= loadProc(glBindBuffer_, "glBindBuffer");
    ok &= loadProc(glBufferData_, "glBufferData");
    ok &= loadProc(glDeleteBuffers_, "glDeleteBuffers");
    ok &= loadProc(glEnableVertexAttribArray_, "glEnableVertexAttribArray");
    ok &= loadProc(glDisableVertexAttribArray_, "glDisableVertexAttribArray");
    ok &= loadProc(glVertexAttribPointer_, "glVertexAttribPointer");
    ok &= loadProc(glDrawArrays_, "glDrawArrays");
    ok &= loadProc(glViewport_, "glViewport");
    ok &= loadProc(glClear_, "glClear");
    ok &= loadProc(glClearColor_, "glClearColor");
    ok &= loadProc(glGenFramebuffers_, "glGenFramebuffers");
    ok &= loadProc(glBindFramebuffer_, "glBindFramebuffer");
    ok &= loadProc(glDeleteFramebuffers_, "glDeleteFramebuffers");
    ok &= loadProc(glFramebufferTexture2D_, "glFramebufferTexture2D");
    ok &= loadProc(glCheckFramebufferStatus_, "glCheckFramebufferStatus");
    ok &= loadProc(glGenTextures_, "glGenTextures");
    ok &= loadProc(glBindTexture_, "glBindTexture");
    ok &= loadProc(glDeleteTextures_, "glDeleteTextures");
    ok &= loadProc(glTexImage2D_, "glTexImage2D");
    ok &= loadProc(glTexParameteri_, "glTexParameteri");
    ok &= loadProc(glActiveTexture_, "glActiveTexture");
    ok &= loadProc(glReadPixels_, "glReadPixels");
    ok &= loadProc(glPixelStorei_, "glPixelStorei");
    return ok;
}

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Embedded fallbacks (CMake regenerates frag; vert/blit are small and inlined here too).
const char* kEmbeddedVert = R"GLSL(
#version 100
attribute vec2 a_pos;
varying vec2 v_uv;
void main() {
    v_uv = a_pos * 0.5 + 0.5;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)GLSL";

const char* kEmbeddedVertGL = R"GLSL(
#version 330 core
layout(location = 0) in vec2 a_pos;
out vec2 v_uv;
void main() {
    v_uv = a_pos * 0.5 + 0.5;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)GLSL";

const char* kEmbeddedBlitVert = R"GLSL(
#version 100
attribute vec2 a_pos;
uniform vec2 u_uvScale;
varying vec2 v_uv;
void main() {
    vec2 base = a_pos * 0.5 + 0.5;
    v_uv = base * u_uvScale;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)GLSL";

const char* kEmbeddedBlitFrag = R"GLSL(
#version 100
precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_tex;
void main() {
    gl_FragColor = texture2D(u_tex, v_uv);
}
)GLSL";


const char* kEmbeddedBlitVertGL = R"GLSL(
#version 330 core
layout(location = 0) in vec2 a_pos;
uniform vec2 u_uvScale;
out vec2 v_uv;
void main() {
    vec2 base = a_pos * 0.5 + 0.5;
    v_uv = base * u_uvScale;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)GLSL";

const char* kEmbeddedBlitFragGL = R"GLSL(
#version 330 core
in vec2 v_uv;
uniform sampler2D u_tex;
out vec4 fragColor;
void main() {
    fragColor = texture(u_tex, v_uv);
}
)GLSL";

#include "embedded_frag.inc"

// Cover NDC with a single oversized triangle: (-1,-1), (3,-1), (-1,3).
const float kFullscreenTri[6] = {
    -1.f, -1.f,
     3.f, -1.f,
    -1.f,  3.f,
};

} // namespace


void GpuRaytracer::shutdown() {
    if (glctx_) {
        SDL_GL_MakeCurrent(window_, glctx_);
        destroyFbo();
        if (vbo_) {
            glDeleteBuffers_(1, &vbo_);
            vbo_ = 0;
        }
        if (program_) {
            glDeleteProgram_(program_);
            program_ = 0;
        }
        if (blitProgram_) {
            glDeleteProgram_(blitProgram_);
            blitProgram_ = 0;
        }
        SDL_GL_DeleteContext(glctx_);
        glctx_ = nullptr;
    }
    ready_ = false;
    window_ = nullptr;
    uniformCache_.clear();
}

GpuRaytracer::~GpuRaytracer() {
    shutdown();
}

unsigned GpuRaytracer::compileShader(unsigned type, const char* source) {
    GLuint s = glCreateShader_(type);
    glShaderSource_(s, 1, &source, nullptr);
    glCompileShader_(s);
    GLint ok = 0;
    glGetShaderiv_(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog_(s, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader compile error:\n%s\n", log);
        glDeleteShader_(s);
        return 0;
    }
    return s;
}

bool GpuRaytracer::linkProgram(unsigned& outProg, unsigned vs, unsigned fs, const char* label) {
    outProg = glCreateProgram_();
    glAttachShader_(outProg, vs);
    glAttachShader_(outProg, fs);
    glLinkProgram_(outProg);
    GLint ok = 0;
    glGetProgramiv_(outProg, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog_(outProg, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Program link error (%s):\n%s\n", label, log);
        glDeleteProgram_(outProg);
        outProg = 0;
        return false;
    }
    return true;
}

bool GpuRaytracer::loadShaders() {
#if defined(KUGELMATCH_USE_OPENGLES2) || defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    std::string vertSrc = readFile("shaders/raytrace_es.vert");
    std::string fragSrc = readFile("shaders/raytrace_es.frag");
    std::string blitVertSrc = readFile("shaders/blit_es.vert");
    std::string blitFragSrc = readFile("shaders/blit_es.frag");
    if (vertSrc.empty()) {
        vertSrc = kEmbeddedVert; // GLES2 fullscreen triangle
    }
    if (fragSrc.empty()) {
        fragSrc = kEmbeddedFragES;
    }
    if (blitVertSrc.empty()) {
        blitVertSrc = kEmbeddedBlitVert;
    }
    if (blitFragSrc.empty()) {
        blitFragSrc = kEmbeddedBlitFrag;
    }
#else
    std::string vertSrc = readFile("shaders/raytrace.vert");
    std::string fragSrc = readFile("shaders/raytrace.frag");
    std::string blitVertSrc = readFile("shaders/blit.vert");
    std::string blitFragSrc = readFile("shaders/blit.frag");
    if (vertSrc.empty()) {
        vertSrc = kEmbeddedVertGL;
    }
    if (fragSrc.empty()) {
        fragSrc = kEmbeddedFrag;
    }
    if (blitVertSrc.empty()) {
        blitVertSrc = kEmbeddedBlitVertGL;
    }
    if (blitFragSrc.empty()) {
        blitFragSrc = kEmbeddedBlitFragGL;
    }
#endif

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc.c_str());
    if (!vs || !fs) {
        if (vs) {
            glDeleteShader_(vs);
        }
        if (fs) {
            glDeleteShader_(fs);
        }
        return false;
    }
    uniformCache_.clear();
    if (!linkProgram(program_, vs, fs, "raytrace")) {
        glDeleteShader_(vs);
        glDeleteShader_(fs);
        return false;
    }
    glDeleteShader_(vs);
    glDeleteShader_(fs);

    GLuint bvs = compileShader(GL_VERTEX_SHADER, blitVertSrc.c_str());
    GLuint bfs = compileShader(GL_FRAGMENT_SHADER, blitFragSrc.c_str());
    if (!bvs || !bfs) {
        if (bvs) {
            glDeleteShader_(bvs);
        }
        if (bfs) {
            glDeleteShader_(bfs);
        }
        return false;
    }
    if (!linkProgram(blitProgram_, bvs, bfs, "blit")) {
        glDeleteShader_(bvs);
        glDeleteShader_(bfs);
        return false;
    }
    glDeleteShader_(bvs);
    glDeleteShader_(bfs);

    aPosLoc_ = glGetAttribLocation_(program_, "a_pos");
    aPosBlitLoc_ = glGetAttribLocation_(blitProgram_, "a_pos");
    uTexLoc_ = glGetUniformLocation_(blitProgram_, "u_tex");
    uUvScaleLoc_ = glGetUniformLocation_(blitProgram_, "u_uvScale");
    if (aPosLoc_ < 0 || aPosBlitLoc_ < 0) {
        std::fprintf(stderr, "Missing a_pos attribute in GLES2 shaders\n");
        return false;
    }
    return true;
}

void GpuRaytracer::syncDrawableSize() {
    if (!window_) {
        return;
    }
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(window_, &w, &h);
    drawableW_ = std::max(1, w);
    drawableH_ = std::max(1, h);
}

void GpuRaytracer::recomputeRtSize() {
    float s = renderScale_;
    if (s < 0.0625f) {
        s = 0.0625f;
    }
    if (s > 2.0f) {
        s = 2.0f;
    }
    int w = static_cast<int>(drawableW_ * s + 0.5f);
    int h = static_cast<int>(drawableH_ * s + 0.5f);
    // Uniform fit into the max box so aspect ratio is preserved.
    if ((maxW_ > 0 && w > maxW_) || (maxH_ > 0 && h > maxH_)) {
        float sx = (maxW_ > 0 && w > maxW_) ? static_cast<float>(maxW_) / static_cast<float>(w) : 1.0f;
        float sy = (maxH_ > 0 && h > maxH_) ? static_cast<float>(maxH_) / static_cast<float>(h) : 1.0f;
        const float sm = sx < sy ? sx : sy;
        w = std::max(1, static_cast<int>(w * sm + 0.5f));
        h = std::max(1, static_cast<int>(h * sm + 0.5f));
    }
    rtW_ = std::max(1, w);
    rtH_ = std::max(1, h);
    useFbo_ = (rtW_ != drawableW_ || rtH_ != drawableH_);
}

void GpuRaytracer::setRenderScale(float scale) {
    if (scale < 0.0625f) {
        scale = 0.0625f;
    }
    if (scale > 2.0f) {
        scale = 2.0f;
    }
    if (std::fabs(scale - renderScale_) < 1e-4f) {
        return;
    }
    renderScale_ = scale;
    recomputeRtSize();
}

void GpuRaytracer::setMaxResolution(int maxW, int maxH) {
    maxW_ = maxW;
    maxH_ = maxH;
    recomputeRtSize();
}

void GpuRaytracer::onResize(int /*windowW*/, int /*windowH*/) {
    if (!ready_) {
        return;
    }
    syncDrawableSize();
    recomputeRtSize();
}

bool GpuRaytracer::init(SDL_Window* window) {
    window_ = window;

    // GLES2 everywhere for the GPU path (desktop Mesa, mobile, WebGL1).
#if defined(KUGELMATCH_USE_OPENGLES2) || defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    glctx_ = SDL_GL_CreateContext(window_);
    if (!glctx_) {
        std::fprintf(stderr, "SDL_GL_CreateContext (GLES2) failed: %s\n", SDL_GetError());
        return false;
    }
    if (SDL_GL_MakeCurrent(window_, glctx_) != 0) {
        std::fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        SDL_GL_DeleteContext(glctx_);
        glctx_ = nullptr;
        return false;
    }

    if (!loadAllProcs()) {
        std::fprintf(stderr, "Failed to load GLES2 entry points\n");
        shutdown();
        return false;
    }
    if (!loadShaders()) {
        shutdown();
        return false;
    }

    glGenBuffers_(1, &vbo_);
    glBindBuffer_(GL_ARRAY_BUFFER, vbo_);
    glBufferData_(GL_ARRAY_BUFFER, sizeof(kFullscreenTri), kFullscreenTri, GL_STATIC_DRAW);
    glBindBuffer_(GL_ARRAY_BUFFER, 0);

    syncDrawableSize();
    recomputeRtSize();
    ready_ = true;
    #if defined(KUGELMATCH_USE_OPENGLES2) || defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    std::fprintf(stderr, "GPU backend: OpenGL ES 2.0 (fragment-shader raytracer)\n");
#else
    std::fprintf(stderr, "GPU backend: OpenGL 3.3 (fragment-shader raytracer)\n");
#endif

    return true;
}

void GpuRaytracer::destroyFbo() {
    if (fbo_) {
        glDeleteFramebuffers_(1, &fbo_);
        fbo_ = 0;
    }
    if (fboTex_) {
        glDeleteTextures_(1, &fboTex_);
        fboTex_ = 0;
    }
    fboW_ = fboH_ = 0;
}

bool GpuRaytracer::ensureFbo(int w, int h) {
    if (fbo_ && fboW_ == w && fboH_ == h) {
        return true;
    }
    destroyFbo();

    glGenTextures_(1, &fboTex_);
    glBindTexture_(GL_TEXTURE_2D, fboTex_);
    // GLES2: internal format must match format (no GL_RGBA8).
    glTexImage2D_(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri_(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri_(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri_(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri_(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture_(GL_TEXTURE_2D, 0);

    glGenFramebuffers_(1, &fbo_);
    glBindFramebuffer_(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D_(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex_, 0);
    const GLenum status = glCheckFramebufferStatus_(GL_FRAMEBUFFER);
    glBindFramebuffer_(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "FBO incomplete: 0x%x\n", status);
        destroyFbo();
        return false;
    }
    fboW_ = w;
    fboH_ = h;
    return true;
}

void GpuRaytracer::drawFullscreenTriangle(int aPosLoc) const {
    glBindBuffer_(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray_(static_cast<GLuint>(aPosLoc));
    glVertexAttribPointer_(static_cast<GLuint>(aPosLoc), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays_(GL_TRIANGLES, 0, 3);
    glDisableVertexAttribArray_(static_cast<GLuint>(aPosLoc));
    glBindBuffer_(GL_ARRAY_BUFFER, 0);
}

void GpuRaytracer::uploadScene(const Scene& scene, const Camera& cam) const {
    auto find = [&](const char* n) -> GLint {
        auto it = uniformCache_.find(n);
        if (it != uniformCache_.end()) {
            return it->second;
        }
        const GLint loc = glGetUniformLocation_(program_, n);
        uniformCache_[n] = loc;
        return loc;
    };
    auto loc1i = [&](const char* n, int v) {
        const GLint loc = find(n);
        if (loc >= 0) {
            glUniform1i_(loc, v);
        }
    };
    auto loc1f = [&](const char* n, float v) {
        const GLint loc = find(n);
        if (loc >= 0) {
            glUniform1f_(loc, v);
        }
    };
    auto loc3 = [&](const char* n, const Vec3& v) {
        const GLint loc = find(n);
        if (loc >= 0) {
            glUniform3f_(loc, v.x, v.y, v.z);
        }
    };
    auto arr3 = [&](const char* n, size_t count, auto getter) {
        const GLint loc = find(n);
        if (loc < 0 || count == 0) {
            return;
        }
        std::vector<float> data(count * 3u);
        for (size_t i = 0; i < count; ++i) {
            const Vec3 v = getter(i);
            data[i * 3u] = v.x;
            data[i * 3u + 1u] = v.y;
            data[i * 3u + 2u] = v.z;
        }
        glUniform3fv_(loc, static_cast<GLsizei>(count), data.data());
    };
    auto arr1f = [&](const char* n, size_t count, auto getter) {
        const GLint loc = find(n);
        if (loc < 0 || count == 0) {
            return;
        }
        std::vector<float> data(count);
        for (size_t i = 0; i < count; ++i) {
            data[i] = getter(i);
        }
        glUniform1fv_(loc, static_cast<GLsizei>(count), data.data());
    };
    auto arr1i = [&](const char* n, size_t count, auto getter) {
        const GLint loc = find(n);
        if (loc < 0 || count == 0) {
            return;
        }
        std::vector<GLint> data(count);
        for (size_t i = 0; i < count; ++i) {
            data[i] = getter(i);
        }
        glUniform1iv_(loc, static_cast<GLsizei>(count), data.data());
    };

    loc3("u_camPos", cam.pos);
    loc3("u_camForward", cam.forward);
    loc3("u_camRight", cam.right);
    loc3("u_camUp", cam.up);
    loc1f("u_fovScale", cam.fovScale);
    float aspect = rtH_ > 0 ? static_cast<float>(rtW_) / static_cast<float>(rtH_) : 1.0f;
    loc1f("u_aspect", aspect);
    loc1f("u_pixelAngle", rtH_ > 0 ? 2.0f * cam.fovScale / static_cast<float>(rtH_) : 0.0f);

    loc3("u_lightPos", scene.lightPos);
    loc3("u_lightColor", scene.lightColor);
    loc3("u_ambient", scene.ambient);
    loc3("u_skyColor", scene.skyColor);
    loc1i("u_maxBounces", scene.maxBounces);
    loc1i("u_shadowSamples", scene.shadowSamples);
    loc1f("u_exposure", scene.exposure);

    const size_t ns = static_cast<size_t>(std::min(static_cast<int>(scene.spheres.size()), MAX_SPHERES));
    loc1i("u_numSpheres", static_cast<int>(ns));
    arr3("u_sphereCenter", ns, [&](size_t i) { return scene.spheres[i].center; });
    arr1f("u_sphereRadius", ns, [&](size_t i) { return scene.spheres[i].radius; });
    arr3("u_sphereColor", ns, [&](size_t i) { return scene.spheres[i].color; });
    arr1f("u_sphereReflect", ns, [&](size_t i) { return scene.spheres[i].reflectivity; });

    const size_t nb = static_cast<size_t>(std::min(static_cast<int>(scene.boxes.size()), MAX_BOXES));
    loc1i("u_numBoxes", static_cast<int>(nb));
    arr3("u_boxMin", nb, [&](size_t i) { return scene.boxes[i].minb; });
    arr3("u_boxMax", nb, [&](size_t i) { return scene.boxes[i].maxb; });
    arr3("u_boxColor", nb, [&](size_t i) { return scene.boxes[i].color; });
    arr1f("u_boxReflect", nb, [&](size_t i) { return scene.boxes[i].reflectivity; });

    const size_t np = static_cast<size_t>(std::min(static_cast<int>(scene.planes.size()), MAX_PLANES));
    loc1i("u_numPlanes", static_cast<int>(np));
    arr3("u_planePoint", np, [&](size_t i) { return scene.planes[i].point; });
    arr3("u_planeNormal", np, [&](size_t i) { return scene.planes[i].normal; });
    arr3("u_planeColorA", np, [&](size_t i) { return scene.planes[i].colorA; });
    arr3("u_planeColorB", np, [&](size_t i) { return scene.planes[i].colorB; });
    arr1f("u_planeScale", np, [&](size_t i) { return scene.planes[i].scale; });
    arr1f("u_planeReflect", np, [&](size_t i) { return scene.planes[i].reflectivity; });
    arr1i("u_planeChecker", np, [&](size_t i) { return scene.planes[i].checker ? 1 : 0; });
    arr1i("u_planeOneSided", np, [&](size_t i) { return scene.planes[i].oneSided ? 1 : 0; });
}

void GpuRaytracer::render(const Scene& scene, const Camera& cam) {
    if (!ready_ || !window_ || !glctx_) {
        return;
    }
    if (SDL_GL_MakeCurrent(window_, glctx_) != 0) {
        return;
    }

    int prevW = drawableW_, prevH = drawableH_;
    syncDrawableSize();
    if (drawableW_ != prevW || drawableH_ != prevH) {
        recomputeRtSize();
    }

    // FBO matches the raytrace resolution (rtW×rtH). Viewport is always the
    // full FBO; a textured blit scales to the drawable. The old "drawable-sized
    // FBO + partial viewport" path broke whenever rt > drawable (scale > 1 or
    // supersample) because glViewport larger than the attachment is invalid.
    if (useFbo_) {
        if (!ensureFbo(rtW_, rtH_)) {
            // Fall back to direct drawable render for this frame.
            useFbo_ = false;
            glBindFramebuffer_(GL_FRAMEBUFFER, 0);
            glViewport_(0, 0, drawableW_, drawableH_);
        } else {
            glBindFramebuffer_(GL_FRAMEBUFFER, fbo_);
            glViewport_(0, 0, fboW_, fboH_);
        }
    } else {
        glBindFramebuffer_(GL_FRAMEBUFFER, 0);
        glViewport_(0, 0, drawableW_, drawableH_);
    }

    glClearColor_(0.f, 0.f, 0.f, 1.f);
    glClear_(GL_COLOR_BUFFER_BIT);

    glUseProgram_(program_);
    uploadScene(scene, cam);
    drawFullscreenTriangle(aPosLoc_);

    if (useFbo_ && fbo_ && blitProgram_) {
        // GLES2: scale with a textured fullscreen triangle (no glBlitFramebuffer).
        glBindFramebuffer_(GL_FRAMEBUFFER, 0);
        glViewport_(0, 0, drawableW_, drawableH_);
        glClear_(GL_COLOR_BUFFER_BIT);
        glUseProgram_(blitProgram_);
        if (uTexLoc_ >= 0) {
            glUniform1i_(uTexLoc_, 0);
        }
        // FBO is exactly the raytraced image — sample the full texture.
        if (uUvScaleLoc_ >= 0 && glUniform2f_) {
            glUniform2f_(uUvScaleLoc_, 1.f, 1.f);
        }
        glActiveTexture_(GL_TEXTURE0);
        glBindTexture_(GL_TEXTURE_2D, fboTex_);
        drawFullscreenTriangle(aPosBlitLoc_);
        glBindTexture_(GL_TEXTURE_2D, 0);
    }
}

void GpuRaytracer::present() {
    if (!ready_ || !window_ || !glctx_) {
        return;
    }
    SDL_GL_SwapWindow(window_);
}

void GpuRaytracer::finish() {
    if (!ready_ || !glFinish_) {
        return;
    }
    glFinish_();
}

bool GpuRaytracer::readPixels(std::vector<uint32_t>& argb, int& w, int& h) const {
    if (!ready_) {
        return false;
    }
    w = drawableW_;
    h = drawableH_;
    if (w <= 0 || h <= 0) {
        return false;
    }
    std::vector<uint8_t> rgba(static_cast<size_t>(w) * static_cast<size_t>(h) * 4u);
    glBindFramebuffer_(GL_FRAMEBUFFER, 0);
    glPixelStorei_(GL_PACK_ALIGNMENT, 1);
    glReadPixels_(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    argb.resize(static_cast<size_t>(w) * static_cast<size_t>(h));
    for (int y = 0; y < h; ++y) {
        const uint8_t* src = &rgba[static_cast<size_t>(h - 1 - y) * static_cast<size_t>(w) * 4u];
        uint32_t* dst = &argb[static_cast<size_t>(y) * static_cast<size_t>(w)];
        for (int x = 0; x < w; ++x) {
            dst[x] = 0xFF000000u | (static_cast<uint32_t>(src[x * 4]) << 16) |
                     (static_cast<uint32_t>(src[x * 4 + 1]) << 8) | static_cast<uint32_t>(src[x * 4 + 2]);
        }
    }
    return true;
}
