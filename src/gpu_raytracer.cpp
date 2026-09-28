// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#include "gpu_raytracer.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Minimal OpenGL 3.3 core loader via SDL_GL_GetProcAddress
#include <SDL_opengl.h>

// Function pointers we need (avoid GLEW dependency)
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
using PFNGLUNIFORM3FVPROC = void (*)(GLint, GLsizei, const GLfloat*);
using PFNGLUNIFORM1FVPROC = void (*)(GLint, GLsizei, const GLfloat*);
using PFNGLUNIFORM1IVPROC = void (*)(GLint, GLsizei, const GLint*);
using PFNGLGENVERTEXARRAYSPROC = void (*)(GLsizei, GLuint*);
using PFNGLBINDVERTEXARRAYPROC = void (*)(GLuint);
using PFNGLDELETEVERTEXARRAYSPROC = void (*)(GLsizei, const GLuint*);
using PFNGLDRAWARRAYSPROC = void (*)(GLenum, GLint, GLsizei);

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
PFNGLUNIFORM3FVPROC glUniform3fv_ = nullptr;
PFNGLUNIFORM1FVPROC glUniform1fv_ = nullptr;
PFNGLUNIFORM1IVPROC glUniform1iv_ = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays_ = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray_ = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays_ = nullptr;
PFNGLDRAWARRAYSPROC glDrawArrays_ = nullptr;

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
    ok &= loadProc(glUniform3fv_, "glUniform3fv");
    ok &= loadProc(glUniform1fv_, "glUniform1fv");
    ok &= loadProc(glUniform1iv_, "glUniform1iv");
    ok &= loadProc(glGenVertexArrays_, "glGenVertexArrays");
    ok &= loadProc(glBindVertexArray_, "glBindVertexArray");
    ok &= loadProc(glDeleteVertexArrays_, "glDeleteVertexArrays");
    ok &= loadProc(glDrawArrays_, "glDrawArrays");
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

// Fallback embedded shaders if files are not found next to the binary
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

// Note: fragment shader is large; we prefer loading from disk.
// Embedded copy kept in sync with shaders/raytrace.frag
#include "embedded_frag.inc"  // generated from shaders/raytrace.frag

} // namespace

GpuRaytracer::~GpuRaytracer() {
    if (glctx_) {
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
        if (vs) glDeleteShader_(vs);
        if (fs) glDeleteShader_(fs);
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

    SDL_GL_SetSwapInterval(1); // vsync when available

    if (!loadGL()) {
        return false;
    }

    if (!loadShaders()) {
        return false;
    }

    glGenVertexArrays_(1, &vao_);
    glBindVertexArray_(vao_);

    ready_ = true;
    return true;
}

void GpuRaytracer::uploadScene(const Scene& scene, const Camera& cam) const {
    glUseProgram_(program_);

    auto loc3 = [&](const char* name, const Vec3& v) {
        GLint l = glGetUniformLocation_(program_, name);
        if (l >= 0) glUniform3f_(l, v.x, v.y, v.z);
    };
    auto loc1f = [&](const char* name, float v) {
        GLint l = glGetUniformLocation_(program_, name);
        if (l >= 0) glUniform1f_(l, v);
    };
    auto loc1i = [&](const char* name, int v) {
        GLint l = glGetUniformLocation_(program_, name);
        if (l >= 0) glUniform1i_(l, v);
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
    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    uploadScene(scene, cam);
    glBindVertexArray_(vao_);
    glDrawArrays_(GL_TRIANGLES, 0, 3);
}

void GpuRaytracer::present() {
    if (window_ && glctx_) {
        SDL_GL_SwapWindow(window_);
    }
}
