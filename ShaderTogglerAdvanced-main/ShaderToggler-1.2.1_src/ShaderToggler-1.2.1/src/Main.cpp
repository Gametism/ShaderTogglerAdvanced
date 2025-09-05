#include "ShaderProfiler.h"
#include <reshade.hpp>
#include <imgui.h>
#include <Xinput.h>
#pragma comment(lib, "Xinput9_1_0.lib")

static ShaderProfiler g_profiler;
static bool show_gamepad_toggle_msg = false;
static int msg_timer_frames = 0;

void poll_gamepad()
{
    XINPUT_STATE state = {};
    if (XInputGetState(0, &state) == ERROR_SUCCESS)
    {
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
        {
            show_gamepad_toggle_msg = true;
            msg_timer_frames = 120; // Show message for ~2 seconds at 60fps

            // Here you could call your shader toggle logic for Group 1
        }
    }
}

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

                if (show_gamepad_toggle_msg && msg_timer_frames > 0) {
                    ImGui::Begin("Toggle Notification", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
                    ImGui::Text("Toggled Group 1 via Gamepad!");
                    ImGui::End();
                }
            });

        reshade::register_event<reshade::addon_event::draw>(
            [](reshade::api::command_list *cmd_list, uint32_t vertices, uint32_t instances, reshade::api::pipeline pipeline) {
                g_profiler.on_draw_call(pipeline);
                return false;
            });

        reshade::register_event<reshade::addon_event::present>(
            [](reshade::api::command_queue *queue, reshade::api::swapchain *swapchain, const reshade::api::rect *src_rect, const reshade::api::rect *dst_rect, uint32_t dirty_count, const reshade::api::rect *dirty_rects) {
                g_profiler.new_frame();
                poll_gamepad();
                if (msg_timer_frames > 0) msg_timer_frames--;
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