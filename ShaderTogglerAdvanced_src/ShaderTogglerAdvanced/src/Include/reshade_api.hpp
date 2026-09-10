/*
 * Copyright (C) 2021 Patrick Mours
 * License: https://github.com/crosire/reshade#license
 */
#pragma once
#include "reshade_api_device.hpp"
namespace reshade::api
{
 RESHADE_DEFINE_HANDLE(effect_technique);
 RESHADE_DEFINE_HANDLE(effect_texture_variable);
 RESHADE_DEFINE_HANDLE(effect_uniform_variable);
 RESHADE_DEFINE_INTERFACE_WITH_BASE(effect_runtime, swapchain)
 {
  virtual command_queue *get_command_queue() = 0;
  virtual void render_effects(command_list *cmd_list, resource_view rtv, resource_view rtv_srgb = { 0 }) = 0;
  virtual bool capture_screenshot(uint8_t *pixels) = 0;
  virtual void get_screenshot_width_and_height(uint32_t *out_width, uint32_t *out_height) const = 0;
  virtual bool is_key_down(uint32_t keycode) const = 0;
  virtual bool is_key_pressed(uint32_t keycode) const = 0;
  virtual bool is_key_released(uint32_t keycode) const = 0;
  virtual bool is_mouse_button_down(uint32_t button) const = 0;
  virtual bool is_mouse_button_pressed(uint32_t button) const = 0;
  virtual bool is_mouse_button_released(uint32_t button) const = 0;
  virtual void get_mouse_cursor_position(uint32_t *out_x, uint32_t *out_y, int16_t *out_wheel_delta = nullptr) const = 0;
  virtual void enumerate_uniform_variables(const char *effect_name, void(*callback)(effect_runtime *runtime, effect_uniform_variable variable, void *user_data), void *user_data) = 0;
  template <typename F>
  inline void enumerate_uniform_variables(const char *effect_name, F lambda) {
   enumerate_uniform_variables(effect_name, [](effect_runtime *runtime, effect_uniform_variable variable, void *user_data) { static_cast<F *>(user_data)->operator()(runtime, variable); }, &lambda);
  }
  virtual effect_uniform_variable find_uniform_variable(const char *effect_name, const char *variable_name) const = 0;
  virtual void get_uniform_variable_type(effect_uniform_variable variable, format *out_base_type, uint32_t *out_rows = nullptr, uint32_t *out_columns = nullptr, uint32_t *out_array_length = nullptr) const = 0;
  virtual void get_uniform_variable_name(effect_uniform_variable variable, char *name, size_t *length) const = 0;
  template <size_t SIZE>
  inline void get_uniform_variable_name(effect_uniform_variable variable, char(&name)[SIZE]) const {
   size_t length = SIZE;
   get_uniform_variable_name(variable, name, &length);
  }
  virtual bool get_annotation_bool_from_uniform_variable(effect_uniform_variable variable, const char *name, bool *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_float_from_uniform_variable(effect_uniform_variable variable, const char *name, float *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_int_from_uniform_variable(effect_uniform_variable variable, const char *name, int32_t *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_uint_from_uniform_variable(effect_uniform_variable variable, const char *name, uint32_t *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_string_from_uniform_variable(effect_uniform_variable variable, const char *name, char *value, size_t *length) const = 0;
  template <size_t SIZE>
  inline bool get_annotation_string_from_uniform_variable(effect_uniform_variable variable, const char *name, char(&value)[SIZE]) const {
   size_t length = SIZE;
   return get_annotation_string_from_uniform_variable(variable, name, value, &length);
  }
  virtual void get_uniform_value_bool(effect_uniform_variable variable, bool *values, size_t count, size_t array_index = 0) const = 0;
  virtual void get_uniform_value_float(effect_uniform_variable variable, float *values, size_t count, size_t array_index = 0) const = 0;
  virtual void get_uniform_value_int(effect_uniform_variable variable, int32_t *values, size_t count, size_t array_index = 0) const = 0;
  virtual void get_uniform_value_uint(effect_uniform_variable variable, uint32_t *values, size_t count, size_t array_index = 0) const = 0;
  virtual void set_uniform_value_bool(effect_uniform_variable variable, const bool *values, size_t count, size_t array_index = 0) = 0;
  inline void set_uniform_value_bool(effect_uniform_variable variable, bool x, bool y = bool(0), bool z = bool(0), bool w = bool(0)) {
   const bool values[4] = { x, y, z, w };
   set_uniform_value_bool(variable, values, 4);
  }
  virtual void set_uniform_value_float(effect_uniform_variable variable, const float *values, size_t count, size_t array_index = 0) = 0;
  inline void set_uniform_value_float(effect_uniform_variable variable, float x, float y = float(0), float z = float(0), float w = float(0)) {
   const float values[4] = { x, y, z, w };
   set_uniform_value_float(variable, values, 4);
  }
  virtual void set_uniform_value_int(effect_uniform_variable variable, const int32_t *values, size_t count, size_t array_index = 0) = 0;
  inline void set_uniform_value_int(effect_uniform_variable variable, int32_t x, int32_t y = int32_t(0), int32_t z = int32_t(0), int32_t w = int32_t(0)) {
   const int32_t values[4] = { x, y, z, w };
   set_uniform_value_int(variable, values, 4);
  }
  virtual void set_uniform_value_uint(effect_uniform_variable variable, const uint32_t *values, size_t count, size_t array_index = 0) = 0;
  inline void set_uniform_value_uint(effect_uniform_variable variable, uint32_t x, uint32_t y = uint32_t(0), uint32_t z = uint32_t(0), uint32_t w = uint32_t(0)) {
   const uint32_t values[4] = { x, y, z, w };
   set_uniform_value_uint(variable, values, 4);
  }
  virtual void enumerate_texture_variables(const char *effect_name, void(*callback)(effect_runtime *runtime, effect_texture_variable variable, void *user_data), void *user_data) = 0;
  template <typename F>
  inline void enumerate_texture_variables(const char *effect_name, F lambda) {
   enumerate_texture_variables(effect_name, [](effect_runtime *runtime, effect_texture_variable variable, void *user_data) { static_cast<F *>(user_data)->operator()(runtime, variable); }, &lambda);
  }
  virtual effect_texture_variable find_texture_variable(const char *effect_name, const char *variable_name) const = 0;
  virtual void get_texture_variable_name(effect_texture_variable variable, char *name, size_t *length) const = 0;
  template <size_t SIZE>
  inline void get_texture_variable_name(effect_texture_variable variable,char(&name)[SIZE]) const {
   size_t length = SIZE;
   get_texture_variable_name(variable, name, &length);
  }
  virtual bool get_annotation_bool_from_texture_variable(effect_texture_variable variable, const char *name, bool *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_float_from_texture_variable(effect_texture_variable variable, const char *name, float *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_int_from_texture_variable(effect_texture_variable variable, const char *name, int32_t *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_uint_from_texture_variable(effect_texture_variable variable, const char *name, uint32_t *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_string_from_texture_variable(effect_texture_variable variable, const char *name, char *value, size_t *length) const = 0;
  template <size_t SIZE>
  inline bool get_annotation_string_from_texture_variable(effect_texture_variable variable, const char *name, char(&value)[SIZE]) const {
   size_t length = SIZE;
   return get_annotation_string_from_texture_variable(variable, name, value, &length);
  }
  virtual void update_texture(effect_texture_variable variable, const uint32_t width, const uint32_t height, const uint8_t *pixels) = 0;
  virtual void get_texture_binding(effect_texture_variable variable, resource_view *out_srv, resource_view *out_srv_srgb = nullptr) const = 0;
  virtual void update_texture_bindings(const char *semantic, resource_view srv, resource_view srv_srgb = { 0 }) = 0;
  virtual void enumerate_techniques(const char *effect_name, void(*callback)(effect_runtime *runtime, effect_technique technique, void *user_data), void *user_data) = 0;
  template <typename F>
  inline void enumerate_techniques(const char *effect_name, F lambda) {
   enumerate_techniques(effect_name, [](effect_runtime *runtime, effect_technique technique, void *user_data) { static_cast<F *>(user_data)->operator()(runtime, technique); }, &lambda);
  }
  virtual effect_technique find_technique(const char *effect_name, const char *technique_name) = 0;
  virtual void get_technique_name(effect_technique technique, char *name, size_t *length) const = 0;
  template <size_t SIZE>
  inline void get_technique_name(effect_technique technique, char(&name)[SIZE]) const {
   size_t length = SIZE;
   get_technique_name(technique, name, &length);
  }
  virtual bool get_annotation_bool_from_technique(effect_technique technique, const char *name, bool *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_float_from_technique(effect_technique technique, const char *name, float *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_int_from_technique(effect_technique technique, const char *name, int32_t *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_uint_from_technique(effect_technique technique, const char *name, uint32_t *values, size_t count, size_t array_index = 0) const = 0;
  virtual bool get_annotation_string_from_technique(effect_technique technique, const char *name, char *value, size_t *length) const = 0;
  template <size_t SIZE>
  inline bool get_annotation_string_from_technique(effect_technique technique, const char *name, char(&value)[SIZE]) const {
   size_t length = SIZE;
   return get_annotation_string_from_technique(technique, name, value, &length);
  }
  virtual bool get_technique_state(effect_technique technique) const = 0;
  virtual void set_technique_state(effect_technique technique, bool enabled) = 0;
  virtual bool get_preprocessor_definition(const char *name, char *value, size_t *length) const = 0;
  template <size_t SIZE>
  inline bool get_preprocessor_definition(const char *name, char(&value)[SIZE]) const {
   size_t length = SIZE;
   return get_preprocessor_definition(name, value, &length);
  }
  virtual void set_preprocessor_definition(const char *name, const char *value) = 0;
 };
}
