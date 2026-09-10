/*
 * Copyright (C) 2021 Patrick Mours
 * License: https://github.com/crosire/reshade#license
 */
#pragma once
#include "reshade_api_resource.hpp"
namespace reshade::api
{
 enum class shader_stage : uint32_t
 {
  vertex = 0x1,
  hull = 0x2,
  domain = 0x4,
  geometry = 0x8,
  pixel = 0x10,
  compute = 0x20,
  all = 0x7FFFFFFF,
  all_compute = compute,
  all_graphics = vertex | hull | domain | geometry | pixel
 };
 RESHADE_DEFINE_ENUM_FLAG_OPERATORS(shader_stage);
 enum class pipeline_stage : uint32_t
 {
  vertex_shader = 0x8,
  hull_shader = 0x10,
  domain_shader = 0x20,
  geometry_shader = 0x40,
  pixel_shader = 0x80,
  compute_shader = 0x800,
  input_assembler = 0x2,
  stream_output = 0x4,
  rasterizer = 0x100,
  depth_stencil = 0x200,
  output_merger = 0x400,
  all = 0x7FFFFFFF,
  all_compute = compute_shader,
  all_graphics = vertex_shader | hull_shader | domain_shader | geometry_shader | pixel_shader | input_assembler | stream_output | rasterizer | depth_stencil | output_merger,
  all_shader_stages = vertex_shader | hull_shader | domain_shader | geometry_shader | pixel_shader | compute_shader
 };
 RESHADE_DEFINE_ENUM_FLAG_OPERATORS(pipeline_stage);
 enum class descriptor_type : uint32_t
 {
  sampler = 0,
  sampler_with_resource_view = 1,
  shader_resource_view = 2,
  unordered_access_view = 3,
  constant_buffer = 6,
  shader_storage_buffer = 7
 };
 enum class pipeline_layout_param_type : uint32_t
 {
  push_constants = 1,
  push_descriptors = 2,
  descriptor_set = 0
 };
 struct constant_range
 {
  uint32_t binding = 0;
  uint32_t dx_register_index = 0;
  uint32_t dx_register_space = 0;
  uint32_t count = 0;
  shader_stage visibility = shader_stage::all;
 };
 struct descriptor_range
 {
  uint32_t binding = 0;
  uint32_t dx_register_index = 0;
  uint32_t dx_register_space = 0;
  uint32_t count = 0;
  shader_stage visibility = shader_stage::all;
  uint32_t array_size = 1;
  descriptor_type type = descriptor_type::sampler;
 };
 struct pipeline_layout_param
 {
  constexpr pipeline_layout_param() : push_descriptors() {}
  constexpr pipeline_layout_param(const constant_range &push_constants) : type(pipeline_layout_param_type::push_constants), push_constants(push_constants) {}
  constexpr pipeline_layout_param(const descriptor_range &push_descriptors) : type(pipeline_layout_param_type::push_descriptors), push_descriptors(push_descriptors) {}
  constexpr pipeline_layout_param(uint32_t count, const descriptor_range *ranges) : type(pipeline_layout_param_type::descriptor_set), descriptor_set({ count, ranges }) {}
  pipeline_layout_param_type type = pipeline_layout_param_type::push_descriptors;
  union
  {
   constant_range push_constants;
   descriptor_range push_descriptors;
   struct
   {
    uint32_t count;
    const descriptor_range *ranges;
   } descriptor_set;
  };
 };
 RESHADE_DEFINE_HANDLE(pipeline_layout);
 enum class fill_mode : uint32_t
 {
  solid = 0,
  wireframe = 1,
  point = 2
 };
 enum class cull_mode : uint32_t
 {
  none = 0,
  front = 1,
  back = 2,
  front_and_back = front | back
 };
 RESHADE_DEFINE_ENUM_FLAG_OPERATORS(cull_mode);
 enum class logic_op : uint32_t
 {
  clear = 0,
  bitwise_and = 1,
  bitwise_and_reverse = 2,
  copy = 3,
  bitwise_and_inverted = 4,
  noop = 5,
  bitwise_xor = 6,
  bitwise_or = 7,
  bitwise_nor = 8,
  equivalent = 9,
  invert = 10,
  bitwise_or_reverse = 11,
  copy_inverted = 12,
  bitwise_or_inverted = 13,
  bitwise_nand = 14,
  set = 15
 };
 enum class blend_op : uint32_t
 {
  add = 0,
  subtract = 1,
  reverse_subtract = 2,
  min = 3,
  max = 4
 };
 enum class blend_factor : uint32_t
 {
  zero = 0,
  one = 1,
  source_color = 2,
  one_minus_source_color = 3,
  dest_color = 4,
  one_minus_dest_color = 5,
  source_alpha = 6,
  one_minus_source_alpha = 7,
  dest_alpha = 8,
  one_minus_dest_alpha = 9,
  constant_color = 10,
  one_minus_constant_color = 11,
  constant_alpha = 12,
  one_minus_constant_alpha = 13,
  source_alpha_saturate = 14,
  source1_color = 15,
  one_minus_source1_color = 16,
  source1_alpha = 17,
  one_minus_source1_alpha = 18
 };
 enum class stencil_op : uint32_t
 {
  keep = 0,
  zero = 1,
  replace = 2,
  increment_saturate = 3,
  decrement_saturate = 4,
  invert = 5,
  increment = 6,
  decrement = 7
 };
 enum class primitive_topology : uint32_t
 {
  undefined = 0,
  point_list = 1,
  line_list = 2,
  line_strip = 3,
  triangle_list = 4,
  triangle_strip = 5,
  triangle_fan = 6,
  line_list_adj = 10,
  line_strip_adj = 11,
  triangle_list_adj = 12,
  triangle_strip_adj = 13,
  patch_list_01_cp = 33,
  patch_list_02_cp,
  patch_list_03_cp,
  patch_list_04_cp,
  patch_list_05_cp,
  patch_list_06_cp,
  patch_list_07_cp,
  patch_list_08_cp,
  patch_list_09_cp,
  patch_list_10_cp,
  patch_list_11_cp,
  patch_list_12_cp,
  patch_list_13_cp,
  patch_list_14_cp,
  patch_list_15_cp,
  patch_list_16_cp,
  patch_list_17_cp,
  patch_list_18_cp,
  patch_list_19_cp,
  patch_list_20_cp,
  patch_list_21_cp,
  patch_list_22_cp,
  patch_list_23_cp,
  patch_list_24_cp,
  patch_list_25_cp,
  patch_list_26_cp,
  patch_list_27_cp,
  patch_list_28_cp,
  patch_list_29_cp,
  patch_list_30_cp,
  patch_list_31_cp,
  patch_list_32_cp
 };
 struct shader_desc
 {
  const void *code = nullptr;
  size_t code_size = 0;
  const char *entry_point = nullptr;
  uint32_t spec_constants = 0;
  const uint32_t *spec_constant_ids = nullptr;
  const uint32_t *spec_constant_values = nullptr;
 };
 struct input_element
 {
  uint32_t location = 0;
  const char *semantic = nullptr;
  uint32_t semantic_index = 0;
  format format = format::unknown;
  uint32_t buffer_binding = 0;
  uint32_t offset = 0;
  uint32_t stride = 0;
  uint32_t instance_step_rate = 0;
 };
 struct stream_output_desc
 {
  uint32_t rasterized_stream = 0;
 };
 struct blend_desc
 {
  bool alpha_to_coverage_enable = false;
  bool blend_enable[8] = { false, false, false, false, false, false, false, false };
  bool logic_op_enable[8] = { false, false, false, false, false, false, false, false };
  blend_factor source_color_blend_factor[8] = { blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one };
  blend_factor dest_color_blend_factor[8] = { blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero };
  blend_op color_blend_op[8] = { blend_op::add, blend_op::add, blend_op::add, blend_op::add, blend_op::add, blend_op::add, blend_op::add, blend_op::add };
  blend_factor source_alpha_blend_factor[8] = { blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one, blend_factor::one };
  blend_factor dest_alpha_blend_factor[8] = { blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero, blend_factor::zero };
  blend_op alpha_blend_op[8] = { blend_op::add, blend_op::add, blend_op::add, blend_op::add, blend_op::add, blend_op::add, blend_op::add, blend_op::add };
  float blend_constant[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  logic_op logic_op[8] = { logic_op::noop, logic_op::noop, logic_op::noop, logic_op::noop, logic_op::noop, logic_op::noop, logic_op::noop, logic_op::noop };
  uint8_t render_target_write_mask[8] = { 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF };
 };
 struct rasterizer_desc
 {
  fill_mode fill_mode = fill_mode::solid;
  cull_mode cull_mode = cull_mode::back;
  bool front_counter_clockwise = false;
  float depth_bias = 0.0f;
  float depth_bias_clamp = 0.0f;
  float slope_scaled_depth_bias = 0.0f;
  bool depth_clip_enable = true;
  bool scissor_enable = false;
  bool multisample_enable = false;
  bool antialiased_line_enable = false;
  uint32_t conservative_rasterization = 0;
 };
 struct depth_stencil_desc
 {
  bool depth_enable = true;
  bool depth_write_mask = true;
  compare_op depth_func = compare_op::less;
  bool stencil_enable = false;
  uint8_t stencil_read_mask = 0xFF;
  uint8_t stencil_write_mask = 0xFF;
  uint8_t stencil_reference_value = 0;
  compare_op front_stencil_func = compare_op::always;
  stencil_op front_stencil_pass_op = stencil_op::keep;
  stencil_op front_stencil_fail_op = stencil_op::keep;
  stencil_op front_stencil_depth_fail_op = stencil_op::keep;
  compare_op back_stencil_func = compare_op::always;
  stencil_op back_stencil_pass_op = stencil_op::keep;
  stencil_op back_stencil_fail_op = stencil_op::keep;
  stencil_op back_stencil_depth_fail_op = stencil_op::keep;
 };
 enum class pipeline_subobject_type
 {
  unknown,
  vertex_shader,
  hull_shader,
  domain_shader,
  geometry_shader,
  pixel_shader,
  compute_shader,
  input_layout,
  stream_output_state,
  blend_state,
  rasterizer_state,
  depth_stencil_state,
  primitive_topology,
  depth_stencil_format,
  render_target_formats,
  sample_mask,
  sample_count,
  viewport_count,
  dynamic_pipeline_states,
  max_vertex_count
 };
 struct pipeline_subobject
 {
  pipeline_subobject_type type = pipeline_subobject_type::unknown;
  uint32_t count = 0;
  void *data = nullptr;
 };
 RESHADE_DEFINE_HANDLE(pipeline);
 struct buffer_range
 {
  resource buffer = { 0 };
  uint64_t offset = 0;
  uint64_t size = UINT64_MAX;
 };
 struct sampler_with_resource_view
 {
  sampler sampler = { 0 };
  resource_view view = { 0 };
 };
 RESHADE_DEFINE_HANDLE(descriptor_set);
 struct descriptor_set_copy
 {
  descriptor_set source_set = { 0 };
  uint32_t source_binding = 0;
  uint32_t source_array_offset = 0;
  descriptor_set dest_set = { 0 };
  uint32_t dest_binding = 0;
  uint32_t dest_array_offset = 0;
  uint32_t count = 0;
 };
 struct descriptor_set_update
 {
  descriptor_set set = { 0 };
  uint32_t binding = 0;
  uint32_t array_offset = 0;
  uint32_t count = 0;
  descriptor_type type = descriptor_type::sampler;
  const void *descriptors = nullptr;
 };
 RESHADE_DEFINE_HANDLE(descriptor_pool);
 enum class query_type
 {
  occlusion = 0,
  binary_occlusion = 1,
  timestamp = 2,
  pipeline_statistics = 3,
  stream_output_statistics_0 = 4,
  stream_output_statistics_1,
  stream_output_statistics_2,
  stream_output_statistics_3
 };
 RESHADE_DEFINE_HANDLE(query_pool);
 enum class dynamic_state
 {
  unknown = 0,
  alpha_test_enable = 15,
  alpha_reference_value = 24,
  alpha_func = 25,
  srgb_write_enable = 194,
  primitive_topology = 1000,
  sample_mask = 162,
  alpha_to_coverage_enable = 1003,
  blend_enable = 27,
  logic_op_enable = 1004,
  source_color_blend_factor = 19,
  dest_color_blend_factor = 20,
  color_blend_op = 171,
  source_alpha_blend_factor = 207,
  dest_alpha_blend_factor = 208,
  alpha_blend_op = 209,
  logic_op = 1005,
  blend_constant = 193,
  render_target_write_mask = 168,
  fill_mode = 8,
  cull_mode = 22,
  front_counter_clockwise = 1001,
  depth_bias = 195,
  depth_bias_clamp = 1002,
  depth_bias_slope_scaled = 175,
  depth_clip_enable = 136,
  scissor_enable = 174,
  multisample_enable = 161,
  antialiased_line_enable = 176,
  depth_enable = 7,
  depth_write_mask = 14,
  depth_func = 23,
  stencil_enable = 52,
  stencil_read_mask = 58,
  stencil_write_mask = 59,
  stencil_reference_value = 57,
  front_stencil_func = 56,
  front_stencil_pass_op = 55,
  front_stencil_fail_op = 53,
  front_stencil_depth_fail_op = 54,
  back_stencil_func = 189,
  back_stencil_pass_op = 188,
  back_stencil_fail_op = 186,
  back_stencil_depth_fail_op = 187
 };
 struct rect
 {
  int32_t left = 0;
  int32_t top = 0;
  int32_t right = 0;
  int32_t bottom = 0;
  constexpr uint32_t width() const { return right - left; }
  constexpr uint32_t height() const { return bottom - top; }
 };
 struct viewport
 {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  float min_depth = 0.0f;
  float max_depth = 1.0f;
 };
}
