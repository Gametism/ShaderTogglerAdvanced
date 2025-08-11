#include "TextureToggle.h"
#include "ToggleGroup.h"
#include "CDataFile.h"
#include <Windows.h>
#include <imgui.h>

using namespace reshade;

extern std::vector<ToggleGroup> g_toggleGroups;

namespace texmod {

bool g_texHuntActive = false;
                g_currentGroup = nullptr;
int  g_texHuntIndex  = 0;
std::vector<SlotBinding> g_texHuntSlots;
ToggleGroup* g_currentGroup = nullptr;

static DummyCache g_dummy;
static api::device* g_device = nullptr;
static bool s_prevN1 = false;
static bool s_prevN2 = false;
static bool s_prevN3 = false;

static bool key_pressed_edge(int vkey, bool &prev)
{
    SHORT s = GetAsyncKeyState(vkey);
    bool down = (s & 0x8000) != 0;
    bool edge = (down && !prev);
    prev = down;
    return edge;
}


static std::unordered_map<ToggleGroup*, std::vector<TextureTarget>> g_groupTargets;

void on_init_device(api::device* device)
{
    g_device = device;
    if (g_dummy.dummy_tex.handle != 0)
        return;

    api::resource_desc desc(
        1, 1, 1, 1,
        api::format::r8g8b8a8_unorm,
        1,
        api::memory_heap::gpu_only,
        api::resource_usage::shader_resource | api::resource_usage::copy_dest);

    if (device->create_resource(desc, nullptr, api::resource_usage::undefined, &g_dummy.dummy_tex))
    {
        uint32_t rgba = 0x00000000u;
        device->update_texture_region(g_dummy.dummy_tex, 0, 0, {0,0,0}, {1,1,1}, &rgba, sizeof(uint32_t), 0);

        api::resource_view_desc srv_desc(api::format::r8g8b8a8_unorm);
        device->create_resource_view(g_dummy.dummy_tex, api::resource_usage::shader_resource, srv_desc, &g_dummy.dummy_srv);
    }
}

void on_destroy_device(api::device* device)
{
    if (g_dummy.dummy_srv.handle != 0)
        device->destroy_resource_view(g_dummy.dummy_srv);
    if (g_dummy.dummy_tex.handle != 0)
        device->destroy_resource(g_dummy.dummy_tex);

    g_dummy = {};
    g_device = nullptr;
}

void on_push_descriptors(api::command_list* cmd_list,
                         api::shader_stage stages,
                         uint32_t param_index,
                         const api::descriptor_range_update* updates,
                         uint32_t update_count)
{
    if (!g_texHuntActive)
        return;

    for (uint32_t ui = 0; ui < update_count; ++ui)
    {
        const auto& upd = updates[ui];
        if (upd.type != api::descriptor_type::texture_shader_resource_view)
            continue;

        for (uint32_t i = 0; i < upd.count; ++i)
        {
            SlotBinding sb{};
            sb.param_index = param_index;
            sb.slot_offset = upd.dest_offset + i;
            sb.view = upd.descriptors[i].texture_shader_resource_view;

            bool exists = false;
            for (const auto& e : g_texHuntSlots)
                if (e.param_index == sb.param_index && e.slot_offset == sb.slot_offset) { exists = true; break; }
            if (!exists)
                g_texHuntSlots.push_back(sb);
            if (g_texHuntSlots.size() > 256)
                g_texHuntSlots.erase(g_texHuntSlots.begin());
        }
    }
}

static void push_dummy_for_slot(api::command_list* cmd_list, uint32_t param_index, uint32_t slot_offset)
{
    if (g_dummy.dummy_srv.handle == 0) return;

    api::descriptor_data one{};
    one.type = api::descriptor_type::texture_shader_resource_view;
    one.texture_shader_resource_view = g_dummy.dummy_srv;

    cmd_list->push_descriptors(api::shader_stage::pixel, param_index, slot_offset, 1, one);
}

void on_draw(api::command_list* cmd_list)
{
    if (g_texHuntActive)
    {
        if (key_pressed_edge(VK_NUMPAD1, s_prevN1) && !g_texHuntSlots.empty())
        {
            g_texHuntIndex = (g_texHuntIndex + (int)g_texHuntSlots.size() - 1) % (int)g_texHuntSlots.size();
        }
        if (key_pressed_edge(VK_NUMPAD2, s_prevN2) && !g_texHuntSlots.empty())
        {
            g_texHuntIndex = (g_texHuntIndex + 1) % (int)g_texHuntSlots.size();
        }
        if (key_pressed_edge(VK_NUMPAD3, s_prevN3) && g_currentGroup && !g_texHuntSlots.empty()
            && g_texHuntIndex >= 0 && g_texHuntIndex < (int)g_texHuntSlots.size())
        {
            const auto& sbm = g_texHuntSlots[g_texHuntIndex];
            TextureTarget t{ sbm.param_index, sbm.slot_offset };
            add_target_to_group(*g_currentGroup, t);
        }

        if (!g_texHuntSlots.empty() && g_texHuntIndex >= 0 && g_texHuntIndex < (int)g_texHuntSlots.size())
        {
            const auto& sb = g_texHuntSlots[g_texHuntIndex];
            push_dummy_for_slot(cmd_list, sb.param_index, sb.slot_offset);
        }
    }
    // Always apply overrides for active groups after preview
    apply_active_overrides(cmd_list);
}

void add_target_to_group(ToggleGroup& group, const TextureTarget& t)
{
    g_groupTargets[&group].push_back(t);
}

std::vector<TextureTarget>& get_targets(ToggleGroup& group)
{
    return g_groupTargets[&group];
}

void draw_group_ui(ToggleGroup& group)
{
    if (ImGui::CollapsingHeader("Texture Hunt (experimental)"))
    {
        if (!g_texHuntActive)
        {
            if (ImGui::Button("Start Texture Hunt"))
            {
                g_texHuntActive = true;
                g_currentGroup = &group;
                g_texHuntSlots.clear();
                g_texHuntIndex = 0;
            }
        }
        else
        {
            if (ImGui::Button("Stop"))
                g_texHuntActive = false;
                g_currentGroup = nullptr;
            ImGui::SameLine();
            ImGui::Text("Observed slots: %d", (int)g_texHuntSlots.size());

            if (!g_texHuntSlots.empty())
            {
                if (ImGui::Button("<")) { g_texHuntIndex = (g_texHuntIndex + (int)g_texHuntSlots.size() - 1) % (int)g_texHuntSlots.size(); }
                ImGui::SameLine();
                if (ImGui::Button(">")) { g_texHuntIndex = (g_texHuntIndex + 1) % (int)g_texHuntSlots.size(); }
                ImGui::SameLine();
                const auto& sb = g_texHuntSlots[g_texHuntIndex];
                ImGui::Text("Preview: root=%u, slot=%u", sb.param_index, sb.slot_offset);

                if (ImGui::Button("Mark slot into group"))
                {
                    TextureTarget t{ sb.param_index, sb.slot_offset };
                    add_target_to_group(group, t);
                }
            }
        }

        auto& targets = get_targets(group);
        if (!targets.empty())
        {
            ImGui::Separator();
            ImGui::TextUnformatted("Marked texture slots:");
            for (size_t i = 0; i < targets.size(); ++i)
            {
                ImGui::PushID((int)i);
                ImGui::Text("root=%u, slot=%u", targets[i].param_index, targets[i].slot_offset);
                ImGui::SameLine();
                if (ImGui::Button("Remove")) { targets.erase(targets.begin() + i); ImGui::PopID(); break; }
                ImGui::PopID();
            }
        }
    }
}



void apply_active_overrides(api::command_list* cmd_list)
{
    // For each active group, override all marked texture slots
    for (auto &grp : g_toggleGroups)
    {
        if (!grp.isActive())
            continue;
        auto it = g_groupTargets.find(&grp);
        if (it == g_groupTargets.end())
            continue;
        for (const auto &t : it->second)
        {
            push_dummy_for_slot(cmd_list, t.param_index, t.slot_offset);
        }
    }
}


void save_group_textures(CDataFile& ini, int groupCounter, ToggleGroup& group)
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

void load_group_textures(CDataFile& ini, int groupCounter, ToggleGroup& group)
{
    const std::string section = "Group" + std::to_string(groupCounter) + "_Textures";
    int count = ini.GetInt("TextureTargetsCount", section.c_str());
    if (count == INT_MIN || count <= 0)
        return;
    auto &vec = g_groupTargets[&group];
    vec.clear();
    for (int i = 0; i < count; ++i)
    {
        int root = ini.GetInt(("T" + std::to_string(i) + "_Root").c_str(), section.c_str());
        int slot = ini.GetInt(("T" + std::to_string(i) + "_Slot").c_str(), section.c_str());
        if (root == INT_MIN || slot == INT_MIN) continue;
        vec.push_back(TextureTarget{ (uint32_t)root, (uint32_t)slot });
    }
}
} // namespace texmod
