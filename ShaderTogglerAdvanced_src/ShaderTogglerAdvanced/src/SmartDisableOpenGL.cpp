// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#include "SmartDisableOther.h"
#include "Vendor/MinHook/include/MinHook.h"
#include "Vendor/Khronos/glcorearb.h"
#include <vector>
#include <memory>
#include <algorithm>
namespace ShaderToggler::smart::other::gl
{
    struct Functions
    {
        PFNGLGETSTRINGPROC GetString = nullptr;
        PFNGLGETINTEGERVPROC GetIntegerv = nullptr;
        PFNGLGETBOOLEANVPROC GetBooleanv = nullptr;
        PFNGLISENABLEDPROC IsEnabled = nullptr;
        PFNGLGETPROGRAMSTAGEIVPROC GetProgramStageiv = nullptr;
        PFNGLGETPROGRAMIVPROC GetProgramiv = nullptr;
        PFNGLGETSHADERIVPROC GetShaderiv = nullptr;
        PFNGLGETATTACHEDSHADERSPROC GetAttachedShaders = nullptr;
        PFNGLGETPROGRAMINTERFACEIVPROC GetProgramInterfaceiv = nullptr;
        PFNGLGETPROGRAMRESOURCEIVPROC GetProgramResourceiv = nullptr;
        PFNGLGETPROGRAMRESOURCENAMEPROC GetProgramResourceName = nullptr;
        PFNGLGETPROGRAMRESOURCEINDEXPROC GetProgramResourceIndex = nullptr;
        PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = nullptr;
        PFNGLGETATTRIBLOCATIONPROC GetAttribLocation = nullptr;
        PFNGLGETACTIVEATTRIBPROC GetActiveAttrib = nullptr;
        PFNGLCREATESHADERPROC CreateShader = nullptr;
        PFNGLSHADERSOURCEPROC ShaderSource = nullptr;
        PFNGLCOMPILESHADERPROC CompileShader = nullptr;
        PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = nullptr;
        PFNGLDELETESHADERPROC DeleteShader = nullptr;
        PFNGLCREATEPROGRAMPROC CreateProgram = nullptr;
        PFNGLATTACHSHADERPROC AttachShader = nullptr;
        PFNGLDETACHSHADERPROC DetachShader = nullptr;
        PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation = nullptr;
        PFNGLLINKPROGRAMPROC LinkProgram = nullptr;
        PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = nullptr;
        PFNGLDELETEPROGRAMPROC DeleteProgram = nullptr;
        PFNGLUSEPROGRAMPROC UseProgram = nullptr;
        PFNGLGETUNIFORMFVPROC GetUniformfv = nullptr;
        PFNGLGETUNIFORMIVPROC GetUniformiv = nullptr;
        PFNGLGETUNIFORMUIVPROC GetUniformuiv = nullptr;
        PFNGLGETUNIFORMDVPROC GetUniformdv = nullptr;
        PFNGLGETACTIVEUNIFORMBLOCKIVPROC GetActiveUniformBlockiv = nullptr;
        PFNGLUNIFORMBLOCKBINDINGPROC UniformBlockBinding = nullptr;
        PFNGLBINDBUFFERPROC BindBuffer = nullptr;
        PFNGLPROGRAMUNIFORM1FVPROC ProgramUniform1fv = nullptr;
        PFNGLPROGRAMUNIFORM2FVPROC ProgramUniform2fv = nullptr;
        PFNGLPROGRAMUNIFORM3FVPROC ProgramUniform3fv = nullptr;
        PFNGLPROGRAMUNIFORM4FVPROC ProgramUniform4fv = nullptr;
        PFNGLPROGRAMUNIFORM1IVPROC ProgramUniform1iv = nullptr;
        PFNGLPROGRAMUNIFORM2IVPROC ProgramUniform2iv = nullptr;
        PFNGLPROGRAMUNIFORM3IVPROC ProgramUniform3iv = nullptr;
        PFNGLPROGRAMUNIFORM4IVPROC ProgramUniform4iv = nullptr;
        PFNGLPROGRAMUNIFORM1UIVPROC ProgramUniform1uiv = nullptr;
        PFNGLPROGRAMUNIFORM2UIVPROC ProgramUniform2uiv = nullptr;
        PFNGLPROGRAMUNIFORM3UIVPROC ProgramUniform3uiv = nullptr;
        PFNGLPROGRAMUNIFORM4UIVPROC ProgramUniform4uiv = nullptr;
        PFNGLPROGRAMUNIFORM1DVPROC ProgramUniform1dv = nullptr;
        PFNGLPROGRAMUNIFORM2DVPROC ProgramUniform2dv = nullptr;
        PFNGLPROGRAMUNIFORM3DVPROC ProgramUniform3dv = nullptr;
        PFNGLPROGRAMUNIFORM4DVPROC ProgramUniform4dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX2FVPROC ProgramUniformMatrix2fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX3FVPROC ProgramUniformMatrix3fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX4FVPROC ProgramUniformMatrix4fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC ProgramUniformMatrix2x3fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC ProgramUniformMatrix3x2fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC ProgramUniformMatrix2x4fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC ProgramUniformMatrix4x2fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC ProgramUniformMatrix3x4fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC ProgramUniformMatrix4x3fv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX2DVPROC ProgramUniformMatrix2dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX3DVPROC ProgramUniformMatrix3dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX4DVPROC ProgramUniformMatrix4dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC ProgramUniformMatrix2x3dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC ProgramUniformMatrix3x2dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC ProgramUniformMatrix2x4dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC ProgramUniformMatrix4x2dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC ProgramUniformMatrix3x4dv = nullptr;
        PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC ProgramUniformMatrix4x3dv = nullptr;
        bool load()
        {
            bool ok = true;
            HMODULE module = GetModuleHandleW(L"opengl32.dll");
            auto proc = [&](const char* name) -> PROC {
                PROC p = wglGetProcAddress(name);
                if (!p || p == reinterpret_cast<PROC>(1) || p == reinterpret_cast<PROC>(2) ||
                    p == reinterpret_cast<PROC>(3) || p == reinterpret_cast<PROC>(-1))
                    p = module ? reinterpret_cast<PROC>(GetProcAddress(module, name)) : nullptr;
                return p;
            };
            GetString = reinterpret_cast<PFNGLGETSTRINGPROC>(proc("glGetString")); ok &= GetString != nullptr;
            GetIntegerv = reinterpret_cast<PFNGLGETINTEGERVPROC>(proc("glGetIntegerv")); ok &= GetIntegerv != nullptr;
            GetBooleanv = reinterpret_cast<PFNGLGETBOOLEANVPROC>(proc("glGetBooleanv")); ok &= GetBooleanv != nullptr;
            IsEnabled = reinterpret_cast<PFNGLISENABLEDPROC>(proc("glIsEnabled")); ok &= IsEnabled != nullptr;
            GetProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(proc("glGetProgramiv")); ok &= GetProgramiv != nullptr;
            GetProgramStageiv = reinterpret_cast<PFNGLGETPROGRAMSTAGEIVPROC>(proc("glGetProgramStageiv")); ok &= GetProgramStageiv != nullptr;
            GetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(proc("glGetShaderiv")); ok &= GetShaderiv != nullptr;
            GetAttachedShaders = reinterpret_cast<PFNGLGETATTACHEDSHADERSPROC>(proc("glGetAttachedShaders")); ok &= GetAttachedShaders != nullptr;
            GetProgramInterfaceiv = reinterpret_cast<PFNGLGETPROGRAMINTERFACEIVPROC>(proc("glGetProgramInterfaceiv")); ok &= GetProgramInterfaceiv != nullptr;
            GetProgramResourceiv = reinterpret_cast<PFNGLGETPROGRAMRESOURCEIVPROC>(proc("glGetProgramResourceiv")); ok &= GetProgramResourceiv != nullptr;
            GetProgramResourceName = reinterpret_cast<PFNGLGETPROGRAMRESOURCENAMEPROC>(proc("glGetProgramResourceName")); ok &= GetProgramResourceName != nullptr;
            GetProgramResourceIndex = reinterpret_cast<PFNGLGETPROGRAMRESOURCEINDEXPROC>(proc("glGetProgramResourceIndex")); ok &= GetProgramResourceIndex != nullptr;
            GetUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(proc("glGetUniformLocation")); ok &= GetUniformLocation != nullptr;
            GetAttribLocation = reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(proc("glGetAttribLocation")); ok &= GetAttribLocation != nullptr;
            GetActiveAttrib = reinterpret_cast<PFNGLGETACTIVEATTRIBPROC>(proc("glGetActiveAttrib")); ok &= GetActiveAttrib != nullptr;
            CreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(proc("glCreateShader")); ok &= CreateShader != nullptr;
            ShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(proc("glShaderSource")); ok &= ShaderSource != nullptr;
            CompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(proc("glCompileShader")); ok &= CompileShader != nullptr;
            GetShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(proc("glGetShaderInfoLog")); ok &= GetShaderInfoLog != nullptr;
            DeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(proc("glDeleteShader")); ok &= DeleteShader != nullptr;
            CreateProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(proc("glCreateProgram")); ok &= CreateProgram != nullptr;
            AttachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(proc("glAttachShader")); ok &= AttachShader != nullptr;
            DetachShader = reinterpret_cast<PFNGLDETACHSHADERPROC>(proc("glDetachShader")); ok &= DetachShader != nullptr;
            BindAttribLocation = reinterpret_cast<PFNGLBINDATTRIBLOCATIONPROC>(proc("glBindAttribLocation")); ok &= BindAttribLocation != nullptr;
            LinkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(proc("glLinkProgram")); ok &= LinkProgram != nullptr;
            GetProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(proc("glGetProgramInfoLog")); ok &= GetProgramInfoLog != nullptr;
            DeleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(proc("glDeleteProgram")); ok &= DeleteProgram != nullptr;
            UseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(proc("glUseProgram")); ok &= UseProgram != nullptr;
            GetUniformfv = reinterpret_cast<PFNGLGETUNIFORMFVPROC>(proc("glGetUniformfv")); ok &= GetUniformfv != nullptr;
            GetUniformiv = reinterpret_cast<PFNGLGETUNIFORMIVPROC>(proc("glGetUniformiv")); ok &= GetUniformiv != nullptr;
            GetUniformuiv = reinterpret_cast<PFNGLGETUNIFORMUIVPROC>(proc("glGetUniformuiv")); ok &= GetUniformuiv != nullptr;
            GetUniformdv = reinterpret_cast<PFNGLGETUNIFORMDVPROC>(proc("glGetUniformdv")); ok &= GetUniformdv != nullptr;
            GetActiveUniformBlockiv = reinterpret_cast<PFNGLGETACTIVEUNIFORMBLOCKIVPROC>(proc("glGetActiveUniformBlockiv")); ok &= GetActiveUniformBlockiv != nullptr;
            UniformBlockBinding = reinterpret_cast<PFNGLUNIFORMBLOCKBINDINGPROC>(proc("glUniformBlockBinding")); ok &= UniformBlockBinding != nullptr;
            BindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(proc("glBindBuffer")); ok &= BindBuffer != nullptr;
            ProgramUniform1fv = reinterpret_cast<PFNGLPROGRAMUNIFORM1FVPROC>(proc("glProgramUniform1fv")); ok &= ProgramUniform1fv != nullptr;
            ProgramUniform2fv = reinterpret_cast<PFNGLPROGRAMUNIFORM2FVPROC>(proc("glProgramUniform2fv")); ok &= ProgramUniform2fv != nullptr;
            ProgramUniform3fv = reinterpret_cast<PFNGLPROGRAMUNIFORM3FVPROC>(proc("glProgramUniform3fv")); ok &= ProgramUniform3fv != nullptr;
            ProgramUniform4fv = reinterpret_cast<PFNGLPROGRAMUNIFORM4FVPROC>(proc("glProgramUniform4fv")); ok &= ProgramUniform4fv != nullptr;
            ProgramUniform1iv = reinterpret_cast<PFNGLPROGRAMUNIFORM1IVPROC>(proc("glProgramUniform1iv")); ok &= ProgramUniform1iv != nullptr;
            ProgramUniform2iv = reinterpret_cast<PFNGLPROGRAMUNIFORM2IVPROC>(proc("glProgramUniform2iv")); ok &= ProgramUniform2iv != nullptr;
            ProgramUniform3iv = reinterpret_cast<PFNGLPROGRAMUNIFORM3IVPROC>(proc("glProgramUniform3iv")); ok &= ProgramUniform3iv != nullptr;
            ProgramUniform4iv = reinterpret_cast<PFNGLPROGRAMUNIFORM4IVPROC>(proc("glProgramUniform4iv")); ok &= ProgramUniform4iv != nullptr;
            ProgramUniform1uiv = reinterpret_cast<PFNGLPROGRAMUNIFORM1UIVPROC>(proc("glProgramUniform1uiv")); ok &= ProgramUniform1uiv != nullptr;
            ProgramUniform2uiv = reinterpret_cast<PFNGLPROGRAMUNIFORM2UIVPROC>(proc("glProgramUniform2uiv")); ok &= ProgramUniform2uiv != nullptr;
            ProgramUniform3uiv = reinterpret_cast<PFNGLPROGRAMUNIFORM3UIVPROC>(proc("glProgramUniform3uiv")); ok &= ProgramUniform3uiv != nullptr;
            ProgramUniform4uiv = reinterpret_cast<PFNGLPROGRAMUNIFORM4UIVPROC>(proc("glProgramUniform4uiv")); ok &= ProgramUniform4uiv != nullptr;
            ProgramUniform1dv = reinterpret_cast<PFNGLPROGRAMUNIFORM1DVPROC>(proc("glProgramUniform1dv")); ok &= ProgramUniform1dv != nullptr;
            ProgramUniform2dv = reinterpret_cast<PFNGLPROGRAMUNIFORM2DVPROC>(proc("glProgramUniform2dv")); ok &= ProgramUniform2dv != nullptr;
            ProgramUniform3dv = reinterpret_cast<PFNGLPROGRAMUNIFORM3DVPROC>(proc("glProgramUniform3dv")); ok &= ProgramUniform3dv != nullptr;
            ProgramUniform4dv = reinterpret_cast<PFNGLPROGRAMUNIFORM4DVPROC>(proc("glProgramUniform4dv")); ok &= ProgramUniform4dv != nullptr;
            ProgramUniformMatrix2fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX2FVPROC>(proc("glProgramUniformMatrix2fv")); ok &= ProgramUniformMatrix2fv != nullptr;
            ProgramUniformMatrix3fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX3FVPROC>(proc("glProgramUniformMatrix3fv")); ok &= ProgramUniformMatrix3fv != nullptr;
            ProgramUniformMatrix4fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX4FVPROC>(proc("glProgramUniformMatrix4fv")); ok &= ProgramUniformMatrix4fv != nullptr;
            ProgramUniformMatrix2x3fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC>(proc("glProgramUniformMatrix2x3fv")); ok &= ProgramUniformMatrix2x3fv != nullptr;
            ProgramUniformMatrix3x2fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC>(proc("glProgramUniformMatrix3x2fv")); ok &= ProgramUniformMatrix3x2fv != nullptr;
            ProgramUniformMatrix2x4fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC>(proc("glProgramUniformMatrix2x4fv")); ok &= ProgramUniformMatrix2x4fv != nullptr;
            ProgramUniformMatrix4x2fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC>(proc("glProgramUniformMatrix4x2fv")); ok &= ProgramUniformMatrix4x2fv != nullptr;
            ProgramUniformMatrix3x4fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC>(proc("glProgramUniformMatrix3x4fv")); ok &= ProgramUniformMatrix3x4fv != nullptr;
            ProgramUniformMatrix4x3fv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC>(proc("glProgramUniformMatrix4x3fv")); ok &= ProgramUniformMatrix4x3fv != nullptr;
            ProgramUniformMatrix2dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX2DVPROC>(proc("glProgramUniformMatrix2dv")); ok &= ProgramUniformMatrix2dv != nullptr;
            ProgramUniformMatrix3dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX3DVPROC>(proc("glProgramUniformMatrix3dv")); ok &= ProgramUniformMatrix3dv != nullptr;
            ProgramUniformMatrix4dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX4DVPROC>(proc("glProgramUniformMatrix4dv")); ok &= ProgramUniformMatrix4dv != nullptr;
            ProgramUniformMatrix2x3dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC>(proc("glProgramUniformMatrix2x3dv")); ok &= ProgramUniformMatrix2x3dv != nullptr;
            ProgramUniformMatrix3x2dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC>(proc("glProgramUniformMatrix3x2dv")); ok &= ProgramUniformMatrix3x2dv != nullptr;
            ProgramUniformMatrix2x4dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC>(proc("glProgramUniformMatrix2x4dv")); ok &= ProgramUniformMatrix2x4dv != nullptr;
            ProgramUniformMatrix4x2dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC>(proc("glProgramUniformMatrix4x2dv")); ok &= ProgramUniformMatrix4x2dv != nullptr;
            ProgramUniformMatrix3x4dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC>(proc("glProgramUniformMatrix3x4dv")); ok &= ProgramUniformMatrix3x4dv != nullptr;
            ProgramUniformMatrix4x3dv = reinterpret_cast<PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC>(proc("glProgramUniformMatrix4x3dv")); ok &= ProgramUniformMatrix4x3dv != nullptr;
            return ok;
        }
    };
    struct Source { GLenum type; std::string code; };
    struct Program { std::vector<Source> sources; uint64_t generation = 0; };
    struct Uniform { GLint from, to; GLenum type; };
    struct Block { GLuint from, to; };
    struct Variant { GLuint program = 0; std::vector<Uniform> uniforms; std::vector<Block> blocks; const char* reason = "OpenGL replacement could not be linked; original pass retained."; };
    struct Cache { uint64_t generation = 0; std::map<std::array<uint32_t,4>, Variant> variants; };
    struct __declspec(uuid("BBAB4702-07E4-4F86-95B8-7644227D733E")) DeviceData
    {
        std::recursive_mutex mutex;
        std::map<uint64_t, Program> programs;
        std::map<uint32_t, Status> statuses;
        std::map<uint32_t, Method> suggestions;
        uint64_t generation = 0;
        size_t sourceBytes = 0;
    };
    struct __declspec(uuid("99369045-17E0-4CAE-8EED-E0B7B02DDAC4")) Context
    {
        Functions f;
        bool tried = false, ready = false, scopesTried = false;
        HGLRC native = nullptr;
        size_t attempts = 0;
        std::map<uint64_t, Cache> cache;
    };
    void report(DeviceData& d, uint32_t hash, Choice choice, bool applied, const char* reason)
    {
        if (!d.statuses.count(hash) && d.statuses.size() >= 4096) return;
        auto& old = d.statuses[hash];
        if (!old.attempted || old.choice != choice || old.applied != applied || old.reason != reason)
            diagnostics::Write("[SMART] OpenGL shader=%08X method=%s applied=%u: %s", hash, name(choice.method), applied, reason);
        old = {choice, true, applied, reason};
    }
    void clear(Context& c, Cache& cache)
    {
        Internal own;
        for (auto& [key, v] : cache.variants) if (v.program) c.f.DeleteProgram(v.program);
        cache.variants.clear();
    }
    bool context(Context& c)
    {
        if (!c.tried)
        {
            c.native=wglGetCurrentContext();if(!c.native)return false;
            c.tried=true;c.ready=c.f.load();
            if(c.ready)
            {
                const char* version=reinterpret_cast<const char*>(c.f.GetString(GL_VERSION));
                int major=0,minor=0;
                if(!version)c.ready=false;
                else
                {
                    const char* end=version+std::strlen(version);
                    auto a=std::from_chars(version,end,major);
                    auto b=a.ec==std::errc{}&&a.ptr<end&&*a.ptr=='.'?std::from_chars(a.ptr+1,end,minor):a;
                    c.ready=a.ec==std::errc{}&&b.ec==std::errc{}&&(major>4||(major==4&&minor>=3));
                }
            }
        }
        return c.ready && c.native == wglGetCurrentContext();
    }
    std::string resourceName(Functions& f, GLuint p, GLenum kind, GLuint index)
    {
        GLenum prop = GL_NAME_LENGTH; GLint length = 0;
        f.GetProgramResourceiv(p,kind,index,1,&prop,1,nullptr,&length);
        if (length <= 1 || length > 65536) return {};
        std::string n(static_cast<size_t>(length), '\0'); GLsizei actual = 0;
        f.GetProgramResourceName(p,kind,index,length,&actual,n.data());
        n.resize(actual > 0 ? static_cast<size_t>(actual) : 0); return n;
    }
    bool sampler(GLenum t)
    {
        switch (t) {
        case GL_SAMPLER_1D:
        case GL_SAMPLER_2D:
        case GL_SAMPLER_3D:
        case GL_SAMPLER_CUBE:
        case GL_SAMPLER_1D_SHADOW:
        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_1D_ARRAY:
        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_1D_ARRAY_SHADOW:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
        case GL_SAMPLER_2D_MULTISAMPLE:
        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_SAMPLER_CUBE_SHADOW:
        case GL_SAMPLER_BUFFER:
        case GL_SAMPLER_2D_RECT:
        case GL_SAMPLER_2D_RECT_SHADOW:
        case GL_SAMPLER_CUBE_MAP_ARRAY:
        case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
        case GL_INT_SAMPLER_1D:
        case GL_INT_SAMPLER_2D:
        case GL_INT_SAMPLER_3D:
        case GL_INT_SAMPLER_CUBE:
        case GL_INT_SAMPLER_1D_ARRAY:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_INT_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_2D_RECT:
        case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_1D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
        case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
            return true; default: return false;
        }
    }
    bool uniformType(GLenum t)
    {
        if (sampler(t)) return true;
        switch (t) {
        case GL_FLOAT:
        case GL_FLOAT_VEC2:
        case GL_FLOAT_VEC3:
        case GL_FLOAT_VEC4:
        case GL_INT:
        case GL_INT_VEC2:
        case GL_INT_VEC3:
        case GL_INT_VEC4:
        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_INT_VEC2:
        case GL_UNSIGNED_INT_VEC3:
        case GL_UNSIGNED_INT_VEC4:
        case GL_BOOL:
        case GL_BOOL_VEC2:
        case GL_BOOL_VEC3:
        case GL_BOOL_VEC4:
        case GL_DOUBLE:
        case GL_DOUBLE_VEC2:
        case GL_DOUBLE_VEC3:
        case GL_DOUBLE_VEC4:
        case GL_FLOAT_MAT2:
        case GL_FLOAT_MAT3:
        case GL_FLOAT_MAT4:
        case GL_FLOAT_MAT2x3:
        case GL_FLOAT_MAT3x2:
        case GL_FLOAT_MAT2x4:
        case GL_FLOAT_MAT4x2:
        case GL_FLOAT_MAT3x4:
        case GL_FLOAT_MAT4x3:
        case GL_DOUBLE_MAT2:
        case GL_DOUBLE_MAT3:
        case GL_DOUBLE_MAT4:
        case GL_DOUBLE_MAT2x3:
        case GL_DOUBLE_MAT3x2:
        case GL_DOUBLE_MAT2x4:
        case GL_DOUBLE_MAT4x2:
        case GL_DOUBLE_MAT3x4:
        case GL_DOUBLE_MAT4x3:
            return true; default: return false;
        }
    }
    void copyUniform(Functions& f, GLuint original, GLuint replacement, const Uniform& u)
    {
        if (sampler(u.type)) { GLint value=0; f.GetUniformiv(original,u.from,&value); f.ProgramUniform1iv(replacement,u.to,1,&value); return; }
        GLfloat fv[16]{}; GLint iv[4]{}; GLuint uv[4]{}; GLdouble dv[16]{};
        switch (u.type) {
        case GL_FLOAT: f.GetUniformfv(original,u.from,fv); f.ProgramUniform1fv(replacement,u.to,1,fv); break;
        case GL_FLOAT_VEC2: f.GetUniformfv(original,u.from,fv); f.ProgramUniform2fv(replacement,u.to,1,fv); break;
        case GL_FLOAT_VEC3: f.GetUniformfv(original,u.from,fv); f.ProgramUniform3fv(replacement,u.to,1,fv); break;
        case GL_FLOAT_VEC4: f.GetUniformfv(original,u.from,fv); f.ProgramUniform4fv(replacement,u.to,1,fv); break;
        case GL_INT: f.GetUniformiv(original,u.from,iv); f.ProgramUniform1iv(replacement,u.to,1,iv); break;
        case GL_INT_VEC2: f.GetUniformiv(original,u.from,iv); f.ProgramUniform2iv(replacement,u.to,1,iv); break;
        case GL_INT_VEC3: f.GetUniformiv(original,u.from,iv); f.ProgramUniform3iv(replacement,u.to,1,iv); break;
        case GL_INT_VEC4: f.GetUniformiv(original,u.from,iv); f.ProgramUniform4iv(replacement,u.to,1,iv); break;
        case GL_BOOL: f.GetUniformiv(original,u.from,iv); f.ProgramUniform1iv(replacement,u.to,1,iv); break;
        case GL_BOOL_VEC2: f.GetUniformiv(original,u.from,iv); f.ProgramUniform2iv(replacement,u.to,1,iv); break;
        case GL_BOOL_VEC3: f.GetUniformiv(original,u.from,iv); f.ProgramUniform3iv(replacement,u.to,1,iv); break;
        case GL_BOOL_VEC4: f.GetUniformiv(original,u.from,iv); f.ProgramUniform4iv(replacement,u.to,1,iv); break;
        case GL_UNSIGNED_INT: f.GetUniformuiv(original,u.from,uv); f.ProgramUniform1uiv(replacement,u.to,1,uv); break;
        case GL_UNSIGNED_INT_VEC2: f.GetUniformuiv(original,u.from,uv); f.ProgramUniform2uiv(replacement,u.to,1,uv); break;
        case GL_UNSIGNED_INT_VEC3: f.GetUniformuiv(original,u.from,uv); f.ProgramUniform3uiv(replacement,u.to,1,uv); break;
        case GL_UNSIGNED_INT_VEC4: f.GetUniformuiv(original,u.from,uv); f.ProgramUniform4uiv(replacement,u.to,1,uv); break;
        case GL_DOUBLE: f.GetUniformdv(original,u.from,dv); f.ProgramUniform1dv(replacement,u.to,1,dv); break;
        case GL_DOUBLE_VEC2: f.GetUniformdv(original,u.from,dv); f.ProgramUniform2dv(replacement,u.to,1,dv); break;
        case GL_DOUBLE_VEC3: f.GetUniformdv(original,u.from,dv); f.ProgramUniform3dv(replacement,u.to,1,dv); break;
        case GL_DOUBLE_VEC4: f.GetUniformdv(original,u.from,dv); f.ProgramUniform4dv(replacement,u.to,1,dv); break;
        case GL_FLOAT_MAT2: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix2fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_FLOAT_MAT3: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix3fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_FLOAT_MAT4: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix4fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_FLOAT_MAT2x3: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix2x3fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_FLOAT_MAT3x2: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix3x2fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_FLOAT_MAT2x4: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix2x4fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_FLOAT_MAT4x2: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix4x2fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_FLOAT_MAT3x4: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix3x4fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_FLOAT_MAT4x3: f.GetUniformfv(original,u.from,fv); f.ProgramUniformMatrix4x3fv(replacement,u.to,1,GL_FALSE,fv); break;
        case GL_DOUBLE_MAT2: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix2dv(replacement,u.to,1,GL_FALSE,dv); break;
        case GL_DOUBLE_MAT3: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix3dv(replacement,u.to,1,GL_FALSE,dv); break;
        case GL_DOUBLE_MAT4: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix4dv(replacement,u.to,1,GL_FALSE,dv); break;
        case GL_DOUBLE_MAT2x3: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix2x3dv(replacement,u.to,1,GL_FALSE,dv); break;
        case GL_DOUBLE_MAT3x2: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix3x2dv(replacement,u.to,1,GL_FALSE,dv); break;
        case GL_DOUBLE_MAT2x4: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix2x4dv(replacement,u.to,1,GL_FALSE,dv); break;
        case GL_DOUBLE_MAT4x2: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix4x2dv(replacement,u.to,1,GL_FALSE,dv); break;
        case GL_DOUBLE_MAT3x4: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix3x4dv(replacement,u.to,1,GL_FALSE,dv); break;
        case GL_DOUBLE_MAT4x3: f.GetUniformdv(original,u.from,dv); f.ProgramUniformMatrix4x3dv(replacement,u.to,1,GL_FALSE,dv); break;
        }
    }
    bool inspect(Functions& f, GLuint program)
    {
        GLint count = 0;
        for (GLenum stage : {GL_VERTEX_SHADER,GL_TESS_CONTROL_SHADER,GL_TESS_EVALUATION_SHADER,GL_GEOMETRY_SHADER,GL_FRAGMENT_SHADER})
        { f.GetProgramStageiv(program,stage,GL_ACTIVE_SUBROUTINE_UNIFORMS,&count); if(count) return false; }
        f.GetProgramInterfaceiv(program,GL_PROGRAM_OUTPUT,GL_ACTIVE_RESOURCES,&count);
        if (count != 1) return false;
        const GLenum props[]{GL_TYPE, GL_LOCATION, GL_ARRAY_SIZE, GL_LOCATION_INDEX}; GLint values[4]{};
        f.GetProgramResourceiv(program,GL_PROGRAM_OUTPUT,0,4,props,4,nullptr,values);
        if (values[1] != 0 || values[2] != 1 || values[3] != 0 ||
            (values[0] != GL_FLOAT && values[0] != GL_FLOAT_VEC2 && values[0] != GL_FLOAT_VEC3 && values[0] != GL_FLOAT_VEC4)) return false;
        for (GLenum kind : {GL_SHADER_STORAGE_BLOCK, GL_ATOMIC_COUNTER_BUFFER})
        { f.GetProgramInterfaceiv(program,kind,GL_ACTIVE_RESOURCES,&count); if (count) return false; }
        f.GetProgramInterfaceiv(program,GL_UNIFORM,GL_ACTIVE_RESOURCES,&count);
        if (count < 0 || count > 4096) return false;
        for (GLint i=0; i<count; ++i)
        {
            const GLenum p[]{GL_REFERENCED_BY_FRAGMENT_SHADER,GL_TYPE}; GLint v[2]{};
            f.GetProgramResourceiv(program,GL_UNIFORM,i,2,p,2,nullptr,v);
            if (v[0] && !uniformType(v[1])) return false;
        }
        f.GetProgramiv(program,GL_TRANSFORM_FEEDBACK_VARYINGS,&count);
        return count == 0;
    }
    bool mapUniforms(Functions& f, GLuint from, Variant& v)
    {
        GLint count = 0; f.GetProgramInterfaceiv(v.program,GL_UNIFORM,GL_ACTIVE_RESOURCES,&count);
        if (count < 0 || count > 4096) return false;
        const GLenum props[]{GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION,GL_BLOCK_INDEX,GL_OFFSET,GL_ARRAY_STRIDE,GL_MATRIX_STRIDE,GL_IS_ROW_MAJOR};
        for (GLint i=0; i<count; ++i)
        {
            const auto n = resourceName(f,v.program,GL_UNIFORM,i); if (n.empty()) return false;
            GLint a[8]{}, b[8]{};
            const auto index = f.GetProgramResourceIndex(from,GL_UNIFORM,n.c_str());
            if (index == GL_INVALID_INDEX) return false;
            f.GetProgramResourceiv(v.program,GL_UNIFORM,i,8,props,8,nullptr,a);
            f.GetProgramResourceiv(from,GL_UNIFORM,index,8,props,8,nullptr,b);
            if (a[0] != b[0] || a[1] > b[1] || a[1] < 1 || a[1] > 4096 || !uniformType(a[0])) return false;
            if (a[3] >= 0)
            {
                if (b[3] < 0 || !std::equal(a+4,a+8,b+4)) return false;
                continue;
            }
            if (a[2] < 0) return false;
            for (GLint element=0; element<a[1]; ++element)
            {
                std::string elementName=n;
                if (a[1] > 1)
                {
                    const auto zero=n.find("[0]");
                    if (zero == n.npos || n.find("[0]",zero+3) != n.npos) return false;
                    elementName.replace(zero,3,"["+std::to_string(element)+"]");
                }
                const auto source=f.GetUniformLocation(from,elementName.c_str());
                const auto dest=f.GetUniformLocation(v.program,elementName.c_str());
                if (source<0 || dest<0 || v.uniforms.size()>=16384) return false;
                v.uniforms.push_back({source,dest,static_cast<GLenum>(a[0])});
            }
        }
        f.GetProgramInterfaceiv(v.program,GL_UNIFORM_BLOCK,GL_ACTIVE_RESOURCES,&count);
        if (count<0 || count>256) return false;
        for (GLint i=0; i<count; ++i)
        {
            const auto n=resourceName(f,v.program,GL_UNIFORM_BLOCK,i);
            const auto index=f.GetProgramResourceIndex(from,GL_UNIFORM_BLOCK,n.c_str());
            if (index==GL_INVALID_INDEX) return false;
            v.blocks.push_back({index,static_cast<GLuint>(i)});
        }
        return true;
    }
    Variant create(Context& c, GLuint original, const Program& snapshot, Choice choice)
    {
        Internal own; auto& f=c.f; Variant result;
        for (const auto& source:snapshot.sources)
            if (source.code.find("bindless_texture") != std::string::npos)
            { result.reason="OpenGL bindless uniforms are unsupported; original pass retained."; return result; }
        if (!inspect(f,original)) { result.reason="OpenGL shader outputs or side effects are unsupported; original pass retained."; return result; }
        const auto bits=modern::colourKey(choice);
        const std::string colour="#version 430 core\nlayout(location=0) out vec4 staColour;\nvoid main(){staColour=uintBitsToFloat(uvec4("+
            std::to_string(bits[0])+"u,"+std::to_string(bits[1])+"u,"+std::to_string(bits[2])+"u,"+std::to_string(bits[3])+"u));}\n";
        std::vector<Source> sources=snapshot.sources; sources.push_back({GL_FRAGMENT_SHADER,colour});
        GLuint program=f.CreateProgram(); if (!program) return result;
        std::vector<GLuint> shaders;
        struct Cleanup
        {
            Functions& f; GLuint& p; std::vector<GLuint>& shaders;
            ~Cleanup() { for(auto s:shaders) { if(p) f.DetachShader(p,s); f.DeleteShader(s); } if(p) f.DeleteProgram(p); }
        } cleanup{f,program,shaders};
        for (const auto& source:sources)
        {
            GLuint shader=f.CreateShader(source.type); if (!shader) return result;
            const char* code=source.code.c_str(); GLint size=static_cast<GLint>(source.code.size());
            while(size>0 && code[size-1]=='\0') --size;
            f.ShaderSource(shader,1,&code,&size); f.CompileShader(shader);
            GLint ok=0; f.GetShaderiv(shader,GL_COMPILE_STATUS,&ok);
            if (!ok)
            {
                char error[512]{}; f.GetShaderInfoLog(shader,sizeof(error),nullptr,error);
                diagnostics::Write("[ERROR] SMART OpenGL shader compilation failed: %s",error);
                f.DeleteShader(shader); return result;
            }
            f.AttachShader(program,shader); shaders.push_back(shader);
        }
        GLint attributes=0, nameLength=0;
        f.GetProgramiv(original,GL_ACTIVE_ATTRIBUTES,&attributes);
        f.GetProgramiv(original,GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,&nameLength);
        if (attributes<0 || attributes>128 || nameLength<0 || nameLength>65536) return result;
        std::vector<char> n(static_cast<size_t>(std::max(nameLength,1)));
        for (GLint i=0;i<attributes;++i)
        {
            GLint size=0; GLenum type=0; f.GetActiveAttrib(original,i,static_cast<GLsizei>(n.size()),nullptr,&size,&type,n.data());
            const auto location=f.GetAttribLocation(original,n.data());
            if (location>=0) f.BindAttribLocation(program,location,n.data());
        }
        f.LinkProgram(program); GLint linked=0; f.GetProgramiv(program,GL_LINK_STATUS,&linked);
        if (!linked)
        {
            char error[512]{}; f.GetProgramInfoLog(program,sizeof(error),nullptr,error);
            diagnostics::Write("[ERROR] SMART OpenGL program link failed: %s",error); return result;
        }
        result.program=program;
        if (!mapUniforms(f,original,result))
        {
            result.program=0; result.uniforms.clear(); result.blocks.clear();
            result.reason="OpenGL uniform layout could not be preserved; original pass retained."; return result;
        }
        for(auto s:shaders) { f.DetachShader(program,s); f.DeleteShader(s); } shaders.clear();
        program=0; result.reason="Colour replacement is running."; return result;
    }
    void initDevice(device* dev) { if(isGL(dev)) { dev->create_private_data<DeviceData>(); diagnostics::Write("[SMART] OpenGL colour replacement initialized (GL 4.3+ linked graphics programs)."); } }
    void destroyDevice(device* dev) { if(isGL(dev)) dev->destroy_private_data<DeviceData>(); }
    void registerNative(command_list*);
    void unregisterNative(command_list*);
    void initCommands(command_list* cmd) { if(isGL(cmd->get_device())) { cmd->create_private_data<Context>(); registerNative(cmd); } }
    void destroyCommands(command_list* cmd)
    {
        if(!isGL(cmd->get_device())) return;
        unregisterNative(cmd);
        auto& c=cmd->get_private_data<Context>();
        if(c.ready && c.native==wglGetCurrentContext()) for(auto& [p,cache]:c.cache) clear(c,cache);
        cmd->destroy_private_data<Context>();
    }
    void forget(device* dev,uint64_t handle)
    {
        if(!isGL(dev) || internal()) return;
        auto& d=dev->get_private_data<DeviceData>(); std::lock_guard lock(d.mutex);
        if(auto it=d.programs.find(handle);it!=d.programs.end())
        { for(auto& s:it->second.sources) d.sourceBytes-=s.code.size(); d.programs.erase(it); }
    }
    void capture(device* dev,uint64_t handle,uint32_t count,const reshade::api::pipeline_subobject* subobjects)
    {
        if(!isGL(dev) || internal()) return;
        forget(dev,handle);
        auto& d=dev->get_private_data<DeviceData>(); std::lock_guard lock(d.mutex);
        if(d.programs.size()>=32768) return;
        Program p; size_t bytes=0; bool vertex=false, fragment=false;
        for(uint32_t i=0;i<count;++i)
        {
            using S=reshade::api::pipeline_subobject_type;
            const auto& object=subobjects[i]; GLenum type=0;
            switch(object.type) {
            case S::vertex_shader:type=GL_VERTEX_SHADER;vertex=true;break;
            case S::hull_shader:type=GL_TESS_CONTROL_SHADER;break;
            case S::domain_shader:type=GL_TESS_EVALUATION_SHADER;break;
            case S::geometry_shader:type=GL_GEOMETRY_SHADER;break;
            case S::pixel_shader:fragment=true;continue;
            case S::compute_shader:return;
            default:continue;
            }
            if(object.count!=1 || !object.data) return;
            const auto& shader=*static_cast<const reshade::api::shader_desc*>(object.data);
            if(!shader.code || !shader.code_size || shader.code_size>4*1024*1024 || p.sources.size()>=16) return;
            bytes+=shader.code_size;
            p.sources.push_back({type,std::string(static_cast<const char*>(shader.code),shader.code_size)});
        }
        if(!vertex || !fragment || d.sourceBytes+bytes>128*1024*1024) return;
        p.generation=++d.generation; d.sourceBytes+=bytes; d.programs[handle]=std::move(p);
    }
    bool begin(command_list* cmd,uint64_t handle,uint32_t hash,Choice choice,GLDraw& token)
    {
        if(!cmd || !isGL(cmd->get_device()) || internal() || !valid(choice)) return false;
        auto& d=cmd->get_device()->get_private_data<DeviceData>(); std::lock_guard lock(d.mutex);
        auto& c=cmd->get_private_data<Context>();
        auto fail=[&](const char* why){report(d,hash,choice,false,why);return false;};
        if(!context(c)) return fail("OpenGL 4.3 shader reflection functions are unavailable; original pass retained.");
        auto& f=c.f; GLint current=0; f.GetIntegerv(GL_CURRENT_PROGRAM,&current);
        if(!current || static_cast<GLuint>(current)!=static_cast<GLuint>(handle))
            return fail("This OpenGL pass needs a known linked graphics program; original pass retained.");
        GLint linked=0; f.GetProgramiv(current,GL_LINK_STATUS,&linked);
        if(!linked) return fail("OpenGL program is not successfully linked; original pass retained.");
        GLboolean feedback=GL_FALSE; f.GetBooleanv(GL_TRANSFORM_FEEDBACK_ACTIVE,&feedback);
        if(feedback || f.IsEnabled(GL_RASTERIZER_DISCARD)) return fail("Transform feedback or rasterizer discard is active; original pass retained.");
        GLint buffers=0;f.GetIntegerv(GL_MAX_DRAW_BUFFERS,&buffers);
        if(buffers<1 || buffers>32) return fail("OpenGL colour targets could not be inspected; original pass retained.");
        for(GLint i=0;i<buffers;++i)
        {
            GLint target=GL_NONE;f.GetIntegerv(GL_DRAW_BUFFER0+i,&target);
            if((i==0 && target==GL_NONE)||(i!=0 && target!=GL_NONE))
                return fail("This pass needs one colour target at output zero; original pass retained.");
        }
        const auto snapshot=d.programs.find(handle);
        if(snapshot==d.programs.end()) return fail("OpenGL shader source was not captured; original pass retained.");
        if(!c.cache.count(handle) && c.attempts>=512) return fail("OpenGL replacement cache is full; original pass retained.");
        auto& cache=c.cache[handle];
        if(cache.generation!=snapshot->second.generation) {clear(c,cache);cache.generation=snapshot->second.generation;}
        const auto key=modern::colourKey(choice); auto found=cache.variants.find(key);
        if(found==cache.variants.end())
        {
            if(c.attempts>=512) return fail("OpenGL replacement cache is full; original pass retained.");
            ++c.attempts; found=cache.variants.emplace(key,create(c,static_cast<GLuint>(current),snapshot->second,choice)).first;
        }
        auto& v=found->second; if(!v.program) return fail(v.reason);
        Internal own;
        for(const auto& u:v.uniforms) copyUniform(f,current,v.program,u);
        for(const auto& b:v.blocks)
        { GLint binding=0;f.GetActiveUniformBlockiv(current,b.from,GL_UNIFORM_BLOCK_BINDING,&binding);f.UniformBlockBinding(v.program,b.to,binding); }
        GLint indirect=0;f.GetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING,&indirect);
        token={cmd,static_cast<GLuint>(current),static_cast<GLuint>(indirect)};
        f.UseProgram(v.program);report(d,hash,choice,true,v.reason);return true;
    }
    void end(GLDraw& token)
    {
        if(!token.commands) return;
        Internal own;auto& c=token.commands->get_private_data<Context>();
        c.f.UseProgram(token.original);c.f.BindBuffer(GL_DRAW_INDIRECT_BUFFER,token.indirectBuffer);
        token.commands=nullptr;
    }
    void observe(command_list* cmd,uint64_t,uint32_t hash)
    {
        auto& d=cmd->get_device()->get_private_data<DeviceData>(); std::lock_guard lock(d.mutex);
        if(d.suggestions.count(hash)||d.suggestions.size()>=4096) return;
        auto& c=cmd->get_private_data<Context>();if(!context(c)) return;
        GLint src=0,dest=0,op=0;
        c.f.GetIntegerv(GL_BLEND_SRC_RGB,&src);c.f.GetIntegerv(GL_BLEND_DST_RGB,&dest);c.f.GetIntegerv(GL_BLEND_EQUATION_RGB,&op);
        Method choice=Method::TransparentBlack;
        if(c.f.IsEnabled(GL_BLEND) && op==GL_FUNC_ADD && (src==GL_DST_COLOR || dest==GL_SRC_COLOR)) choice=Method::White;
        d.suggestions[hash]=choice;
    }
    Status status(device* dev,uint32_t hash) { auto& d=dev->get_private_data<DeviceData>();std::lock_guard lock(d.mutex);auto i=d.statuses.find(hash);return i==d.statuses.end()?Status{}:i->second; }
    Method suggestion(device* dev,uint32_t hash) { auto& d=dev->get_private_data<DeviceData>();std::lock_guard lock(d.mutex);auto i=d.suggestions.find(hash);return i==d.suggestions.end()?Method::TransparentBlack:i->second; }
    std::recursive_mutex nativeMutex;
    std::map<HGLRC,command_list*> nativeContexts;
    std::map<void*,void*> nativeOriginals;
    std::vector<HMODULE> nativeModules;
    struct NativeDraw;
    thread_local NativeDraw* currentDraw=nullptr;
    struct NativeDraw
    {
        NativeDraw* previous=currentDraw;
        command_list* commands=nullptr;
        GLDraw token;
        uint32_t hash=0;Choice choice;
        bool single=true;
        NativeDraw(bool singleCall=true):single(singleCall)
        {
            std::lock_guard lock(nativeMutex);
            auto i=nativeContexts.find(wglGetCurrentContext());
            if(i!=nativeContexts.end()) commands=i->second;
            currentDraw=this;
        }
        ~NativeDraw(){end(token);currentDraw=previous;}
    };
    bool safeDraw(command_list* cmd)
    {
        bool found=false;
        for(auto* scope=currentDraw;scope;scope=scope->previous)
            if(scope->commands==cmd) {if(!scope->single)return false;found=true;}
        return found;
    }
    PROC drawProc(const char* name)
    {
        auto p=wglGetProcAddress(name);
        if(!p||p==reinterpret_cast<PROC>(1)||p==reinterpret_cast<PROC>(2)||p==reinterpret_cast<PROC>(3)||p==reinterpret_cast<PROC>(-1))
            p=reinterpret_cast<PROC>(GetProcAddress(GetModuleHandleW(L"opengl32.dll"),name));
        return p;
    }
    template<class T> T originalDraw(const char* name)
    {
        auto* address=reinterpret_cast<void*>(drawProc(name));std::lock_guard lock(nativeMutex);
        auto i=nativeOriginals.find(address);
        return reinterpret_cast<T>(i==nativeOriginals.end()?address:i->second);
    }
    void APIENTRY nativeDrawArrays(GLenum mode,GLint first,GLsizei count)
    {
        const auto original=originalDraw<PFNGLDRAWARRAYSPROC>("glDrawArrays");
        if(!original)return;
        NativeDraw scope;original(mode,first,count);
    }
    void APIENTRY nativeDrawElements(GLenum mode,GLsizei count,GLenum type,const void* indices)
    {
        const auto original=originalDraw<PFNGLDRAWELEMENTSPROC>("glDrawElements");
        if(!original)return;
        NativeDraw scope;original(mode,count,type,indices);
    }
    void APIENTRY nativeDrawRangeElements(GLenum mode,GLuint start,GLuint end,GLsizei count,GLenum type,const void* indices)
    {
        const auto original=originalDraw<PFNGLDRAWRANGEELEMENTSPROC>("glDrawRangeElements");
        if(!original)return;
        NativeDraw scope;original(mode,start,end,count,type,indices);
    }
    void APIENTRY nativeDrawArraysInstanced(GLenum mode,GLint first,GLsizei count,GLsizei instances)
    {
        const auto original=originalDraw<PFNGLDRAWARRAYSINSTANCEDPROC>("glDrawArraysInstanced");
        if(!original)return;
        NativeDraw scope;original(mode,first,count,instances);
    }
    void APIENTRY nativeDrawElementsInstanced(GLenum mode,GLsizei count,GLenum type,const void* indices,GLsizei instances)
    {
        const auto original=originalDraw<PFNGLDRAWELEMENTSINSTANCEDPROC>("glDrawElementsInstanced");
        if(!original)return;
        NativeDraw scope;original(mode,count,type,indices,instances);
    }
    void APIENTRY nativeDrawElementsBaseVertex(GLenum mode,GLsizei count,GLenum type,const void* indices,GLint base)
    {
        const auto original=originalDraw<PFNGLDRAWELEMENTSBASEVERTEXPROC>("glDrawElementsBaseVertex");
        if(!original)return;
        NativeDraw scope;original(mode,count,type,indices,base);
    }
    void APIENTRY nativeDrawRangeElementsBaseVertex(GLenum mode,GLuint start,GLuint end,GLsizei count,GLenum type,const void* indices,GLint base)
    {
        const auto original=originalDraw<PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC>("glDrawRangeElementsBaseVertex");
        if(!original)return;
        NativeDraw scope;original(mode,start,end,count,type,indices,base);
    }
    void APIENTRY nativeDrawElementsInstancedBaseVertex(GLenum mode,GLsizei count,GLenum type,const void* indices,GLsizei instances,GLint base)
    {
        const auto original=originalDraw<PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC>("glDrawElementsInstancedBaseVertex");
        if(!original)return;
        NativeDraw scope;original(mode,count,type,indices,instances,base);
    }
    void APIENTRY nativeDrawArraysInstancedBaseInstance(GLenum mode,GLint first,GLsizei count,GLsizei instances,GLuint baseInstance)
    {
        const auto original=originalDraw<PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC>("glDrawArraysInstancedBaseInstance");
        if(!original)return;
        NativeDraw scope;original(mode,first,count,instances,baseInstance);
    }
    void APIENTRY nativeDrawElementsInstancedBaseInstance(GLenum mode,GLsizei count,GLenum type,const void* indices,GLsizei instances,GLuint baseInstance)
    {
        const auto original=originalDraw<PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC>("glDrawElementsInstancedBaseInstance");
        if(!original)return;
        NativeDraw scope;original(mode,count,type,indices,instances,baseInstance);
    }
    void APIENTRY nativeDrawElementsInstancedBaseVertexBaseInstance(GLenum mode,GLsizei count,GLenum type,const void* indices,GLsizei instances,GLint base,GLuint baseInstance)
    {
        const auto original=originalDraw<PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC>("glDrawElementsInstancedBaseVertexBaseInstance");
        if(!original)return;
        NativeDraw scope;original(mode,count,type,indices,instances,base,baseInstance);
    }
    void APIENTRY nativeMultiDrawArrays(GLenum mode,const GLint* first,const GLsizei* count,GLsizei draws)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWARRAYSPROC>("glMultiDrawArrays");
        if(!original)return;
        NativeDraw scope(false);original(mode,first,count,draws);
    }
    void APIENTRY nativeMultiDrawElements(GLenum mode,const GLsizei* count,GLenum type,const void* const* indices,GLsizei draws)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWELEMENTSPROC>("glMultiDrawElements");
        if(!original)return;
        NativeDraw scope(false);original(mode,count,type,indices,draws);
    }
    void APIENTRY nativeMultiDrawElementsBaseVertex(GLenum mode,const GLsizei* count,GLenum type,const void* const* indices,GLsizei draws,const GLint* base)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC>("glMultiDrawElementsBaseVertex");
        if(!original)return;
        NativeDraw scope(false);original(mode,count,type,indices,draws,base);
    }
    void APIENTRY nativeDrawArraysIndirect(GLenum mode,const void* indirect)
    {
        const auto original=originalDraw<PFNGLDRAWARRAYSINDIRECTPROC>("glDrawArraysIndirect");
        if(!original)return;
        NativeDraw scope;original(mode,indirect);
    }
    void APIENTRY nativeDrawElementsIndirect(GLenum mode,GLenum type,const void* indirect)
    {
        const auto original=originalDraw<PFNGLDRAWELEMENTSINDIRECTPROC>("glDrawElementsIndirect");
        if(!original)return;
        NativeDraw scope;original(mode,type,indirect);
    }
    void APIENTRY nativeMultiDrawArraysIndirect(GLenum mode,const void* indirect,GLsizei count,GLsizei stride)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWARRAYSINDIRECTPROC>("glMultiDrawArraysIndirect");
        if(!original)return;
        NativeDraw scope(false);original(mode,indirect,count,stride);
    }
    void APIENTRY nativeMultiDrawElementsIndirect(GLenum mode,GLenum type,const void* indirect,GLsizei count,GLsizei stride)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWELEMENTSINDIRECTPROC>("glMultiDrawElementsIndirect");
        if(!original)return;
        NativeDraw scope(false);original(mode,type,indirect,count,stride);
    }
    void APIENTRY nativeMultiDrawArraysIndirectCount(GLenum mode,const void* indirect,GLintptr count,GLsizei maxCount,GLsizei stride)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC>("glMultiDrawArraysIndirectCount");
        if(!original)return;
        NativeDraw scope(false);original(mode,indirect,count,maxCount,stride);
    }
    void APIENTRY nativeMultiDrawElementsIndirectCount(GLenum mode,GLenum type,const void* indirect,GLintptr count,GLsizei maxCount,GLsizei stride)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC>("glMultiDrawElementsIndirectCount");
        if(!original)return;
        NativeDraw scope(false);original(mode,type,indirect,count,maxCount,stride);
    }
    void APIENTRY nativeMultiDrawArraysIndirectCountARB(GLenum mode,const void* indirect,GLintptr count,GLsizei maxCount,GLsizei stride)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWARRAYSINDIRECTCOUNTARBPROC>("glMultiDrawArraysIndirectCountARB");
        if(!original)return;
        NativeDraw scope(false);original(mode,indirect,count,maxCount,stride);
    }
    void APIENTRY nativeMultiDrawElementsIndirectCountARB(GLenum mode,GLenum type,const void* indirect,GLintptr count,GLsizei maxCount,GLsizei stride)
    {
        const auto original=originalDraw<PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTARBPROC>("glMultiDrawElementsIndirectCountARB");
        if(!original)return;
        NativeDraw scope(false);original(mode,type,indirect,count,maxCount,stride);
    }
    bool installNative(const char* name,void* replacement)
    {
        auto* target=reinterpret_cast<void*>(drawProc(name));if(!target)return false;
        if(nativeOriginals.count(target))return true;
        HMODULE module=nullptr;
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<const wchar_t*>(target),&module))return false;
        void* original=nullptr;auto result=MH_CreateHook(target,replacement,&original);
        if(result!=MH_OK){FreeLibrary(module);return false;}
        nativeOriginals[target]=original;
        result=MH_EnableHook(target);
        if(result!=MH_OK){MH_RemoveHook(target);nativeOriginals.erase(target);FreeLibrary(module);return false;}
        nativeModules.push_back(module);return true;
    }
    void registerNative(command_list* cmd)
    {
        auto& c=cmd->get_private_data<Context>();if(c.scopesTried || !context(c))return;
        c.scopesTried=true;
        std::lock_guard lock(nativeMutex);nativeContexts[c.native]=cmd;
        auto result=MH_Initialize();if(result!=MH_OK&&result!=MH_ERROR_ALREADY_INITIALIZED)return;
        unsigned installed=0;
        installed+=installNative("glDrawArrays",reinterpret_cast<void*>(&nativeDrawArrays));
        installed+=installNative("glDrawElements",reinterpret_cast<void*>(&nativeDrawElements));
        installed+=installNative("glDrawRangeElements",reinterpret_cast<void*>(&nativeDrawRangeElements));
        installed+=installNative("glDrawArraysInstanced",reinterpret_cast<void*>(&nativeDrawArraysInstanced));
        installed+=installNative("glDrawElementsInstanced",reinterpret_cast<void*>(&nativeDrawElementsInstanced));
        installed+=installNative("glDrawElementsBaseVertex",reinterpret_cast<void*>(&nativeDrawElementsBaseVertex));
        installed+=installNative("glDrawRangeElementsBaseVertex",reinterpret_cast<void*>(&nativeDrawRangeElementsBaseVertex));
        installed+=installNative("glDrawElementsInstancedBaseVertex",reinterpret_cast<void*>(&nativeDrawElementsInstancedBaseVertex));
        installed+=installNative("glDrawArraysInstancedBaseInstance",reinterpret_cast<void*>(&nativeDrawArraysInstancedBaseInstance));
        installed+=installNative("glDrawElementsInstancedBaseInstance",reinterpret_cast<void*>(&nativeDrawElementsInstancedBaseInstance));
        installed+=installNative("glDrawElementsInstancedBaseVertexBaseInstance",reinterpret_cast<void*>(&nativeDrawElementsInstancedBaseVertexBaseInstance));
        installed+=installNative("glMultiDrawArrays",reinterpret_cast<void*>(&nativeMultiDrawArrays));
        installed+=installNative("glMultiDrawElements",reinterpret_cast<void*>(&nativeMultiDrawElements));
        installed+=installNative("glMultiDrawElementsBaseVertex",reinterpret_cast<void*>(&nativeMultiDrawElementsBaseVertex));
        installed+=installNative("glDrawArraysIndirect",reinterpret_cast<void*>(&nativeDrawArraysIndirect));
        installed+=installNative("glDrawElementsIndirect",reinterpret_cast<void*>(&nativeDrawElementsIndirect));
        installed+=installNative("glMultiDrawArraysIndirect",reinterpret_cast<void*>(&nativeMultiDrawArraysIndirect));
        installed+=installNative("glMultiDrawElementsIndirect",reinterpret_cast<void*>(&nativeMultiDrawElementsIndirect));
        installed+=installNative("glMultiDrawArraysIndirectCount",reinterpret_cast<void*>(&nativeMultiDrawArraysIndirectCount));
        installed+=installNative("glMultiDrawElementsIndirectCount",reinterpret_cast<void*>(&nativeMultiDrawElementsIndirectCount));
        installed+=installNative("glMultiDrawArraysIndirectCountARB",reinterpret_cast<void*>(&nativeMultiDrawArraysIndirectCountARB));
        installed+=installNative("glMultiDrawElementsIndirectCountARB",reinterpret_cast<void*>(&nativeMultiDrawElementsIndirectCountARB));
        diagnostics::Write("[SMART] OpenGL native draw scopes available=%u",installed);
    }
    void unregisterNative(command_list* cmd)
    {
        std::lock_guard lock(nativeMutex);
        for(auto i=nativeContexts.begin();i!=nativeContexts.end();)
            if(i->second==cmd)i=nativeContexts.erase(i);else ++i;
    }
    bool beginNative(command_list* cmd,uint64_t handle,uint32_t hash,Choice choice)
    {
        for(auto* scope=currentDraw;scope;scope=scope->previous)
            if(scope->commands==cmd && scope->token.commands && scope->hash==hash && scope->choice==choice)return true;
        if(currentDraw && currentDraw->commands==cmd)
        {
            if(!begin(cmd,handle,hash,choice,currentDraw->token))return false;
            currentDraw->hash=hash;currentDraw->choice=choice;return true;
        }
        registerNative(cmd);
        auto& d=cmd->get_device()->get_private_data<DeviceData>();std::lock_guard lock(d.mutex);
        report(d,hash,choice,false,"OpenGL native draw capture is unavailable for this call; original pass retained.");return false;
    }
    void shutdown(bool processExit)
    {
        if(processExit)return;
        std::lock_guard lock(nativeMutex);nativeContexts.clear();
        for(auto& [target,original]:nativeOriginals)MH_DisableHook(target);
        for(auto& [target,original]:nativeOriginals)MH_RemoveHook(target);
        nativeOriginals.clear();
        for(auto module:nativeModules)FreeLibrary(module);nativeModules.clear();
    }
}
