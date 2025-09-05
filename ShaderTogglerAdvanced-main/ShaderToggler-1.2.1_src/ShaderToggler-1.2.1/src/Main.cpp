#include "ShaderProfiler.h"
#include <reshade.hpp>
#include <imgui.h>

static ShaderProfiler g_profiler;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        reshade::register_event<reshade::addon_event::reshade_overlay>(
            [](reshade::api::effect_runtime *runtime) {
                ImGui::Begin("Shader Stats Overlay");
                for (const auto &[pipeline, stats] : g_profiler.get_stats()) {
                    ImGui::Text("Shader 0x%p - Draw Calls: %u", (void *)pipeline.handle, stats.draw_calls);
                }
                ImGui::End();
            });

        reshade::register_event<reshade::addon_event::draw>(
            [](reshade::api::command_list *cmd_list, uint32_t vertices, uint32_t instances, reshade::api::pipeline pipeline) {
                g_profiler.on_draw_call(pipeline);
                return false;
            });

        reshade::register_event<reshade::addon_event::present>(
            [](reshade::api::command_queue *queue, reshade::api::swapchain *swapchain, const reshade::api::rect *src_rect, const reshade::api::rect *dst_rect, uint32_t dirty_count, const reshade::api::rect *dirty_rects) {
                g_profiler.new_frame();
                return false;
            });
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        reshade::unregister_event<reshade::addon_event::reshade_overlay>();
        reshade::unregister_event<reshade::addon_event::draw>();
        reshade::unregister_event<reshade::addon_event::present>();
    }

    return TRUE;
}