#pragma once
#include <reshade.hpp>
#include <vector>
#include <unordered_map>
#include <string>

// Use fully-qualified name to avoid conflicts with ShaderToggler::ToggleGroup
namespace ShaderToggler { class ToggleGroup; }

namespace texmod
{
    using namespace reshade;

    struct TextureTarget
    {
        uint32_t param_index = 0; // pipeline layout parameter index (SRV table param)
        uint32_t slot_offset = 0; // descriptor offset in that param
    };

    // Observed SRV slots during hunting
    struct SlotBinding
    {
        uint32_t param_index = 0;
        uint32_t slot_offset = 0;
    };

    // Global state for texture hunting/overrides
    extern bool g_texHuntActive;
    extern int  g_texHuntIndex;
    extern std::vector<SlotBinding> g_texHuntSlots;
    extern ShaderToggler::ToggleGroup* g_currentGroup;

    // Device / dummy SRV state
    void on_init_device(reshade::api::device* device);
    void on_destroy_device(reshade::api::device* device);

    // Called while hunting (does live preview) and at end always applies active overrides
    void on_draw(reshade::api::command_list* cmd_list);

    // Apply overrides for all active groups (called every draw)
    void apply_active_overrides(reshade::api::command_list* cmd_list);

    // UI bits used by your overlay code
    void begin_hunt_for_group(ShaderToggler::ToggleGroup& group);
    void end_hunt();

    // Group <-> texture targets storage
    std::vector<TextureTarget>& get_targets(ShaderToggler::ToggleGroup& group);
    void add_target_to_group(ShaderToggler::ToggleGroup& group, const TextureTarget& t);

    // INI persistence
    class CDataFile; // forward
    void save_group_textures(CDataFile& ini, int groupCounter, ShaderToggler::ToggleGroup& group);
    void load_group_textures(CDataFile& ini, int groupCounter, ShaderToggler::ToggleGroup& group);

    // Register a safe draw hook to apply overrides every frame
    void register_draw_hook();
    void unregister_draw_hook();
}
