// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#include "SmartDisableOther.h"
#include "Vendor/MinHook/include/MinHook.h"
namespace ShaderToggler::smart::other
{
    namespace gl
    {
        bool safeDraw(command_list*);
        void initDevice(device*);void destroyDevice(device*);void initCommands(command_list*);void destroyCommands(command_list*);
        void capture(device*,uint64_t,uint32_t,const reshade::api::pipeline_subobject*);void forget(device*,uint64_t);
        bool begin(command_list*,uint64_t,uint32_t,Choice,GLDraw&);void end(GLDraw&);
        bool beginNative(command_list*,uint64_t,uint32_t,Choice);void shutdown(bool);
        void observe(command_list*,uint64_t,uint32_t);Method suggestion(device*,uint32_t);Status status(device*,uint32_t);
    }
    namespace vk
    {
        void initDevice(device*);void destroyDevice(device*);void initCommands(command_list*);void destroyCommands(command_list*);
        void reset(command_list*);void bound(command_list*,reshade::api::pipeline_stage,uint64_t);void restore(command_list*);
        void beginPass(command_list*,uint32_t,const reshade::api::render_pass_render_target_desc*,const reshade::api::render_pass_depth_stencil_desc*);
        void endPass(command_list*);bool apply(command_list*,uint64_t,uint32_t,Choice);
        void observe(command_list*,uint64_t,uint32_t);Method suggestion(device*,uint32_t);Status status(device*,uint32_t);void summary(device*);void shutdown(bool);
    }
    void initDevice(device* d){gl::initDevice(d);vk::initDevice(d);}
    void destroyDevice(device* d){gl::destroyDevice(d);vk::destroyDevice(d);}
    void initCommands(command_list* c){gl::initCommands(c);vk::initCommands(c);}
    void destroyCommands(command_list* c){gl::destroyCommands(c);vk::destroyCommands(c);}
    void resetCommands(command_list* c){vk::reset(c);}
    void capture(device* d,uint64_t h,uint32_t n,const reshade::api::pipeline_subobject* s){gl::capture(d,h,n,s);}
    void forget(device* d,uint64_t h){gl::forget(d,h);}
    void bound(command_list* c,reshade::api::pipeline_stage s,uint64_t h){vk::bound(c,s,h);}
    void beginPass(command_list* c,uint32_t n,const reshade::api::render_pass_render_target_desc* r,const reshade::api::render_pass_depth_stencil_desc* d){vk::beginPass(c,n,r,d);}
    void endPass(command_list* c){vk::endPass(c);}
    void restore(command_list* c){vk::restore(c);}
    void observe(command_list* c,uint64_t h,uint32_t hash){if(isGL(c->get_device()))gl::observe(c,h,hash);else if(isVK(c->get_device()))vk::observe(c,h,hash);}
    Method suggestion(device* d,uint32_t h){return isGL(d)?gl::suggestion(d,h):isVK(d)?vk::suggestion(d,h):Method::TransparentBlack;}
    Status status(device* d,uint32_t h){return isGL(d)?gl::status(d,h):isVK(d)?vk::status(d,h):Status{};}
    void summary(device* d){vk::summary(d);}
    bool applyVK(command_list* c,uint64_t h,uint32_t hash,Choice v){return vk::apply(c,h,hash,v);}
    bool beginGL(command_list* c,uint64_t h,uint32_t hash,Choice v,GLDraw& t){return gl::begin(c,h,hash,v,t);}
    void endGL(GLDraw& t){gl::end(t);}
    bool applyGLNative(command_list* c,uint64_t h,uint32_t hash,Choice v){return gl::beginNative(c,h,hash,v);}
    void shutdown(bool processExit){gl::shutdown(processExit);vk::shutdown(processExit);}
    void finishShutdown(bool processExit){if(!processExit) MH_Uninitialize();}
    bool safeGLDraw(command_list* c){return gl::safeDraw(c);}
}
