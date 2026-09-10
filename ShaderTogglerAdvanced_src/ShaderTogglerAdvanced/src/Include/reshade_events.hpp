/*
 * Copyright (C) 2021 Patrick Mours
 * License: https://github.com/crosire/reshade#license
 */
#pragma once
#include "reshade_api.hpp"
namespace reshade
{
 enum class addon_event
 {
  init_device,
  destroy_device,
  init_command_list,
  destroy_command_list,
  init_command_queue,
  destroy_command_queue,
  init_swapchain,
  create_swapchain,
  destroy_swapchain,
  init_effect_runtime,
  destroy_effect_runtime,
  init_sampler,
  create_sampler,
  destroy_sampler,
  init_resource,
  create_resource,
  destroy_resource,
  init_resource_view,
  create_resource_view,
  destroy_resource_view,
  map_buffer_region,
  unmap_buffer_region,
  map_texture_region,
  unmap_texture_region,
  update_buffer_region,
  update_texture_region,
  init_pipeline,
  create_pipeline,
  destroy_pipeline,
  init_pipeline_layout,
  create_pipeline_layout,
  destroy_pipeline_layout,
  copy_descriptor_sets,
  update_descriptor_sets,
  init_query_pool,
  create_query_pool,
  destroy_query_pool,
  get_query_pool_results,
  barrier,
  begin_render_pass,
  end_render_pass,
  bind_render_targets_and_depth_stencil,
  bind_pipeline,
  bind_pipeline_states,
  bind_viewports,
  bind_scissor_rects,
  push_constants,
  push_descriptors,
  bind_descriptor_sets,
  bind_index_buffer,
  bind_vertex_buffers,
  bind_stream_output_buffers,
  draw,
  draw_indexed,
  dispatch,
  draw_or_dispatch_indirect,
  copy_resource,
  copy_buffer_region,
  copy_buffer_to_texture,
  copy_texture_region,
  copy_texture_to_buffer,
  resolve_texture_region,
  clear_depth_stencil_view,
  clear_render_target_view,
  clear_unordered_access_view_uint,
  clear_unordered_access_view_float,
  generate_mipmaps,
  begin_query,
  end_query,
  copy_query_pool_results,
  reset_command_list,
  close_command_list,
  execute_command_list,
  execute_secondary_command_list,
  present,
  reshade_present,
  reshade_begin_effects,
  reshade_finish_effects,
  reshade_reloaded_effects,
  reshade_set_uniform_value,
  reshade_set_technique_state,
  reshade_overlay,
#ifdef RESHADE_ADDON
  max
#endif
 };
 template <addon_event ev>
 struct addon_event_traits;
#define RESHADE_DEFINE_ADDON_EVENT_TRAITS(ev,ret,...) \
 template <> \
 struct addon_event_traits<ev> { \
  using decl = ret(*)(__VA_ARGS__); \
  using type = ret; \
 }
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_device, void, api::device *device);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_device, void, api::device *device);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_command_list, void, api::command_list *cmd_list);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_command_list, void, api::command_list *cmd_list);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_command_queue, void, api::command_queue *queue);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_command_queue, void, api::command_queue *queue);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_swapchain, void, api::swapchain *swapchain);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::create_swapchain, bool, api::resource_desc &back_buffer_desc, void *hwnd);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_swapchain, void, api::swapchain *swapchain);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_effect_runtime, void, api::effect_runtime *runtime);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_effect_runtime, void, api::effect_runtime *runtime);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_sampler, void, api::device *device, const api::sampler_desc &desc, api::sampler sampler);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::create_sampler, bool, api::device *device, api::sampler_desc &desc);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_sampler, void, api::device *device, api::sampler sampler);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_resource, void, api::device *device, const api::resource_desc &desc, const api::subresource_data *initial_data, api::resource_usage initial_state, api::resource resource);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::create_resource, bool, api::device *device, api::resource_desc &desc, api::subresource_data *initial_data, api::resource_usage initial_state);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_resource, void, api::device *device, api::resource resource);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_resource_view, void, api::device *device, api::resource resource, api::resource_usage usage_type, const api::resource_view_desc &desc, api::resource_view view);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::create_resource_view, bool, api::device *device, api::resource resource, api::resource_usage usage_type, api::resource_view_desc &desc);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_resource_view, void, api::device *device, api::resource_view view);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::map_buffer_region, void, api::device *device, api::resource resource, uint64_t offset, uint64_t size, api::map_access access, void **data);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::unmap_buffer_region, void, api::device *device, api::resource resource);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::map_texture_region, void, api::device *device, api::resource resource, uint32_t subresource, const api::subresource_box *box, api::map_access access, api::subresource_data *data);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::unmap_texture_region, void, api::device *device, api::resource resource, uint32_t subresource);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::update_buffer_region, bool, api::device *device, const void *data, api::resource resource, uint64_t offset, uint64_t size);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::update_texture_region, bool, api::device *device, const api::subresource_data &data, api::resource resource, uint32_t subresource, const api::subresource_box *box);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_pipeline, void, api::device *device, api::pipeline_layout layout, uint32_t subobject_count, const api::pipeline_subobject *subobjects, api::pipeline pipeline);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::create_pipeline, bool, api::device *device, api::pipeline_layout layout, uint32_t subobject_count, const api::pipeline_subobject *subobjects);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_pipeline, void, api::device *device, api::pipeline pipeline);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_pipeline_layout, void, api::device *device, uint32_t param_count, const api::pipeline_layout_param *params, api::pipeline_layout layout);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::create_pipeline_layout, bool, api::device *device, uint32_t param_count, const api::pipeline_layout_param *params);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_pipeline_layout, void, api::device *device, api::pipeline_layout layout);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::copy_descriptor_sets, bool, api::device *device, uint32_t count, const api::descriptor_set_copy *copies);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::update_descriptor_sets, bool, api::device *device, uint32_t count, const api::descriptor_set_update *updates);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::init_query_pool, void, api::device *device, api::query_type type, uint32_t size, api::query_pool pool);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::create_query_pool, bool, api::device *device, api::query_type type, uint32_t &size);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::destroy_query_pool, void, api::device *device, api::query_pool pool);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::get_query_pool_results, bool, api::device *device, api::query_pool pool, uint32_t first, uint32_t count, void *results, uint32_t stride);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::barrier, void, api::command_list *cmd_list, uint32_t count, const api::resource *resources, const api::resource_usage *old_states, const api::resource_usage *new_states);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::begin_render_pass, void, api::command_list *cmd_list, uint32_t count, const api::render_pass_render_target_desc *rts, const api::render_pass_depth_stencil_desc *ds);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::end_render_pass, void, api::command_list *cmd_list);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_render_targets_and_depth_stencil, void, api::command_list *cmd_list, uint32_t count, const api::resource_view *rtvs, api::resource_view dsv);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_pipeline, void, api::command_list *cmd_list, api::pipeline_stage stages, api::pipeline pipeline);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_pipeline_states, void, api::command_list *cmd_list, uint32_t count, const api::dynamic_state *states, const uint32_t *values);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_viewports, void, api::command_list *cmd_list, uint32_t first, uint32_t count, const api::viewport *viewports);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_scissor_rects, void, api::command_list *cmd_list, uint32_t first, uint32_t count, const api::rect *rects);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::push_constants, void, api::command_list *cmd_list, api::shader_stage stages, api::pipeline_layout layout, uint32_t layout_param, uint32_t first, uint32_t count, const uint32_t *values);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::push_descriptors, void, api::command_list *cmd_list, api::shader_stage stages, api::pipeline_layout layout, uint32_t layout_param, const api::descriptor_set_update &update);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_descriptor_sets, void, api::command_list *cmd_list, api::shader_stage stages, api::pipeline_layout layout, uint32_t first, uint32_t count, const api::descriptor_set *sets);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_index_buffer, void, api::command_list *cmd_list, api::resource buffer, uint64_t offset, uint32_t index_size);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_vertex_buffers, void, api::command_list *cmd_list, uint32_t first, uint32_t count, const api::resource *buffers, const uint64_t *offsets, const uint32_t *strides);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::bind_stream_output_buffers, void, api::command_list *cmd_list, uint32_t first, uint32_t count, const api::resource *buffers, const uint64_t *offsets, const uint64_t *max_sizes);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::draw, bool, api::command_list *cmd_list, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::draw_indexed, bool, api::command_list *cmd_list, uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::dispatch, bool, api::command_list *cmd_list, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::draw_or_dispatch_indirect, bool, api::command_list *cmd_list, api::indirect_command type, api::resource buffer, uint64_t offset, uint32_t draw_count, uint32_t stride);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::copy_resource, bool, api::command_list *cmd_list, api::resource source, api::resource dest);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::copy_buffer_region, bool, api::command_list *cmd_list, api::resource source, uint64_t source_offset, api::resource dest, uint64_t dest_offset, uint64_t size);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::copy_buffer_to_texture, bool, api::command_list *cmd_list, api::resource source, uint64_t source_offset, uint32_t row_length, uint32_t slice_height, api::resource dest, uint32_t dest_subresource, const api::subresource_box *dest_box);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::copy_texture_region, bool, api::command_list *cmd_list, api::resource source, uint32_t source_subresource, const api::subresource_box *source_box, api::resource dest, uint32_t dest_subresource, const api::subresource_box *dest_box, api::filter_mode filter);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::copy_texture_to_buffer, bool, api::command_list *cmd_list, api::resource source, uint32_t source_subresource, const api::subresource_box *source_box, api::resource dest, uint64_t dest_offset, uint32_t row_length, uint32_t slice_height);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::resolve_texture_region, bool, api::command_list *cmd_list, api::resource source, uint32_t source_subresource, const api::subresource_box *source_box, api::resource dest, uint32_t dest_subresource, int32_t dest_x, int32_t dest_y, int32_t dest_z, api::format format);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::clear_depth_stencil_view, bool, api::command_list *cmd_list, api::resource_view dsv, const float *depth, const uint8_t *stencil, uint32_t rect_count, const api::rect *rects);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::clear_render_target_view, bool, api::command_list *cmd_list, api::resource_view rtv, const float color[4], uint32_t rect_count, const api::rect *rects);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::clear_unordered_access_view_uint, bool, api::command_list *cmd_list, api::resource_view uav, const uint32_t values[4], uint32_t rect_count, const api::rect *rects);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::clear_unordered_access_view_float, bool, api::command_list *cmd_list, api::resource_view uav, const float values[4], uint32_t rect_count, const api::rect *rects);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::generate_mipmaps, bool, api::command_list *cmd_list, api::resource_view srv);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::begin_query, bool, api::command_list *cmd_list, api::query_pool pool, api::query_type type, uint32_t index);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::end_query, bool, api::command_list *cmd_list, api::query_pool pool, api::query_type type, uint32_t index);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::copy_query_pool_results, bool, api::command_list *cmd_list, api::query_pool pool, api::query_type type, uint32_t first, uint32_t count, api::resource dest, uint64_t dest_offset, uint32_t stride);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::reset_command_list, void, api::command_list *cmd_list);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::close_command_list, void, api::command_list *cmd_list);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::execute_command_list, void, api::command_queue *queue, api::command_list *cmd_list);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::execute_secondary_command_list, void, api::command_list *cmd_list, api::command_list *secondary_cmd_list);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::present, void, api::command_queue *queue, api::swapchain *swapchain, const api::rect *source_rect, const api::rect *dest_rect, uint32_t dirty_rect_count, const api::rect *dirty_rects);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::reshade_present, void, api::effect_runtime *runtime);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::reshade_begin_effects, void, api::effect_runtime *runtime, api::command_list *cmd_list, api::resource_view rtv, api::resource_view rtv_srgb);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::reshade_finish_effects, void, api::effect_runtime *runtime, api::command_list *cmd_list, api::resource_view rtv, api::resource_view rtv_srgb);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::reshade_reloaded_effects, void, api::effect_runtime *runtime);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::reshade_set_uniform_value, bool, api::effect_runtime *runtime, api::effect_uniform_variable variable, const void *data, size_t size);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::reshade_set_technique_state, bool, api::effect_runtime *runtime, api::effect_technique technique, bool enabled);
 RESHADE_DEFINE_ADDON_EVENT_TRAITS(addon_event::reshade_overlay, void, api::effect_runtime *runtime);
}
