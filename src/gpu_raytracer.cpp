// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#include "gpu_raytracer.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include <SDL_opengl.h>

// Core FBO tokens (in case the system GL header is limited)
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif


namespace {

using PFNGLCREATESHADERPROC = GLuint (*)(GLenum);
using PFNGLSHADERSOURCEPROC = void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
using PFNGLCOMPILESHADERPROC = void (*)(GLuint);
using PFNGLGETSHADERIVPROC = void (*)(GLuint, GLenum, GLint*);
using PFNGLGETSHADERINFOLOGPROC = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFNGLCREATEPROGRAMPROC = GLuint (*)(void);
using PFNGLATTACHSHADERPROC = void (*)(GLuint, GLuint);
using PFNGLLINKPROGRAMPROC = void (*)(GLuint);
using PFNGLGETPROGRAMIVPROC = void (*)(GLuint, GLenum, GLint*);
using PFNGLGETPROGRAMINFOLOGPROC = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFNGLDELETESHADERPROC = void (*)(GLuint);
using PFNGLDELETEPROGRAMPROC = void (*)(GLuint);
using PFNGLUSEPROGRAMPROC = void (*)(GLuint);
using PFNGLGETUNIFORMLOCATIONPROC = GLint (*)(GLuint, const GLchar*);
using PFNGLUNIFORM1IPROC = void (*)(GLint, GLint);
using PFNGLUNIFORM1FPROC = void (*)(GLint, GLfloat);
using PFNGLUNIFORM3FPROC = void (*)(GLint, GLfloat, GLfloat, GLfloat);
using PFNGLGENVERTEXARRAYSPROC = void (*)(GLsizei, GLuint*);
using PFNGLBINDVERTEXARRAYPROC = void (*)(GLuint);
using PFNGLDELETEVERTEXARRAYSPROC = void (*)(GLsizei, const GLuint*);
using PFNGLDRAWARRAYSPROC = void (*)(GLenum, GLint, GLsizei);
using PFNGLGENFRAMEBUFFERSPROC = void (*)(GLsizei, GLuint*);
using PFNGLBINDFRAMEBUFFERPROC = void (*)(GLenum, GLuint);
using PFNGLFRAMEBUFFERTEXTURE2DPROC = void (*)(GLenum, GLenum, GLenum, GLuint, GLint);
using PFNGLCHECKFRAMEBUFFERSTATUSPROC = GLenum (*)(GLenum);
using PFNGLDELETEFRAMEBUFFERSPROC = void (*)(GLsizei, const GLuint*);
using PFNGLGENTEXTURESPROC = void (*)(GLsizei, GLuint*);
using PFNGLBINDTEXTUREPROC = void (*)(GLenum, GLuint);
using PFNGLTEXIMAGE2DPROC = void (*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
using PFNGLTEXPARAMETERIPROC = void (*)(GLenum, GLenum, GLint);
using PFNGLDELETETEXTURESPROC = void (*)(GLsizei, const GLuint*);
using PFNGLBLITFRAMEBUFFERPROC = void (*)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
using PFNGLVIEWPORTPROC = void (*)(GLint, GLint, GLsizei, GLsizei);
using PFNGLCLEARPROC = void (*)(GLbitfield);
using PFNGLCLEARCOLORPROC = void (*)(GLfloat, GLfloat, GLfloat, GLfloat);

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
PFNGLUNIFORM1IPROC glUniform1i_ = nullptr;
PFNGLUNIFORM1FPROC glUniform1f_ = nullptr;
PFNGLUNIFORM3FPROC glUniform3f_ = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays_ = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray_ = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays_ = nullptr;
PFNGLDRAWARRAYSPROC glDrawArrays_ = nullptr;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers_ = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer_ = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D_ = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus_ = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers_ = nullptr;
PFNGLGENTEXTURESPROC glGenTextures_ = nullptr;
PFNGLBINDTEXTUREPROC glBindTexture_ = nullptr;
PFNGLTEXIMAGE2DPROC glTexImage2D_ = nullptr;
PFNGLTEXPARAMETERIPROC glTexParameteri_ = nullptr;
PFNGLDELETETEXTURESPROC glDeleteTextures_ = nullptr;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer_ = nullptr;
PFNGLVIEWPORTPROC glViewport_ = nullptr;
PFNGLCLEARPROC glClear_ = nullptr;
PFNGLCLEARCOLORPROC glClearColor_ = nullptr;

template <typename T>
bool loadProc(T& fn, const char* name) {
    fn = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!fn) {
        std::fprintf(stderr, "Failed to load GL function: %s\n", name);
        return false;
    }
    return true;
}

bool loadGL() {
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
    ok &= loadProc(glUniform1i_, "glUniform1i");
    ok &= loadProc(glUniform1f_, "glUniform1f");
    ok &= loadProc(glUniform3f_, "glUniform3f");
    ok &= loadProc(glGenVertexArrays_, "glGenVertexArrays");
    ok &= loadProc(glBindVertexArray_, "glBindVertexArray");
    ok &= loadProc(glDeleteVertexArrays_, "glDeleteVertexArrays");
    ok &= loadProc(glDrawArrays_, "glDrawArrays");
    ok &= loadProc(glGenFramebuffers_, "glGenFramebuffers");
    ok &= loadProc(glBindFramebuffer_, "glBindFramebuffer");
    ok &= loadProc(glFramebufferTexture2D_, "glFramebufferTexture2D");
    ok &= loadProc(glCheckFramebufferStatus_, "glCheckFramebufferStatus");
    ok &= loadProc(glDeleteFramebuffers_, "glDeleteFramebuffers");
    ok &= loadProc(glGenTextures_, "glGenTextures");
    ok &= loadProc(glBindTexture_, "glBindTexture");
    ok &= loadProc(glTexImage2D_, "glTexImage2D");
    ok &= loadProc(glTexParameteri_, "glTexParameteri");
    ok &= loadProc(glDeleteTextures_, "glDeleteTextures");
    ok &= loadProc(glBlitFramebuffer_, "glBlitFramebuffer");
    ok &= loadProc(glViewport_, "glViewport");
    ok &= loadProc(glClear_, "glClear");
    ok &= loadProc(glClearColor_, "glClearColor");
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

const char* kEmbeddedVert = R"GLSL(
#version 330 core
out vec2 v_uv;
void main() {
    float x = float((gl_VertexID & 1) << 2) - 1.0;
    float y = float((gl_VertexID & 2) << 1) - 1.0;
    v_uv = vec2(x, y) * 0.5 + 0.5;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)GLSL";

#include "embedded_frag.inc"

} // namespace

GpuRaytracer::~GpuRaytracer() {
    if (glctx_) {
        if (fbo_) {
            glDeleteFramebuffers_(1, &fbo_);
            fbo_ = 0;
        }
        if (colorTex_) {
            glDeleteTextures_(1, &colorTex_);
            colorTex_ = 0;
        }
        if (program_) {
            glDeleteProgram_(program_);
            program_ = 0;
        }
        if (vao_) {
            glDeleteVertexArrays_(1, &vao_);
            vao_ = 0;
        }
        SDL_GL_DeleteContext(glctx_);
        glctx_ = nullptr;
    }
    ready_ = false;
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

bool GpuRaytracer::loadShaders() {
    std::string vertSrc = readFile("shaders/raytrace.vert");
    std::string fragSrc = readFile("shaders/raytrace.frag");
    if (vertSrc.empty()) {
        vertSrc = kEmbeddedVert;
    }
    if (fragSrc.empty()) {
        fragSrc = kEmbeddedFrag;
    }

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

    program_ = glCreateProgram_();
    glAttachShader_(program_, vs);
    glAttachShader_(program_, fs);
    glLinkProgram_(program_);
    glDeleteShader_(vs);
    glDeleteShader_(fs);

    GLint ok = 0;
    glGetProgramiv_(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog_(program_, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Program link error:\n%s\n", log);
        glDeleteProgram_(program_);
        program_ = 0;
        return false;
    }
    return true;
}

bool GpuRaytracer::createFbo() {
    glGenTextures_(1, &colorTex_);
    glBindTexture_(GL_TEXTURE_2D, colorTex_);
    glTexImage2D_(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri_(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri_(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri_(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri_(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers_(1, &fbo_);
    glBindFramebuffer_(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D_(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex_, 0);
    GLenum status = glCheckFramebufferStatus_(GL_FRAMEBUFFER);
    glBindFramebuffer_(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "FBO incomplete: 0x%x\n", status);
        return false;
    }
    return true;
}

bool GpuRaytracer::init(SDL_Window* window) {
    window_ = window;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    glctx_ = SDL_GL_CreateContext(window_);
    if (!glctx_) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    if (SDL_GL_MakeCurrent(window_, glctx_) != 0) {
        std::fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetSwapInterval(1);

    if (!loadGL()) {
        return false;
    }
    if (!loadShaders()) {
        return false;
    }
    if (!createFbo()) {
        return false;
    }

    glGenVertexArrays_(1, &vao_);
    glBindVertexArray_(vao_);

    SDL_GetWindowSize(window_, &winW_, &winH_);
    ready_ = true;
    return true;
}

void GpuRaytracer::onResize(int windowW, int windowH) {
    winW_ = std::max(1, windowW);
    winH_ = std::max(1, windowH);
}

void GpuRaytracer::letterboxDst(int& dx, int& dy, int& dw, int& dh) const {
    const float srcAspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float dstAspect = static_cast<float>(winW_) / static_cast<float>(winH_);
    if (dstAspect > srcAspect) {
        // window wider — pillarbox
        dh = winH_;
        dw = static_cast<int>(winH_ * srcAspect + 0.5f);
        dx = (winW_ - dw) / 2;
        dy = 0;
    } else {
        // window taller — letterbox
        dw = winW_;
        dh = static_cast<int>(winW_ / srcAspect + 0.5f);
        dx = 0;
        dy = (winH_ - dh) / 2;
    }
}

void GpuRaytracer::uploadScene(const Scene& scene, const Camera& cam) const {
    glUseProgram_(program_);

    auto loc3 = [&](const char* name, const Vec3& v) {
        GLint l = glGetUniformLocation_(program_, name);
        if (l >= 0) {
            glUniform3f_(l, v.x, v.y, v.z);
        }
    };
    auto loc1f = [&](const char* name, float v) {
        GLint l = glGetUniformLocation_(program_, name);
        if (l >= 0) {
            glUniform1f_(l, v);
        }
    };
    auto loc1i = [&](const char* name, int v) {
        GLint l = glGetUniformLocation_(program_, name);
        if (l >= 0) {
            glUniform1i_(l, v);
        }
    };

    loc3("u_camPos", cam.pos);
    loc3("u_camForward", cam.forward);
    loc3("u_camRight", cam.right);
    loc3("u_camUp", cam.up);
    loc1f("u_fovScale", cam.fovScale);
    loc1f("u_aspect", static_cast<float>(WIDTH) / static_cast<float>(HEIGHT));

    loc3("u_lightPos", scene.lightPos);
    loc3("u_lightColor", scene.lightColor);
    loc3("u_ambient", scene.ambient);
    loc3("u_skyColor", scene.skyColor);

    int ns = std::min(static_cast<int>(scene.spheres.size()), MAX_SPHERES);
    loc1i("u_numSpheres", ns);
    for (int i = 0; i < ns; ++i) {
        const auto& s = scene.spheres[static_cast<size_t>(i)];
        char name[64];
        std::snprintf(name, sizeof(name), "u_sphereCenter[%d]", i);
        loc3(name, s.center);
        std::snprintf(name, sizeof(name), "u_sphereRadius[%d]", i);
        loc1f(name, s.radius);
        std::snprintf(name, sizeof(name), "u_sphereColor[%d]", i);
        loc3(name, s.color);
        std::snprintf(name, sizeof(name), "u_sphereReflect[%d]", i);
        loc1f(name, s.reflectivity);
    }

    int nb = std::min(static_cast<int>(scene.boxes.size()), MAX_BOXES);
    loc1i("u_numBoxes", nb);
    for (int i = 0; i < nb; ++i) {
        const auto& b = scene.boxes[static_cast<size_t>(i)];
        char name[64];
        std::snprintf(name, sizeof(name), "u_boxMin[%d]", i);
        loc3(name, b.minb);
        std::snprintf(name, sizeof(name), "u_boxMax[%d]", i);
        loc3(name, b.maxb);
        std::snprintf(name, sizeof(name), "u_boxColor[%d]", i);
        loc3(name, b.color);
        std::snprintf(name, sizeof(name), "u_boxReflect[%d]", i);
        loc1f(name, b.reflectivity);
    }

    int np = std::min(static_cast<int>(scene.planes.size()), MAX_PLANES);
    loc1i("u_numPlanes", np);
    for (int i = 0; i < np; ++i) {
        const auto& p = scene.planes[static_cast<size_t>(i)];
        char name[64];
        std::snprintf(name, sizeof(name), "u_planePoint[%d]", i);
        loc3(name, p.point);
        std::snprintf(name, sizeof(name), "u_planeNormal[%d]", i);
        loc3(name, p.normal);
        std::snprintf(name, sizeof(name), "u_planeColorA[%d]", i);
        loc3(name, p.colorA);
        std::snprintf(name, sizeof(name), "u_planeColorB[%d]", i);
        loc3(name, p.colorB);
        std::snprintf(name, sizeof(name), "u_planeScale[%d]", i);
        loc1f(name, p.scale);
        std::snprintf(name, sizeof(name), "u_planeReflect[%d]", i);
        loc1f(name, p.reflectivity);
        std::snprintf(name, sizeof(name), "u_planeChecker[%d]", i);
        loc1i(name, p.checker ? 1 : 0);
    }
}

void GpuRaytracer::render(const Scene& scene, const Camera& cam) {
    if (!ready_) {
        return;
    }

    // Raytrace into fixed-resolution FBO
    glBindFramebuffer_(GL_FRAMEBUFFER, fbo_);
    glViewport_(0, 0, WIDTH, HEIGHT);
    glClearColor_(0.f, 0.f, 0.f, 1.f);
    glClear_(GL_COLOR_BUFFER_BIT);

    uploadScene(scene, cam);
    glBindVertexArray_(vao_);
    glDrawArrays_(GL_TRIANGLES, 0, 3);
    glBindFramebuffer_(GL_FRAMEBUFFER, 0);
}

void GpuRaytracer::present() {
    if (!ready_ || !window_ || !glctx_) {
        return;
    }

    // Use drawable size (handles HiDPI)
    int dw = winW_, dh = winH_;
    SDL_GL_GetDrawableSize(window_, &dw, &dh);
    winW_ = std::max(1, dw);
    winH_ = std::max(1, dh);

    glBindFramebuffer_(GL_FRAMEBUFFER, 0);
    glViewport_(0, 0, winW_, winH_);
    glClearColor_(0.f, 0.f, 0.f, 1.f);
    glClear_(GL_COLOR_BUFFER_BIT);

    int dx = 0, dy = 0, dww = winW_, dhh = winH_;
    letterboxDst(dx, dy, dww, dhh);

    glBindFramebuffer_(GL_READ_FRAMEBUFFER, fbo_);
    glBindFramebuffer_(GL_DRAW_FRAMEBUFFER, 0);
    // Note: GL Y is bottom-up; FBO content matches shader orientation
    glBlitFramebuffer_(0, 0, WIDTH, HEIGHT, dx, dy, dx + dww, dy + dhh,
                       GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer_(GL_FRAMEBUFFER, 0);

    SDL_GL_SwapWindow(window_);
}
