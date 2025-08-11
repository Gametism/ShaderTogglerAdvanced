#include "CDataFile.h"
///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – Texture toggling (experimental)
//
// MIT-compatible header retained as discussed.
//
/////////////////////////////////////////////////////////////////////////

#pragma once

namespace ShaderToggler { struct ToggleGroup; }
#include <reshade.hpp>
#include <vector>
#include <unordered_map>
#include <string>

// Forward decls from existing codebase
class CDataFile;

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
                         const /*descriptor_range_update_removed*/ int* updates,
                         uint32_t update_count);

void on_draw(reshade::api::command_list* cmd_list);
// Called each draw to apply overrides for any active groups
void apply_active_overrides(reshade::api::command_list* cmd_list);

void draw_group_ui(struct ShaderToggler::ToggleGroup& group);

void add_target_to_group(struct ShaderToggler::ToggleGroup& group, const TextureTarget& t);
std::vector<TextureTarget>& get_targets(struct ShaderToggler::ToggleGroup& group);

// Persistence into ShaderToggler.ini
void save_group_textures(CDataFile& ini, int groupCounter, struct ShaderToggler::ToggleGroup& group);
void load_group_textures(CDataFile& ini, int groupCounter, struct ShaderToggler::ToggleGroup& group);


} // namespace texmod
