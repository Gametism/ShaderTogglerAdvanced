/*
 * Copyright (C) 2021 Patrick Mours
 * License: https://github.com/crosire/reshade#license
 */
#pragma once
#define RESHADE_DEFINE_INTERFACE(name) \
 struct __declspec(novtable) name
#define RESHADE_DEFINE_INTERFACE_WITH_BASE(name,base) \
 struct __declspec(novtable) name : public base
#include "reshade_api_pipeline.hpp"
namespace reshade::api
{
 enum class device_api
 {
  d3d9 = 0x9000,
  d3d10 = 0xa000,
  d3d11 = 0xb000,
  d3d12 = 0xc000,
  opengl = 0x10000,
  vulkan = 0x20000
 };
 enum class device_caps
 {
  compute_shader = 1,
  geometry_shader,
  hull_and_domain_shader,
  logic_op,
  dual_source_blend,
  independent_blend,
  fill_mode_non_solid,
  conservative_rasterization,
  bind_render_targets_and_depth_stencil,
  multi_viewport,
  partial_push_constant_updates,
  partial_push_descriptor_updates,
  draw_instanced,
  draw_or_dispatch_indirect,
  copy_buffer_region,
  copy_buffer_to_texture,
  blit,
  resolve_region,
  copy_query_pool_results,
  sampler_compare,
  sampler_anisotropic,
  sampler_with_resource_view,
  shared_resource,
  shared_resource_nt_handle
 };
 RESHADE_DEFINE_INTERFACE(api_object)
 {
  virtual uint64_t get_native() const = 0;
  virtual void get_private_data(const uint8_t guid[16], uint64_t *data) const = 0;
  virtual void set_private_data(const uint8_t guid[16], const uint64_t data) = 0;
  template <typename T> inline T &get_private_data() const
  {
   uint64_t res;
   get_private_data(reinterpret_cast<const uint8_t *>(&__uuidof(T)), &res);
   return *reinterpret_cast<T *>(static_cast<uintptr_t>(res));
  }
  template <typename T> inline T &create_private_data()
  {
   uint64_t res = reinterpret_cast<uintptr_t>(new T());
   set_private_data(reinterpret_cast<const uint8_t *>(&__uuidof(T)), res);
   return *reinterpret_cast<T *>(static_cast<uintptr_t>(res));
  }
  template <typename T> inline void destroy_private_data()
  {
   uint64_t res;
   get_private_data(reinterpret_cast<const uint8_t *>(&__uuidof(T)), &res);
   delete reinterpret_cast<T *>(static_cast<uintptr_t>(res));
   set_private_data(reinterpret_cast<const uint8_t *>(&__uuidof(T)), 0);
  }
 };
 RESHADE_DEFINE_INTERFACE_WITH_BASE(device, api_object)
 {
  virtual device_api get_api() const = 0;
  virtual bool check_capability(device_caps capability) const = 0;
  virtual bool check_format_support(format format, resource_usage usage) const = 0;
  virtual bool create_sampler(const sampler_desc &desc, sampler *out_handle) = 0;
  virtual void destroy_sampler(sampler handle) = 0;
  virtual bool create_resource(const resource_desc &desc, const subresource_data *initial_data, resource_usage initial_state, resource *out_handle, void **shared_handle = nullptr) = 0;
  virtual void destroy_resource(resource handle) = 0;
  virtual resource_desc get_resource_desc(resource resource) const = 0;
  virtual bool create_resource_view(resource resource, resource_usage usage_type, const resource_view_desc &desc, resource_view *out_handle) = 0;
  virtual void destroy_resource_view(resource_view handle) = 0;
  virtual resource get_resource_from_view(resource_view view) const = 0;
  virtual resource_view_desc get_resource_view_desc(resource_view view) const = 0;
  virtual bool map_buffer_region(resource resource, uint64_t offset, uint64_t size, map_access access, void **out_data) = 0;
  virtual void unmap_buffer_region(resource resource) = 0;
  virtual bool map_texture_region(resource resource, uint32_t subresource, const subresource_box *box, map_access access, subresource_data *out_data) = 0;
  virtual void unmap_texture_region(resource resource, uint32_t subresource) = 0;
  virtual void update_buffer_region(const void *data, resource resource, uint64_t offset, uint64_t size) = 0;
  virtual void update_texture_region(const subresource_data &data, resource resource, uint32_t subresource, const subresource_box *box = nullptr) = 0;
  virtual bool create_pipeline(pipeline_layout layout, uint32_t subobject_count, const pipeline_subobject *subobjects, pipeline *out_handle) = 0;
  virtual void destroy_pipeline(pipeline handle) = 0;
  virtual bool create_pipeline_layout(uint32_t param_count, const pipeline_layout_param *params, pipeline_layout *out_handle) = 0;
  virtual void destroy_pipeline_layout(pipeline_layout handle) = 0;
  inline bool allocate_descriptor_set(pipeline_layout layout, uint32_t param, descriptor_set *out_handle) { return allocate_descriptor_sets(1, layout, param, out_handle); }
  virtual bool allocate_descriptor_sets(uint32_t count, pipeline_layout layout, uint32_t param, descriptor_set *out_handles) = 0;
  inline void free_descriptor_set(descriptor_set handle) { free_descriptor_sets(1, &handle); }
  virtual void free_descriptor_sets(uint32_t count, const descriptor_set *handles) = 0;
  virtual void get_descriptor_pool_offset(descriptor_set set, uint32_t binding, uint32_t array_offset, descriptor_pool *out_pool, uint32_t *out_offset) const = 0;
  inline void copy_descriptors(const descriptor_set_copy &copy) { copy_descriptor_sets(1, &copy); }
  virtual void copy_descriptor_sets(uint32_t count, const descriptor_set_copy *copies) = 0;
  inline void update_descriptors(const descriptor_set_update &update) { update_descriptor_sets(1, &update); }
  virtual void update_descriptor_sets(uint32_t count, const descriptor_set_update *updates) = 0;
  virtual bool create_query_pool(query_type type, uint32_t size, query_pool *out_handle) = 0;
  virtual void destroy_query_pool(query_pool handle) = 0;
  virtual bool get_query_pool_results(query_pool pool, uint32_t first, uint32_t count, void *results, uint32_t stride) = 0;
  virtual void set_resource_name(resource handle, const char *name) = 0;
  virtual void set_resource_view_name(resource_view handle, const char *name) = 0;
 };
 RESHADE_DEFINE_INTERFACE_WITH_BASE(device_object, api_object)
 {
  virtual device *get_device() = 0;
 };
 enum class indirect_command
 {
  unknown,
  draw,
  draw_indexed,
  dispatch
 };
 RESHADE_DEFINE_INTERFACE_WITH_BASE(command_list, device_object)
 {
  inline void barrier(resource resource, resource_usage old_state, resource_usage new_state) { barrier(1, &resource, &old_state, &new_state); }
  virtual void barrier(uint32_t count, const resource *resources, const resource_usage *old_states, const resource_usage *new_states) = 0;
  virtual void begin_render_pass(uint32_t count, const render_pass_render_target_desc *rts, const render_pass_depth_stencil_desc *ds = nullptr) = 0;
  virtual void end_render_pass() = 0;
  virtual void bind_render_targets_and_depth_stencil(uint32_t count, const resource_view *rtvs, resource_view dsv = { 0 }) = 0;
  virtual void bind_pipeline(pipeline_stage stages, pipeline pipeline) = 0;
  inline void bind_pipeline_state(dynamic_state state, uint32_t value) { bind_pipeline_states(1, &state, &value); }
  virtual void bind_pipeline_states(uint32_t count, const dynamic_state *states, const uint32_t *values) = 0;
  virtual void bind_viewports(uint32_t first, uint32_t count, const viewport *viewports) = 0;
  virtual void bind_scissor_rects(uint32_t first, uint32_t count, const rect *rects) = 0;
  virtual void push_constants(shader_stage stages, pipeline_layout layout, uint32_t param, uint32_t first, uint32_t count, const void *values) = 0;
  virtual void push_descriptors(shader_stage stages, pipeline_layout layout, uint32_t param, const descriptor_set_update &update) = 0;
  inline void bind_descriptor_set(shader_stage stages, pipeline_layout layout, uint32_t param, descriptor_set set) { bind_descriptor_sets(stages, layout, param, 1, &set); }
  virtual void bind_descriptor_sets(shader_stage stages, pipeline_layout layout, uint32_t first, uint32_t count, const descriptor_set *sets) = 0;
  virtual void bind_index_buffer(resource buffer, uint64_t offset, uint32_t index_size) = 0;
  inline void bind_vertex_buffer(uint32_t index, resource buffer, uint64_t offset, uint32_t stride) { bind_vertex_buffers(index, 1, &buffer, &offset, &stride); }
  virtual void bind_vertex_buffers(uint32_t first, uint32_t count, const resource *buffers, const uint64_t *offsets, const uint32_t *strides) = 0;
  virtual void bind_stream_output_buffers(uint32_t first, uint32_t count, const api::resource *buffers, const uint64_t *offsets, const uint64_t *max_sizes) = 0;
  virtual void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) = 0;
  virtual void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) = 0;
  virtual void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) = 0;
  virtual void draw_or_dispatch_indirect(indirect_command type, resource buffer, uint64_t offset, uint32_t draw_count, uint32_t stride) = 0;
  virtual void copy_resource(resource source, resource dest) = 0;
  virtual void copy_buffer_region(resource source, uint64_t source_offset, resource dest, uint64_t dest_offset, uint64_t size) = 0;
  virtual void copy_buffer_to_texture(resource source, uint64_t source_offset, uint32_t row_length, uint32_t slice_height, resource dest, uint32_t dest_subresource, const subresource_box *dest_box = nullptr) = 0;
  virtual void copy_texture_region(resource source, uint32_t source_subresource, const subresource_box *source_box, resource dest, uint32_t dest_subresource, const subresource_box *dest_box, filter_mode filter = filter_mode::min_mag_mip_point) = 0;
  virtual void copy_texture_to_buffer(resource source, uint32_t source_subresource, const subresource_box *source_box, resource dest, uint64_t dest_offset, uint32_t row_length = 0, uint32_t slice_height = 0) = 0;
  virtual void resolve_texture_region(resource source, uint32_t source_subresource, const subresource_box *source_box, resource dest, uint32_t dest_subresource, int32_t dest_x, int32_t dest_y, int32_t dest_z, format format) = 0;
  virtual void clear_depth_stencil_view(resource_view dsv, const float *depth, const uint8_t *stencil, uint32_t rect_count = 0, const rect *rects = nullptr) = 0;
  virtual void clear_render_target_view(resource_view rtv, const float color[4], uint32_t rect_count = 0, const rect *rects = nullptr) = 0;
  virtual void clear_unordered_access_view_uint(resource_view uav, const uint32_t values[4], uint32_t rect_count = 0, const rect *rects = nullptr) = 0;
  virtual void clear_unordered_access_view_float(resource_view uav, const float values[4], uint32_t rect_count = 0, const rect *rects = nullptr) = 0;
  virtual void generate_mipmaps(resource_view srv) = 0;
  virtual void begin_query(query_pool pool, query_type type, uint32_t index) = 0;
  virtual void end_query(query_pool pool, query_type type, uint32_t index) = 0;
  virtual void copy_query_pool_results(query_pool pool, query_type type, uint32_t first, uint32_t count, resource dest, uint64_t dest_offset, uint32_t stride) = 0;
  virtual void begin_debug_event(const char *label, const float color[4] = nullptr) = 0;
  virtual void end_debug_event() = 0;
  virtual void insert_debug_marker(const char *label, const float color[4] = nullptr) = 0;
 };
 enum class command_queue_type
 {
  graphics = 0x1,
  compute = 0x2,
  copy = 0x4
 };
 RESHADE_DEFINE_ENUM_FLAG_OPERATORS(command_queue_type);
 RESHADE_DEFINE_INTERFACE_WITH_BASE(command_queue, device_object)
 {
  virtual command_queue_type get_type() const = 0;
  virtual void wait_idle() const = 0;
  virtual void flush_immediate_command_list() const = 0;
  virtual command_list *get_immediate_command_list() = 0;
  virtual void begin_debug_event(const char *label, const float color[4] = nullptr) = 0;
  virtual void end_debug_event() = 0;
  virtual void insert_debug_marker(const char *label, const float color[4] = nullptr) = 0;
 };
 RESHADE_DEFINE_INTERFACE_WITH_BASE(swapchain, device_object)
 {
  virtual void * get_hwnd() const = 0;
  virtual resource get_back_buffer(uint32_t index) = 0;
  virtual uint32_t get_back_buffer_count() const = 0;
  inline resource get_current_back_buffer() { return get_back_buffer(get_current_back_buffer_index()); }
  virtual uint32_t get_current_back_buffer_index() const = 0;
 };
}
