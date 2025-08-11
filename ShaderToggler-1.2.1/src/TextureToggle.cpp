#include "TextureToggle.h"
#include "ToggleGroup.h"         // real definition lives in namespace ShaderToggler
#include "CDataFile.h"
#include <Windows.h>
#include <limits>

using namespace reshade;

namespace texmod
{
    // -------- Global state
    bool g_texHuntActive = false;
    int  g_texHuntIndex  = -1;
    std::vector<SlotBinding> g_texHuntSlots;
    ShaderToggler::ToggleGroup* g_currentGroup = nullptr;

    static api::device* g_device = nullptr;
    static api::resource g_dummy_tex = {};
    static api::resource_view g_dummy_srv = {};

    // Per-group targets
    static std::unordered_map<ShaderToggler::ToggleGroup*, std::vector<TextureTarget>> g_groupTargets;

    // Key edge detection
    static bool s_prevN1 = false, s_prevN2 = false, s_prevN3 = false;
    static bool key_pressed_edge(int vkey, bool &prev)
    {
        SHORT s = GetAsyncKeyState(vkey);
        bool down = (s & 0x8000) != 0;
        bool edge = (down && !prev);
        prev = down;
        return edge;
    }

    // -------- Utility: ensure dummy 1x1 texture + SRV exist
    static void ensure_dummy()
    {
        if (!g_device) return;
        if (g_dummy_tex.handle != 0) return;

        api::resource_desc desc(
            1, 1, 1, 1, api::format::r8g8b8a8_unorm, 1,
            api::memory_heap::gpu_only,
            api::resource_usage::shader_resource);
        g_device->create_resource(desc, nullptr, api::resource_usage::shader_resource, &g_dummy_tex);

        api::resource_view_desc srv_desc(api::resource_view_type::texture_2d, api::format::r8g8b8a8_unorm);
        g_device->create_resource_view(g_dummy_tex, api::resource_usage::shader_resource, srv_desc, &g_dummy_srv);
    }

    static void destroy_dummy()
    {
        if (!g_device) return;
        if (g_dummy_srv.handle) { g_device->destroy_resource_view(g_dummy_srv); g_dummy_srv = {}; }
        if (g_dummy_tex.handle) { g_device->destroy_resource(g_dummy_tex); g_dummy_tex = {}; }
    }

    // -------- Device events
    void on_init_device(api::device* device)
    {
        g_device = device;
        ensure_dummy();
    }

    void on_destroy_device(api::device* device)
    {
        if (device == g_device)
        {
            destroy_dummy();
            g_device = nullptr;
        }
    }

    // -------- Write a single dummy SRV via push_descriptors on the command list
    static void push_dummy_for_slot(api::command_list* cmd_list, uint32_t param_index, uint32_t slot_offset)
    {
        if (!g_dummy_srv.handle) ensure_dummy();
        cmd_list->push_descriptors(api::shader_stage::all, param_index, api::descriptor_type::texture_shader_resource_view, slot_offset, 1, &g_dummy_srv);
    }

    // -------- Public API
    std::vector<TextureTarget>& get_targets(ShaderToggler::ToggleGroup& group)
    {
        return g_groupTargets[&group];
    }

    void add_target_to_group(ShaderToggler::ToggleGroup& group, const TextureTarget& t)
    {
        auto &vec = g_groupTargets[&group];
        // avoid duplicates
        for (auto &e : vec)
            if (e.param_index == t.param_index && e.slot_offset == t.slot_offset)
                return;
        vec.push_back(t);
    }

    void begin_hunt_for_group(ShaderToggler::ToggleGroup& group)
    {
        g_texHuntActive = true;
        g_currentGroup = &group;
        g_texHuntSlots.clear();
        g_texHuntIndex = -1;
        // Note: collecting observed slots requires you to fill g_texHuntSlots from your tracking code.
    }

    void end_hunt()
    {
        g_texHuntActive = false;
        g_currentGroup = nullptr;
        g_texHuntSlots.clear();
        g_texHuntIndex = -1;
    }

    // Apply overrides for all *active* groups every draw
    void apply_active_overrides(api::command_list* cmd_list)
    {
        for (auto &pair : g_groupTargets)
        {
            ShaderToggler::ToggleGroup* grp = pair.first;
            if (!grp || !grp->isActive()) continue;
            for (const auto &t : pair.second)
            {
                push_dummy_for_slot(cmd_list, t.param_index, t.slot_offset);
            }
        }
    }

    // Called from draw event: handle hunting hotkeys + preview, then apply overrides for active groups
    void on_draw(api::command_list* cmd_list)
    {
        if (g_texHuntActive)
        {
            if (key_pressed_edge(VK_NUMPAD1, s_prevN1) && !g_texHuntSlots.empty())
                g_texHuntIndex = (g_texHuntIndex + (int)g_texHuntSlots.size() - 1) % (int)g_texHuntSlots.size();

            if (key_pressed_edge(VK_NUMPAD2, s_prevN2) && !g_texHuntSlots.empty())
                g_texHuntIndex = (g_texHuntIndex + 1) % (int)g_texHuntSlots.size();

            if (key_pressed_edge(VK_NUMPAD3, s_prevN3) && g_currentGroup && !g_texHuntSlots.empty() &&
                g_texHuntIndex >= 0 && g_texHuntIndex < (int)g_texHuntSlots.size())
            {
                const auto& sbm = g_texHuntSlots[g_texHuntIndex];
                add_target_to_group(*g_currentGroup, TextureTarget{ sbm.param_index, sbm.slot_offset });
            }

            if (!g_texHuntSlots.empty() && g_texHuntIndex >= 0 && g_texHuntIndex < (int)g_texHuntSlots.size())
            {
                const auto& sb = g_texHuntSlots[g_texHuntIndex];
                push_dummy_for_slot(cmd_list, sb.param_index, sb.slot_offset);
            }
        }

        // Always apply for any active group
        apply_active_overrides(cmd_list);
    }

    // -------- INI persistence
    void save_group_textures(CDataFile& ini, int groupCounter, ShaderToggler::ToggleGroup& group)
    {
        auto &targets = g_groupTargets[&group];
        const std::string section = "Group" + std::to_string(groupCounter) + "_Textures";
        ini.SetInt("TextureTargetsCount", (int)targets.size(), "", section.c_str());
        for (size_t i = 0; i < targets.size(); ++i)
        {
            const auto &t = targets[i];
            ini.SetInt(("T" + std::to_string(i) + "_Root").c_str(), (int)t.param_index, "", section.c_str());
            ini.SetInt(("T" + std::to_string(i) + "_Slot").c_str(), (int)t.slot_offset, "", section.c_str());
        }
    }

    void load_group_textures(CDataFile& ini, int groupCounter, ShaderToggler::ToggleGroup& group)
    {
        const std::string section = "Group" + std::to_string(groupCounter) + "_Textures";
        int count = ini.GetInt("TextureTargetsCount", section.c_str());
        if (count <= 0)
            return;
        auto &vec = g_groupTargets[&group];
        vec.clear();
        for (int i = 0; i < count; ++i)
        {
            int root = ini.GetInt(("T" + std::to_string(i) + "_Root").c_str(), section.c_str());
            int slot = ini.GetInt(("T" + std::to_string(i) + "_Slot").c_str(), section.c_str());
            if (root == INT_MIN || slot == INT_MIN)
                continue;
            vec.push_back(TextureTarget{ (uint32_t)root, (uint32_t)slot });
        }
    }

    // -------- ReShade event hook (draw)
    static bool on_draw_event(api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, uint32_t)
    {
        on_draw(cmd_list);
        return false; // don't block app draw
    }

    void register_draw_hook()
    {
        reshade::register_event<reshade::addon_event::draw>(&on_draw_event);
    }

    void unregister_draw_hook()
    {
        reshade::unregister_event<reshade::addon_event::draw>(&on_draw_event);
    }
} // namespace texmod
