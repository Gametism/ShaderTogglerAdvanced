#include "ShaderManager.h"
#include "ShaderProfiler.h"
#include <reshade.hpp>
#include <imgui.h>

static ShaderProfiler g_profiler;

void on_present()
{
    g_profiler.new_frame();

    if (ImGui::Begin("Shader Stats Overlay"))
    {
        for (const auto& [pipeline, stats] : g_profiler.get_stats())
        {
            ImGui::Text("Shader 0x%p - Draw Calls: %u", (void*)pipeline.handle, stats.draw_calls);
        }
        ImGui::End();
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        reshade::register_overlay("ShaderStats", &on_present);

        reshade::register_event<reshade::addon_event::draw>(
            [](reshade::api::command_list* cmd_list, uint32_t vertices, uint32_t instances, reshade::api::pipeline pipeline) {
                g_profiler.on_draw_call(pipeline);
                return false;
            });
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        reshade::unregister_overlay("ShaderStats", &on_present);
        reshade::unregister_event<reshade::addon_event::draw>();
    }

    return TRUE;
}