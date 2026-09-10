// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#pragma once
#include "SmartDisableD3D1011.h"
namespace ShaderToggler::smart::other
{
    bool safeGLDraw(reshade::api::command_list*);
    using reshade::api::device;
    using reshade::api::command_list;
    inline thread_local unsigned internalDepth = 0;
    struct Internal { Internal() { ++internalDepth; } ~Internal() { --internalDepth; } };
    inline bool internal() { return internalDepth != 0; }
    inline bool isGL(device* d) { return d && d->get_api() == reshade::api::device_api::opengl; }
    inline bool isVK(device* d) { return d && d->get_api() == reshade::api::device_api::vulkan; }
    inline bool supported(device* d) { return isGL(d) || isVK(d); }
    void initDevice(device*);
    void destroyDevice(device*);
    void initCommands(command_list*);
    void destroyCommands(command_list*);
    void resetCommands(command_list*);
    void capture(device*, uint64_t, uint32_t, const reshade::api::pipeline_subobject*);
    void forget(device*, uint64_t);
    void bound(command_list*, reshade::api::pipeline_stage, uint64_t);
    void beginPass(command_list*, uint32_t, const reshade::api::render_pass_render_target_desc*, const reshade::api::render_pass_depth_stencil_desc*);
    void endPass(command_list*);
    void restore(command_list*);
    void observe(command_list*, uint64_t, uint32_t);
    Method suggestion(device*, uint32_t);
    Status status(device*, uint32_t);
    void summary(device*);
    bool applyVK(command_list*, uint64_t, uint32_t, Choice);
    struct GLDraw { command_list* commands = nullptr; uint32_t original = 0, indirectBuffer = 0; };
    bool beginGL(command_list*, uint64_t, uint32_t, Choice, GLDraw&);
    void endGL(GLDraw&);
    bool applyGLNative(command_list*, uint64_t, uint32_t, Choice);
    void shutdown(bool processExit);
    void finishShutdown(bool processExit);
}
