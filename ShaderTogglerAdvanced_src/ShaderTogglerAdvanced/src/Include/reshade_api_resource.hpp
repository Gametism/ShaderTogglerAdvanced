/*
 * Copyright (C) 2021 Patrick Mours
 * License: https://github.com/crosire/reshade#license
 */
#pragma once
#define RESHADE_DEFINE_HANDLE(name) \
 typedef struct { uint64_t handle; } name; \
 constexpr bool operator< (name lhs, name rhs) { return lhs.handle < rhs.handle; } \
 constexpr bool operator!=(name lhs, name rhs) { return lhs.handle != rhs.handle; } \
 constexpr bool operator!=(name lhs, uint64_t rhs) { return lhs.handle != rhs; } \
 constexpr bool operator==(name lhs, name rhs) { return lhs.handle == rhs.handle; } \
 constexpr bool operator==(name lhs, uint64_t rhs) { return lhs.handle == rhs; }
#define RESHADE_DEFINE_ENUM_FLAG_OPERATORS(type) \
 constexpr type operator~(type a) { return static_cast<type>(~static_cast<uint32_t>(a)); } \
 inline type &operator&=(type &a, type b) { return reinterpret_cast<type &>(reinterpret_cast<uint32_t &>(a) &= static_cast<uint32_t>(b)); } \
 constexpr type operator&(type a, type b) { return static_cast<type>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); } \
 inline type &operator|=(type &a, type b) { return reinterpret_cast<type &>(reinterpret_cast<uint32_t &>(a) |= static_cast<uint32_t>(b)); } \
 constexpr type operator|(type a, type b) { return static_cast<type>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); } \
 inline type &operator^=(type &a, type b) { return reinterpret_cast<type &>(reinterpret_cast<uint32_t &>(a) ^= static_cast<uint32_t>(b)); } \
 constexpr type operator^(type a, type b) { return static_cast<type>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b)); } \
 constexpr bool operator==(type lhs, uint32_t rhs) { return static_cast<uint32_t>(lhs) == rhs; } \
 constexpr bool operator!=(type lhs, uint32_t rhs) { return static_cast<uint32_t>(lhs) != rhs; }
#include "reshade_api_format.hpp"
namespace reshade::api
{
 enum class compare_op : uint32_t
 {
  never = 0,
  less = 1,
  equal = 2,
  less_equal = 3,
  greater = 4,
  not_equal = 5,
  greater_equal = 6,
  always = 7
 };
 enum class filter_mode : uint32_t
 {
  min_mag_mip_point = 0,
  min_mag_point_mip_linear = 0x1,
  min_point_mag_linear_mip_point = 0x4,
  min_point_mag_mip_linear = 0x5,
  min_linear_mag_mip_point = 0x10,
  min_linear_mag_point_mip_linear = 0x11,
  min_mag_linear_mip_point = 0x14,
  min_mag_mip_linear = 0x15,
  anisotropic = 0x55,
  compare_min_mag_mip_point = 0x80,
  compare_min_mag_point_mip_linear = 0x81,
  compare_min_point_mag_linear_mip_point = 0x84,
  compare_min_point_mag_mip_linear = 0x85,
  compare_min_linear_mag_mip_point = 0x90,
  compare_min_linear_mag_point_mip_linear = 0x91,
  compare_min_mag_linear_mip_point = 0x94,
  compare_min_mag_mip_linear = 0x95,
  compare_anisotropic = 0xd5
 };
 enum class texture_address_mode : uint32_t
 {
  wrap = 1,
  mirror = 2,
  clamp = 3,
  border = 4,
  mirror_once = 5
 };
 struct sampler_desc
 {
  filter_mode filter = filter_mode::min_mag_mip_linear;
  texture_address_mode address_u = texture_address_mode::clamp;
  texture_address_mode address_v = texture_address_mode::clamp;
  texture_address_mode address_w = texture_address_mode::clamp;
  float mip_lod_bias = 0.0f;
  float max_anisotropy = 1.0f;
  compare_op compare_op = compare_op::never;
  float border_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  float min_lod = -FLT_MAX;
  float max_lod = +FLT_MAX;
 };
 RESHADE_DEFINE_HANDLE(sampler);
 enum class map_access
 {
  read_only = 1,
  write_only,
  read_write,
  write_discard
 };
 enum class memory_heap : uint32_t
 {
  unknown,
  gpu_only,
  cpu_to_gpu,
  gpu_to_cpu,
  cpu_only,
  custom
 };
 enum class resource_type : uint32_t
 {
  unknown,
  buffer,
  texture_1d,
  texture_2d,
  texture_3d,
  surface
 };
 enum class resource_flags : uint32_t
 {
  none = 0,
  dynamic = (1 << 3),
  cube_compatible = (1 << 2),
  generate_mipmaps = (1 << 0),
  shared = (1 << 1),
  shared_nt_handle = (1 << 11),
  structured = (1 << 6),
  sparse_binding = (1 << 18)
 };
 RESHADE_DEFINE_ENUM_FLAG_OPERATORS(resource_flags);
 enum class resource_usage : uint32_t
 {
  undefined = 0,
  index_buffer = 0x2,
  vertex_buffer = 0x1,
  constant_buffer = 0x8000,
  stream_output = 0x100,
  indirect_argument = 0x200,
  depth_stencil = 0x30,
  depth_stencil_read = 0x20,
  depth_stencil_write = 0x10,
  render_target = 0x4,
  shader_resource = 0xC0,
  shader_resource_pixel = 0x80,
  shader_resource_non_pixel = 0x40,
  unordered_access = 0x8,
  copy_dest = 0x400,
  copy_source = 0x800,
  resolve_dest = 0x1000,
  resolve_source = 0x2000,
  general = 0x80000000,
  present = 0x80000000 | render_target | copy_source,
  cpu_access = vertex_buffer | index_buffer | shader_resource | indirect_argument | copy_source
 };
 RESHADE_DEFINE_ENUM_FLAG_OPERATORS(resource_usage);
 struct resource_desc
 {
  constexpr resource_desc() : texture() {}
  constexpr resource_desc(uint64_t size, memory_heap heap, resource_usage usage) :
   type(resource_type::buffer), buffer({ size }), heap(heap), usage(usage) {}
  constexpr resource_desc(uint32_t width, uint32_t height, uint16_t layers, uint16_t levels, format format, uint16_t samples, memory_heap heap, resource_usage usage, resource_flags flags = resource_flags::none) :
   type(resource_type::texture_2d), texture({ width, height, layers, levels, format, samples }), heap(heap), usage(usage), flags(flags) {}
  constexpr resource_desc(resource_type type, uint32_t width, uint32_t height, uint16_t depth_or_layers, uint16_t levels, format format, uint16_t samples, memory_heap heap, resource_usage usage, resource_flags flags = resource_flags::none) :
   type(type), texture({ width, height, depth_or_layers, levels, format, samples }), heap(heap), usage(usage), flags(flags) {}
  resource_type type = resource_type::unknown;
  union
  {
   struct
   {
    uint64_t size = 0;
    uint32_t stride = 0;
   } buffer;
   struct
   {
    uint32_t width = 1;
    uint32_t height = 1;
    uint16_t depth_or_layers = 1;
    uint16_t levels = 1;
    format format = format::unknown;
    uint16_t samples = 1;
   } texture;
  };
  memory_heap heap = memory_heap::unknown;
  resource_usage usage = resource_usage::undefined;
  resource_flags flags = resource_flags::none;
 };
 RESHADE_DEFINE_HANDLE(resource);
 enum class resource_view_type : uint32_t
 {
  unknown,
  buffer,
  texture_1d,
  texture_1d_array,
  texture_2d,
  texture_2d_array,
  texture_2d_multisample,
  texture_2d_multisample_array,
  texture_3d,
  texture_cube,
  texture_cube_array
 };
 struct resource_view_desc
 {
  constexpr resource_view_desc() : texture() {}
  constexpr resource_view_desc(format format, uint64_t offset, uint64_t size) :
   type(resource_view_type::buffer), format(format), buffer({ offset, size }) {}
  constexpr resource_view_desc(format format, uint32_t first_level, uint32_t levels, uint32_t first_layer, uint32_t layers) :
   type(resource_view_type::texture_2d), format(format), texture({ first_level, levels, first_layer, layers }) {}
  constexpr resource_view_desc(resource_view_type type, format format, uint32_t first_level, uint32_t levels, uint32_t first_layer, uint32_t layers) :
   type(type), format(format), texture({ first_level, levels, first_layer, layers }) {}
  constexpr explicit resource_view_desc(format format) : type(resource_view_type::texture_2d), format(format), texture({ 0, 1, 0, 1 }) {}
  resource_view_type type = resource_view_type::unknown;
  format format = format::unknown;
  union
  {
   struct
   {
    uint64_t offset = 0;
    uint64_t size = UINT64_MAX;
   } buffer;
   struct
   {
    uint32_t first_level = 0;
    uint32_t level_count = UINT32_MAX;
    uint32_t first_layer = 0;
    uint32_t layer_count = UINT32_MAX;
   } texture;
  };
 };
 RESHADE_DEFINE_HANDLE(resource_view);
 struct subresource_box
 {
  int32_t left = 0;
  int32_t top = 0;
  int32_t front = 0;
  int32_t right = 0;
  int32_t bottom = 0;
  int32_t back = 0;
  constexpr uint32_t width() const { return right - left; }
  constexpr uint32_t height() const { return bottom - top; }
  constexpr uint32_t depth() const { return back - front; }
 };
 struct subresource_data
 {
  void *data = nullptr;
  uint32_t row_pitch = 0;
  uint32_t slice_pitch = 0;
 };
 enum class render_pass_load_op : uint32_t
 {
  load,
  clear,
  discard,
  dont_care
 };
 enum class render_pass_store_op : uint32_t
 {
  store,
  discard,
  dont_care
 };
 struct render_pass_depth_stencil_desc
 {
  resource_view view = { 0 };
  render_pass_load_op depth_load_op = render_pass_load_op::load;
  render_pass_store_op depth_store_op = render_pass_store_op::store;
  render_pass_load_op stencil_load_op = render_pass_load_op::load;
  render_pass_store_op stencil_store_op = render_pass_store_op::store;
  float clear_depth = 0.0f;
  uint8_t clear_stencil = 0;
 };
 struct render_pass_render_target_desc
 {
  resource_view view = { 0 };
  render_pass_load_op load_op = render_pass_load_op::load;
  render_pass_store_op store_op = render_pass_store_op::store;
  float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
 };
}
