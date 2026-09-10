/*
 * Copyright (C) 2021 Patrick Mours
 * License: https://github.com/crosire/reshade#license
 */
#pragma once
#include "reshade_events.hpp"
#include "reshade_overlay.hpp"
#include <charconv>
#include <Windows.h>
#define RESHADE_API_VERSION 2
extern "C" BOOL WINAPI K32EnumProcessModules(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
namespace reshade
{
 namespace internal
 {
  inline HMODULE &get_reshade_module_handle()
  {
   static HMODULE handle = nullptr;
   if (handle == nullptr)
   {
    HMODULE modules[1024]; DWORD num = 0;
    if (K32EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &num))
    {
     if (num > sizeof(modules))
      num = sizeof(modules);
     for (DWORD i = 0; i < num / sizeof(HMODULE); ++i)
     {
      if (GetProcAddress(modules[i], "ReShadeRegisterAddon") &&
       GetProcAddress(modules[i], "ReShadeUnregisterAddon"))
      {
       handle = modules[i];
       break;
      }
     }
    }
   }
   return handle;
  }
  inline HMODULE &get_current_module_handle()
  {
   static HMODULE handle = nullptr;
   return handle;
  }
 }
 inline void log_message(int level, const char *message)
 {
  static const auto func = reinterpret_cast<void(*)(HMODULE, int, const char *)>(
   GetProcAddress(internal::get_reshade_module_handle(), "ReShadeLogMessage"));
  func(internal::get_current_module_handle(), level, message);
 }
 inline bool config_get_value(api::effect_runtime *runtime, const char *section, const char *key, char *value, size_t *length)
 {
  static const auto func = reinterpret_cast<bool(*)(HMODULE, api::effect_runtime *, const char *, const char *, char *, size_t *)>(
   GetProcAddress(internal::get_reshade_module_handle(), "ReShadeGetConfigValue"));
  return func(internal::get_current_module_handle(), runtime, section, key, value, length);
 }
 template <typename T>
 inline bool config_get_value(api::effect_runtime *runtime, const char *section, const char *key, T &value)
 {
  char value_string[32] = ""; size_t value_length = sizeof(value_string) - 1;
  if (!config_get_value(runtime, section, key, value_string, &value_length))
   return false;
  return std::from_chars(value_string, value_string + value_length, value).ec == std::errc {};
 }
 template <>
 inline bool config_get_value<bool>(api::effect_runtime *runtime, const char *section, const char *key, bool &value)
 {
  int value_int = 0;
  if (!config_get_value<int>(runtime, section, key, value_int))
   return false;
  value = value_int != 0;
  return true;
 }
 inline void config_set_value(api::effect_runtime *runtime, const char *section, const char *key, const char *value)
 {
  static const auto func = reinterpret_cast<void(*)(HMODULE, api::effect_runtime *, const char *, const char *, const char *)>(
   GetProcAddress(internal::get_reshade_module_handle(), "ReShadeSetConfigValue"));
  func(internal::get_current_module_handle(), runtime, section, key, value);
 }
 template <typename T>
 inline void config_set_value(api::effect_runtime *runtime, const char *section, const char *key, const T &value)
 {
  char value_string[32] = "";
  std::to_chars(value_string, value_string + sizeof(value_string) - 1, value);
  config_set_value(runtime, section, key, static_cast<const char *>(value_string));
 }
 template <>
 inline void config_set_value<bool>(api::effect_runtime *runtime, const char *section, const char *key, const bool &value)
 {
  config_set_value<int>(runtime, section, key, value ? 1 : 0);
 }
 inline bool register_addon(HMODULE module)
 {
  internal::get_current_module_handle() = module;
  const HMODULE reshade_module = internal::get_reshade_module_handle();
  if (reshade_module == nullptr)
   return false;
  const auto func = reinterpret_cast<bool(*)(HMODULE, uint32_t)>(
   GetProcAddress(reshade_module, "ReShadeRegisterAddon"));
  if (!func(module, RESHADE_API_VERSION))
   return false;
#if defined(IMGUI_VERSION_NUM)
  const auto imgui_func = reinterpret_cast<const imgui_function_table *(*)(uint32_t)>(
   GetProcAddress(reshade_module, "ReShadeGetImGuiFunctionTable"));
  if (imgui_func == nullptr || !(imgui_function_table_instance() = imgui_func(IMGUI_VERSION_NUM)))
   return false;
#endif
  return true;
 }
 inline void unregister_addon(HMODULE module)
 {
  const HMODULE reshade_module = internal::get_reshade_module_handle();
  if (reshade_module == nullptr)
   return;
  const auto func = reinterpret_cast<bool(*)(HMODULE)>(
   GetProcAddress(reshade_module, "ReShadeUnregisterAddon"));
  func(module);
 }
 template <reshade::addon_event ev>
 inline void register_event(typename reshade::addon_event_traits<ev>::decl callback)
 {
  static const auto func = reinterpret_cast<void(*)(reshade::addon_event, void *)>(
   GetProcAddress(internal::get_reshade_module_handle(), "ReShadeRegisterEvent"));
  func(ev, static_cast<void *>(callback));
 }
 template <reshade::addon_event ev>
 inline void unregister_event(typename reshade::addon_event_traits<ev>::decl callback)
 {
  static const auto func = reinterpret_cast<void(*)(reshade::addon_event, void *)>(
   GetProcAddress(internal::get_reshade_module_handle(), "ReShadeUnregisterEvent"));
  func(ev, static_cast<void *>(callback));
 }
 inline void register_overlay(const char *title, void(*callback)(reshade::api::effect_runtime *runtime))
 {
  static const auto func = reinterpret_cast<void(*)(const char *, void(*)(reshade::api::effect_runtime *))>(
   GetProcAddress(internal::get_reshade_module_handle(), "ReShadeRegisterOverlay"));
  func(title, callback);
 }
 inline void unregister_overlay(const char *title, void(*callback)(reshade::api::effect_runtime *runtime))
 {
  static const auto func = reinterpret_cast<void(*)(const char *, void(*)(reshade::api::effect_runtime *))>(
   GetProcAddress(internal::get_reshade_module_handle(), "ReShadeUnregisterOverlay"));
  func(title, callback);
 }
}
