///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – Texture toggling (experimental)
//
// MIT-compatible header retained as discussed.
//
/////////////////////////////////////////////////////////////////////////

#pragma once
#include <reshade.hpp>
#include <vector>
#include <unordered_map>
#include <string>

// Forward decls from existing codebase
class CDataFile;
struct ToggleGroup;

namespace texmod {

struct TextureTarget {
    uint32_t param_index = 0;
    uint32_t slot_offset = 0;
};

struct DummyCache {
    reshade::api::resource dummy_tex{};
    reshade::api::resource_view dummy_srv{};
};

struct SlotBinding {
    uint32_t param_index = 0;
    uint32_t slot_offset = 0;
    reshade::api::resource_view view{};
};

extern bool g_texHuntActive;
extern int  g_texHuntIndex;
extern std::vector<SlotBinding> g_texHuntSlots;

extern struct ToggleGroup* g_currentGroup;

void on_init_device(reshade::api::device* device);
void on_destroy_device(reshade::api::device* device);

void on_push_descriptors(reshade::api::command_list* cmd_list,
                         reshade::api::shader_stage stages,
                         uint32_t param_index,
                         const reshade::api::descriptor_range_update* updates,
                         uint32_t update_count);

void on_draw(reshade::api::command_list* cmd_list);
// Called each draw to apply overrides for any active groups
void apply_active_overrides(reshade::api::command_list* cmd_list);

void draw_group_ui(struct ToggleGroup& group);

void add_target_to_group(struct ToggleGroup& group, const TextureTarget& t);
std::vector<TextureTarget>& get_targets(struct ToggleGroup& group);

// Persistence into ShaderToggler.ini
void save_group_textures(CDataFile& ini, int groupCounter, struct ToggleGroup& group);
void load_group_textures(CDataFile& ini, int groupCounter, struct ToggleGroup& group);


} // namespace texmod
