///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – A shader toggler add-on for ReShade 5+
// which allows you to define groups of game shaders to toggle them on/off
// with one key press.
//
// Based on the original ShaderToggler by Frans 'Otis_Inf' Bouma.
// (c) Frans 'Otis_Inf' Bouma. All rights reserved.
//
// https://github.com/FransBouma/ShaderToggler
//
// Modifications
// (c) 2026 Sven 'Gametism' Königsmann. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H 
#define ImTextureID unsigned long long
#include <imgui.h>
#include <reshade.hpp>
#include "crc32_hash.hpp"
#include "SmartDisableD3D12.h"
#include "SmartDisableOther.h"
#include "ShaderManager.h"
#include "CDataFile.h"
#include "ToggleGroup.h"
#include "GroupMerge.h"
#include "KeyData.h"
#include "DiagnosticLog.h"
#include "UiFontBridge.h"
#include <vector>
#include <filesystem>
#include <Windows.h>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "SafeIniSave.h"
bool saveShaderTogglerIniFile();
using namespace reshade::api;
using namespace ShaderToggler;
extern "C" __declspec(dllexport) const char *NAME = "Shader Toggler Advanced";
extern "C" __declspec(dllexport) const char *DESCRIPTION = "Add-on which allows you to define groups of game shaders to toggle on/off with one key press.";
extern "C" __declspec(dllexport) const char STA_LICENSE_NOTICE[] =
 "Copyright (c) 2026 Sven \"Gametism\" K\xC3\xB6nigsmann. All Rights Reserved. "
 "These new modifications are proprietary. Unauthorized copying, reuse, modification, "
 "redistribution, derivative works, and AI-assisted reuse of this code are prohibited "
 "without prior written permission. Use of the mod as supplied is permitted. "
 "Original and third-party code remains subject to its existing license notices.";
struct __declspec(uuid("038B03AA-4C75-443B-A695-752D80797037")) CommandListDataContainer
{
 uint64_t activePixelShaderPipeline = 0;
 uint64_t activeVertexShaderPipeline = 0;
 uint64_t activeComputeShaderPipeline = 0;
    uint64_t originalPixelShaderPipeline = 0;
    uint64_t originalVertexShaderPipeline = 0;
    uint64_t originalComputeShaderPipeline = 0;
};
#define FRAMECOUNT_COLLECTION_PHASE_DEFAULT 250
#define HASH_FILE_NAME L"ShaderToggler.ini"
static smart::Settings g_smartSettings;
static smart::Settings g_smartSettings10;
static smart::Settings g_smartSettings11;
static smart::Settings g_smartSettings12;
static smart::Settings g_smartSettingsVK;
static smart::Settings g_smartSettingsGL;
static smart::Settings& smartSettingsFor(device* device)
{
    if (device && device->get_api() == device_api::d3d10) return g_smartSettings10;
    if (device && device->get_api() == device_api::d3d11) return g_smartSettings11;
    if (device && device->get_api() == device_api::d3d12) return g_smartSettings12;
    if (smart::other::isVK(device)) return g_smartSettingsVK;
    if (smart::other::isGL(device)) return g_smartSettingsGL;
    return g_smartSettings;
}
static void cancelSmartPreviews()
{
    g_smartSettings.cancel();
    g_smartSettings10.cancel();
    g_smartSettings11.cancel();
    g_smartSettings12.cancel();
    g_smartSettingsVK.cancel();
    g_smartSettingsGL.cancel();
}
struct SmartDrawRequest
{
    uint64_t handle = 0;
    uint32_t hash = 0;
    smart::Choice choice;
};
static bool g_syndicateProfile = false;
static std::array<smart::Method, 5> g_smartTrialOrder = smart::candidates(smart::Method::TransparentBlack);
static size_t g_smartTrialIndex = 0;
static float g_smartCustomColour[4] = {0, 0, 0, 0};
static bool g_addonRegistered = false;
static ShaderManager g_pixelShaderManager;
static ShaderManager g_vertexShaderManager;
static ShaderManager g_computeShaderManager;
static KeyData g_keyCollector;
static std::atomic_uint32_t g_activeCollectorFrameCounter = 0;
static std::vector<ToggleGroup> g_toggleGroups;
static void logFinderTransition(const char* action,int group,size_t selected,size_t total,bool preview,bool includeCompute,const std::set<finder::Signature>& rules) noexcept
{
    diagnostics::Write("[FINDER] Transition=%s group=%d preview=%u candidate=%zu/%zu compute_enabled=%u members=%zu",
        action,group,preview, total?selected+1:0,total,includeCompute,rules.size());
    for(const auto& s:rules)
    {
        if(s[10] || (s[1]>=4 && s[7]==2)) diagnostics::Write("[FINDER] Preview member type=graphics rule=2,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
            s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7],s[8],s[9],s[10],s[11]);
        else diagnostics::Write("[FINDER] Preview member type=%s rule=1,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
            finder::compute(s)?"compute":"graphics",s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7],s[8],s[9]);
    }
}
static finder::Engine g_effectFinder{logFinderTransition};
static effect_runtime* g_finderRuntime = nullptr;
static int g_finderGroup = -1;
static std::array<bool, 5> g_finderKeysDown{};
static bool g_finderKeyboardCaptured = false;
static int g_finderMode = 0;
static std::atomic_bool g_originalShaderMatching{true};
static int g_finderFocusGroup = -1;
static std::set<int> g_finderKeepGroups;
static bool g_mergePanelOpen=false;
static int g_mergeDestination=-1;
static std::set<int> g_mergeSources;
static bool g_mergeRemoveSources=true;
static std::string g_mergeMessage;
static void publishEffectFilters();
static std::atomic_int g_toggleGroupIdKeyBindingEditing = -1;
static std::atomic_int g_toggleGroupIdTimedTriggerKeyEditing = -1;
static std::atomic_int g_toggleGroupTimedTriggerKeySlotEditing = -1;
static std::atomic_int g_toggleGroupIdTimedSuppressionKeyEditing = -1;
static std::atomic_int g_toggleGroupTimedSuppressionKeySlotEditing = -1;
static std::atomic_int g_toggleGroupIdShaderEditing = -1;
static std::atomic_int g_globalSuspendHotkeySlotEditing = -1;
static std::atomic_int g_globalRestoreHotkeySlotEditing = -1;
static std::vector<KeyData> g_globalSuspendHotkeys;
static std::vector<KeyData> g_globalRestoreHotkeys;
static bool g_allToggleGroupsSuspended = false;
static std::chrono::steady_clock::time_point g_globalSuspensionStarted;
static std::unordered_set<int> g_pendingSuspendedGroupToggles;
static float g_overlayOpacity = 1.0f;
static int g_startValueFramecountCollectionPhase = FRAMECOUNT_COLLECTION_PHASE_DEFAULT;
static std::filesystem::path g_iniFileName;
static std::unordered_map<int, bool> g_groupHotkeyWasDown;
static std::unordered_map<int, std::vector<uint32_t>> g_groupMenuInputs;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupHotkeyLastToggleTime;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupLastTimedTriggerTime;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupTimedVisibleSince;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupTimedFadeOutStart;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupLastTimedSuppressionInputTime;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupStartupActivationStartTime;
static const int g_groupHotkeyDebounceMs = 150;
static void publishEffectFilters()
{
    std::set<finder::Signature> rules;
    if (!g_allToggleGroupsSuspended)
        for (const auto& group : g_toggleGroups)
            if (group.isActive() && group.getId() != g_toggleGroupIdShaderEditing)
                rules.insert(group.getEffectFilters().begin(), group.getEffectFilters().end());
    g_effectFinder.publish(std::move(rules));
}
static void closeEffectFinder()
{
    g_effectFinder.close();
    g_finderGroup = -1;
    g_finderRuntime = nullptr;
    g_finderFocusGroup = -1;
    g_finderKeepGroups.clear();
}
static void onFinderDestroyRuntime(effect_runtime* runtime)
{
    if (runtime == g_finderRuntime) closeEffectFinder();
}
static void onFinderDestroyDevice(device* dev)
{
    if (g_effectFinder.isolates(reinterpret_cast<uintptr_t>(dev))) closeEffectFinder();
}
static std::chrono::steady_clock::time_point s_lastNP1, s_lastNP2, s_lastNP4, s_lastNP5, s_lastNP7, s_lastNP8;
static std::chrono::steady_clock::time_point s_holdStartNP1, s_holdStartNP2, s_holdStartNP4, s_holdStartNP5, s_holdStartNP7, s_holdStartNP8;
static bool s_np1Held = false, s_np2Held = false, s_np4Held = false, s_np5Held = false, s_np7Held = false, s_np8Held = false;
static bool s_prevNP1Down = false;
static bool s_prevNP2Down = false;
static bool s_prevNP3Down = false;
static bool s_prevNP4Down = false;
static bool s_prevNP5Down = false;
static bool s_prevNP6Down = false;
static bool s_prevNP7Down = false;
static bool s_prevNP8Down = false;
static bool s_prevNP9Down = false;
static const int s_holdRepeatStartMs = 200;
static const int s_holdRepeatMidMs = 120;
static const int s_holdRepeatFastMs = 70;
static const int s_holdRepeatVeryFastMs = 35;
static const char* GT_CREATOR = "Gametism";
static const char* GT_CACHE_KEY = "CacheStamp";
static const char* GT_SIG_A = "STA";
static const char* GT_SIG_B = "Gametism";
static const char* GT_SIG_C = "ShaderToggler";
static const char* GT_SIG_D = "Advanced";
static const uint64_t GT_SIG_SEED = 0x9E3779B185EBCA87ull;
static const char* GT_HEADER =
 "; ==========================================\n"
 "; ShaderToggler Advanced\n"
 "; Created by Gametism\n"
 "; ==========================================\n\n";
static const char* GT_FOOTER =
 "\n; ==========================================\n"
 "; End of file\n"
 "; ==========================================\n";
static int g_uiScalePercent=0;
static char g_groupSearch[160] = {};
static int g_groupListFilter=0;
static int g_standaloneFinderDestination=-1;
static int g_uiSaveResult=0;
static std::chrono::steady_clock::time_point g_overlayMouseCaptureLastSeen;
static void logGroupConfiguration(const ToggleGroup& group)
{
 diagnostics::Write("[GROUP] id=%d name=%.120s active=%u startup=%u hold=%u inverted=%u timed=%u key=%08X shaders(P/V/C)=%zu/%zu/%zu",
  group.getId(), group.getName().c_str(), group.isActive(), group.isActiveAtStartup(),
  group.isHoldMode(), group.isHoldInverted(), group.isTimedMode(),
  static_cast<unsigned>(group.getToggleKey().toInt()), group.getPixelShaderHashes().size(),
  group.getVertexShaderHashes().size(), group.getComputeShaderHashes().size());
 const auto logHashes = [&](const char* stage, const std::unordered_set<uint32_t>& hashes)
 {
  if (hashes.empty()) return;
  char list[256]{};
  size_t used = 0, count = 0;
  for (uint32_t hash : hashes)
  {
   if (count++ == 16) break;
   used += static_cast<size_t>(std::snprintf(list + used, sizeof(list) - used, "%08X ", hash));
  }
  diagnostics::Write("[HASHES] group=%d %s %s%s", group.getId(), stage, list, hashes.size() > 16 ? "(first 16 only)" : "");
 };
 logHashes("pixel", group.getPixelShaderHashes());
 logHashes("vertex", group.getVertexShaderHashes());
 logHashes("compute", group.getComputeShaderHashes());
}
static void logConfiguration()
{
 if (!diagnostics::recording.load(std::memory_order_relaxed)) return;
 diagnostics::Write("[CONFIG] groups=%zu controller_labels=%d global_modifier=%d suspend_keys=%zu restore_keys=%zu original_shader_matching=%u ui_scale_percent=%d",
  g_toggleGroups.size(), static_cast<int>(KeyData::getControllerLabelMode()),
  KeyData::globalHotkeyModifierToInt(KeyData::getGlobalHotkeyModifier()),
  g_globalSuspendHotkeys.size(), g_globalRestoreHotkeys.size(), g_originalShaderMatching.load(),g_uiScalePercent);
 const size_t count = (std::min)(g_toggleGroups.size(), size_t{24});
 for (size_t i = 0; i < count; ++i) logGroupConfiguration(g_toggleGroups[i]);
 if (count < g_toggleGroups.size()) diagnostics::Write("[CONFIG] Group details limited to first 24 groups.");
}
static const char* diagnosticApiName(device_api api)
{
 switch (api)
 {
 case device_api::d3d9: return "Direct3D 9";
 case device_api::d3d10: return "Direct3D 10";
 case device_api::d3d11: return "Direct3D 11";
 case device_api::d3d12: return "Direct3D 12";
 case device_api::opengl: return "OpenGL";
 case device_api::vulkan: return "Vulkan";
 default: return "Unknown API";
 }
}
static void onDiagnosticInitDevice(device* device)
{
 diagnostics::Write("[DEVICE] Initialized: %s", diagnosticApiName(device->get_api()));
}
static void onDiagnosticInitRuntime(effect_runtime* runtime)
{
 uint32_t width = 0, height = 0;
 runtime->get_screenshot_width_and_height(&width, &height);
 diagnostics::Write("[RUNTIME] Initialized: %s, %ux%u", diagnosticApiName(runtime->get_device()->get_api()), width, height);
}
static void logPresentDiagnostics(effect_runtime* runtime, std::chrono::steady_clock::time_point now)
{
 if (!diagnostics::recording.load(std::memory_order_relaxed)) return;
 static std::atomic<int64_t> nextCheck{0}, nextSummary{0};
 const int64_t ticks = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
 int64_t deadline = nextCheck.load(std::memory_order_relaxed);
 if (ticks < deadline || !nextCheck.compare_exchange_strong(deadline, ticks + 1000, std::memory_order_relaxed)) return;
 static std::atomic<uint32_t> lastPixel{0}, lastVertex{0}, lastCompute{0};
 const uint32_t pixel = g_pixelShaderManager.getActiveHuntedShaderHash();
 const uint32_t vertex = g_vertexShaderManager.getActiveHuntedShaderHash();
 const uint32_t compute = g_computeShaderManager.getActiveHuntedShaderHash();
 const bool pixelChanged = lastPixel.exchange(pixel) != pixel;
 const bool vertexChanged = lastVertex.exchange(vertex) != vertex;
 const bool computeChanged = lastCompute.exchange(compute) != compute;
 if (pixelChanged || vertexChanged || computeChanged)
  diagnostics::Write("[HUNT] Selected P=%08X V=%08X C=%08X (sampled once per second)", pixel, vertex, compute);
 deadline = nextSummary.load(std::memory_order_relaxed);
 if (ticks < deadline || !nextSummary.compare_exchange_strong(deadline, ticks + 60000, std::memory_order_relaxed)) return;
 uint32_t width = 0, height = 0;
 runtime->get_screenshot_width_and_height(&width, &height);
 diagnostics::Write("[STATUS] %s %ux%u shaders(P/V/C)=%u/%u/%u pipelines=%u/%u/%u groups=%zu suspended=%u editing=%d",
  diagnosticApiName(runtime->get_device()->get_api()), width, height,
  g_pixelShaderManager.getShaderCount(), g_vertexShaderManager.getShaderCount(), g_computeShaderManager.getShaderCount(),
  g_pixelShaderManager.getPipelineCount(), g_vertexShaderManager.getPipelineCount(), g_computeShaderManager.getPipelineCount(),
  g_toggleGroups.size(), g_allToggleGroupsSuspended, g_toggleGroupIdShaderEditing.load());
    smart::dx12::summary(runtime->get_device());
    smart::other::summary(runtime->get_device());
}
static bool is_key_down_numpad_only(reshade::api::effect_runtime* runtime, int vk_numpad)
{
 bool down = runtime->is_key_down(vk_numpad);
 down = down || ((GetAsyncKeyState(vk_numpad) & 0x8000) != 0);
 return down;
}
static int getAcceleratedRepeatMs(const std::chrono::steady_clock::time_point& holdStart, const std::chrono::steady_clock::time_point& now)
{
 const auto heldMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - holdStart).count();
 if (heldMs >= 2400)
  return s_holdRepeatVeryFastMs;
 if (heldMs >= 1400)
  return s_holdRepeatFastMs;
 if (heldMs >= 700)
  return s_holdRepeatMidMs;
 return s_holdRepeatStartMs;
}
static uint64_t fnv1a64(const std::string& text)
{
 uint64_t hash = 14695981039346656037ull;
 for (unsigned char c : text)
 {
  hash ^= static_cast<uint64_t>(c);
  hash *= 1099511628211ull;
 }
 return hash;
}
static std::string toHex64(uint64_t value)
{
 char buf[17] = {};
 snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(value));
 return std::string(buf);
}
static std::string toLowerCopy(std::string text)
{
 std::transform(text.begin(), text.end(), text.begin(),
  [](unsigned char c)
  {
   return static_cast<char>(std::tolower(c));
  });
 return text;
}
static int getHotkeyLayoutSortPriority(const ToggleGroup& group)
{
 const uint8_t keyCode = group.getToggleKey().getKeyCode();
 switch (keyCode)
 {
 case VK_END: return 0;
 case VK_DIVIDE: return 10;
 case VK_MULTIPLY: return 11;
 case VK_SUBTRACT: return 12;
 case VK_ADD: return 13;
 case VK_BACK: return 20;
 case VK_PRIOR: return 21;
 case VK_NEXT: return 22;
 case VK_UP: return 30;
 case VK_RIGHT: return 31;
 case VK_DOWN: return 32;
 case VK_LEFT: return 33;
 case VK_NUMPAD7: return 40;
 case VK_NUMPAD8: return 41;
 case VK_NUMPAD9: return 42;
 case VK_NUMPAD0: return 43;
 case VK_DECIMAL: return 44;
 case VK_INSERT: return 50;
 case VK_DELETE: return 51;
 default:
  return 1000 + static_cast<int>(keyCode);
 }
}
static void sortToggleGroupsByHotkey()
{
 std::stable_sort(g_toggleGroups.begin(), g_toggleGroups.end(),
  [](const ToggleGroup& a, const ToggleGroup& b)
  {
   const int priorityA = getHotkeyLayoutSortPriority(a);
   const int priorityB = getHotkeyLayoutSortPriority(b);
   if (priorityA != priorityB)
   {
    return priorityA < priorityB;
   }
   const int keyA = a.getToggleKey().toInt();
   const int keyB = b.getToggleKey().toInt();
   if (keyA != keyB)
   {
    return keyA < keyB;
   }
   return toLowerCopy(a.getName()) < toLowerCopy(b.getName());
  });
 saveShaderTogglerIniFile();
}
static void sortToggleGroupsByNameAZ()
{
 std::stable_sort(g_toggleGroups.begin(), g_toggleGroups.end(),
  [](const ToggleGroup& a, const ToggleGroup& b)
  {
   const std::string nameA = toLowerCopy(a.getName());
   const std::string nameB = toLowerCopy(b.getName());
   if (nameA != nameB)
   {
    return nameA < nameB;
   }
   return a.getId() < b.getId();
  });
 saveShaderTogglerIniFile();
}
static void sortToggleGroupsByNameLength()
{
 std::stable_sort(g_toggleGroups.begin(), g_toggleGroups.end(),
  [](const ToggleGroup& a, const ToggleGroup& b)
  {
   const size_t lengthA = a.getName().length();
   const size_t lengthB = b.getName().length();
   if (lengthA != lengthB)
   {
    return lengthA < lengthB;
   }
   return a.getName() < b.getName();
  });
 saveShaderTogglerIniFile();
}
static void appendSortedHashesToSignature(std::string& data, const char* prefix, const std::unordered_set<uint32_t>& hashes)
{
 std::vector<uint32_t> sorted(hashes.begin(), hashes.end());
 std::sort(sorted.begin(), sorted.end());
 for (const auto value : sorted)
 {
  data += "|";
  data += prefix;
  data += "=";
  data += std::to_string(value);
 }
}
static bool isTimedTriggerBindingActive(const ToggleGroup::TimedTriggerBinding& binding, reshade::api::effect_runtime* runtime)
{
 switch (binding.mode)
 {
 case ToggleGroup::TimedTriggerMode::OnPress:
  return binding.key.isKeyPressed(runtime);
 case ToggleGroup::TimedTriggerMode::WhileHeld:
  return binding.key.isKeyDown(runtime);
 case ToggleGroup::TimedTriggerMode::PressAndHold:
  return binding.key.isKeyPressed(runtime) || binding.key.isKeyDown(runtime);
 default:
  return binding.key.isKeyPressed(runtime);
 }
}
static bool isAnyTimedTriggerActive(const ToggleGroup& group, reshade::api::effect_runtime* runtime)
{
 if (group.hasTimedTriggerKeys())
 {
  for (size_t i = 0; i < group.getTimedTriggerKeyCount(); ++i)
  {
   if (isTimedTriggerBindingActive(group.getTimedTriggerBindingAt(i), runtime))
    return true;
  }
  return false;
 }
 return group.getToggleKey().isKeyPressed(runtime);
}
static bool isAnyTimedSuppressionKeyDown(const ToggleGroup& group, reshade::api::effect_runtime* runtime)
{
 if (!group.hasTimedSuppressionKeys())
  return false;
 for (size_t i = 0; i < group.getTimedSuppressionKeyCount(); ++i)
 {
  if (group.getTimedSuppressionKeyAt(i).isKeyDown(runtime))
   return true;
 }
 return false;
}
static void setGroupActiveWithEditRefresh(ToggleGroup& group, bool newActive)
{
 const bool previousActive = group.isActive();
 group.setActive(newActive);
 if (previousActive != newActive)
  diagnostics::Write("[TOGGLE] group=%d name=%.120s state=%s", group.getId(), group.getName().c_str(), newActive ? "blocked" : "visible");
 if (group.getId() == g_toggleGroupIdShaderEditing && previousActive != newActive)
 {
  g_vertexShaderManager.toggleHideMarkedShaders();
  g_pixelShaderManager.toggleHideMarkedShaders();
  g_computeShaderManager.toggleHideMarkedShaders();
 }
}
static std::vector<uint32_t> groupMenuInputs(const ToggleGroup& group,effect_runtime* runtime)
{
    std::vector<uint32_t> down;
    const auto collect=[&](const KeyData& key){if(key.isValid() && key.isKeyDown(runtime)) down.push_back(key.toInt());};
    collect(group.getToggleKey());
    if(group.isTimedMode())
    {
        for(size_t i=0;i<group.getTimedTriggerKeyCount();++i) collect(group.getTimedTriggerKeyAt(i));
        for(size_t i=0;i<group.getTimedSuppressionKeyCount();++i) collect(group.getTimedSuppressionKeyAt(i));
    }
    return down;
}
static bool keepGroupMenuState(const ToggleGroup& group,effect_runtime* runtime)
{
    const auto found=g_groupMenuInputs.find(group.getId());
    if(found==g_groupMenuInputs.end()) return false;
    if((group.isHoldMode() || group.isTimedMode()) && found->second==groupMenuInputs(group,runtime))
    {
        g_groupHotkeyWasDown[group.getId()]=group.getToggleKey().isKeyDown(runtime);
        return true;
    }
    g_groupMenuInputs.erase(found);
    return false;
}
static void toggleGroupFromMenu(ToggleGroup& group,effect_runtime* runtime)
{
    if(!runtime || g_allToggleGroupsSuspended || g_finderRuntime) return;
    const int id=group.getId();
    if(group.isHoldMode() || group.isTimedMode()) g_groupMenuInputs[id]=groupMenuInputs(group,runtime);
    else g_groupMenuInputs.erase(id);
    g_groupStartupActivationStartTime.erase(id);
    g_groupLastTimedTriggerTime.erase(id);
    g_groupTimedVisibleSince.erase(id);
    g_groupTimedFadeOutStart.erase(id);
    g_groupLastTimedSuppressionInputTime.erase(id);
    g_groupHotkeyWasDown[id]=group.getToggleKey().isKeyDown(runtime);
    g_groupHotkeyLastToggleTime[id]=std::chrono::steady_clock::now();
    setGroupActiveWithEditRefresh(group,!group.isActive());
    publishEffectFilters();
}
template <typename TMap>
static void shiftTimePointMap(TMap& values, const std::chrono::steady_clock::duration& amount)
{
 for (auto& entry : values)
  entry.second += amount;
}
static bool globalHotkeyExists(
 const std::vector<KeyData>& hotkeys,
 const KeyData& key,
 int ignoredIndex = -1)
{
 if (!key.isValid())
  return false;
 for (size_t i = 0; i < hotkeys.size(); ++i)
 {
  if (static_cast<int>(i) == ignoredIndex)
   continue;
  if (hotkeys[i].toInt() == key.toInt())
   return true;
 }
 return false;
}
static void restoreAllToggleGroups()
{
 if (!g_allToggleGroupsSuspended)
  return;
 const auto now = std::chrono::steady_clock::now();
 const auto pausedDuration = now - g_globalSuspensionStarted;
 shiftTimePointMap(g_groupHotkeyLastToggleTime, pausedDuration);
 shiftTimePointMap(g_groupLastTimedTriggerTime, pausedDuration);
 shiftTimePointMap(g_groupTimedVisibleSince, pausedDuration);
 shiftTimePointMap(g_groupTimedFadeOutStart, pausedDuration);
 shiftTimePointMap(g_groupLastTimedSuppressionInputTime, pausedDuration);
 shiftTimePointMap(g_groupStartupActivationStartTime, pausedDuration);
 for (const int groupId : g_pendingSuspendedGroupToggles)
 {
  for (auto& group : g_toggleGroups)
  {
   if (group.getId() == groupId)
   {
    setGroupActiveWithEditRefresh(group, !group.isActive());
    break;
   }
  }
 }
 g_pendingSuspendedGroupToggles.clear();
 g_allToggleGroupsSuspended = false;
 g_globalSuspensionStarted = {};
 diagnostics::Write("[GROUPS] Global suspension ended.");
}
static void suspendAllToggleGroups()
{
 if (g_allToggleGroupsSuspended)
  return;
 g_allToggleGroupsSuspended = true;
 g_globalSuspensionStarted = std::chrono::steady_clock::now();
 g_pendingSuspendedGroupToggles.clear();
 diagnostics::Write("[GROUPS] Globally suspended.");
}
static void toggleAllToggleGroupsSuspension()
{
 if (g_allToggleGroupsSuspended)
  restoreAllToggleGroups();
 else
  suspendAllToggleGroups();
}
static bool isAnyGlobalHotkeyPressed(
 const std::vector<KeyData>& hotkeys,
 reshade::api::effect_runtime* runtime)
{
 for (const auto& key : hotkeys)
 {
  if (key.isKeyPressed(runtime))
   return true;
 }
 return false;
}
static std::string buildIniSignature()
{
 std::string data;
 data += "Creator=";
 data += GT_CREATOR;
 data += "|FinderMode=" + std::to_string(g_finderMode);
    data += "|UIScalePercent=" + std::to_string(g_uiScalePercent);
    data += "|OriginalShaderMatching=" + std::to_string(g_originalShaderMatching.load());
 data += "|AmountGroups=";
 data += std::to_string(g_toggleGroups.size());
 data += "|ControllerMode=";
 data += std::to_string(static_cast<int>(KeyData::getControllerLabelMode()));
 data += "|GlobalHotkeyModifier=";
 data += std::to_string(KeyData::globalHotkeyModifierToInt(KeyData::getGlobalHotkeyModifier()));
 data += "|GlobalSuspendHotkeyCount=";
 data += std::to_string(g_globalSuspendHotkeys.size());
 for (size_t i = 0; i < g_globalSuspendHotkeys.size(); ++i)
 {
  data += "|GlobalSuspendHotkey" + std::to_string(i) + "=";
  data += std::to_string(g_globalSuspendHotkeys[i].toInt());
 }
 data += "|GlobalRestoreHotkeyCount=";
 data += std::to_string(g_globalRestoreHotkeys.size());
 for (size_t i = 0; i < g_globalRestoreHotkeys.size(); ++i)
 {
  data += "|GlobalRestoreHotkey" + std::to_string(i) + "=";
  data += std::to_string(g_globalRestoreHotkeys[i].toInt());
 }
 data += "|SigA=";
 data += GT_SIG_A;
 data += "|SigB=";
 data += GT_SIG_B;
 data += "|SigC=";
 data += GT_SIG_C;
 data += "|SigD=";
 data += GT_SIG_D;
 data += "|SigSeed=";
 data += toHex64(GT_SIG_SEED);
 for (const auto& group : g_toggleGroups)
 {
  data += "|Name=" + group.getName();
  data += "|Notice=" + group.getNotice();
  data += "|Key=" + std::to_string(group.getToggleKey().toInt());
  data += "|Startup=" + std::to_string(group.isActiveAtStartup() ? 1 : 0);
  data += "|StartupTimed=" + std::to_string(group.isStartupTimed() ? 1 : 0);
  data += "|StartupDurationMs=" + std::to_string(group.getStartupDurationMs());
  data += "|Hold=" + std::to_string(group.isHoldMode() ? 1 : 0);
  data += "|HoldInverted=" + std::to_string(group.isHoldInverted() ? 1 : 0);
  data += "|Timed=" + std::to_string(group.isTimedMode() ? 1 : 0);
  data += "|TimedInverted=" + std::to_string(group.isTimedModeInverted() ? 1 : 0);
  data += "|TimedDelay=" + std::to_string(group.getTimedModeDelayMs());
  data += "|TimedMinVisible=" + std::to_string(group.getTimedModeMinVisibleMs());
  data += "|TimedFadeOut=" + std::to_string(group.getTimedModeFadeOutMs());
  data += "|TimedTriggerCount=" + std::to_string(group.getTimedTriggerKeyCount());
  for (size_t i = 0; i < group.getTimedTriggerKeyCount(); ++i)
  {
   data += "|TimedTriggerKey" + std::to_string(i) + "=" + std::to_string(group.getTimedTriggerKeyAt(i).toInt());
   data += "|TimedTriggerMode" + std::to_string(i) + "=" + std::to_string(ToggleGroup::timedTriggerModeToInt(group.getTimedTriggerModeAt(i)));
  }
  data += "|TimedSuppressionCount=" + std::to_string(group.getTimedSuppressionKeyCount());
  data += "|TimedSuppressionLinger=" + std::to_string(group.getTimedSuppressionLingerMs());
  for (size_t i = 0; i < group.getTimedSuppressionKeyCount(); ++i)
  {
   data += "|TimedSuppressionKey" + std::to_string(i) + "=" + std::to_string(group.getTimedSuppressionKeyAt(i).toInt());
  }
  const bool hunting = group.getId() == g_toggleGroupIdShaderEditing;
  appendSortedHashesToSignature(data, "P", hunting ? g_pixelShaderManager.getMarkedShaderHashes() : group.getPixelShaderHashes());
  appendSortedHashesToSignature(data, "V", hunting ? g_vertexShaderManager.getMarkedShaderHashes() : group.getVertexShaderHashes());
  appendSortedHashesToSignature(data, "C", hunting ? g_computeShaderManager.getMarkedShaderHashes() : group.getComputeShaderHashes());
        for(const auto& rule:group.getEffectFilters()) data += "|EffectFilter="+finder::encode(rule);
 }
 for (const auto& [hash, choice] : g_smartSettings.saved())
  data += "|SmartD3D9=" + std::to_string(hash) + ":" + smart::encode(choice);
    for (const auto& [hash, choice] : g_smartSettings10.saved())
        data += "|SmartD3D10=" + std::to_string(hash) + ":" + smart::encode(choice);
    for (const auto& [hash, choice] : g_smartSettings11.saved())
        data += "|SmartD3D11=" + std::to_string(hash) + ":" + smart::encode(choice);
    for (const auto& [hash, choice] : g_smartSettings12.saved())
        data += "|SmartD3D12=" + std::to_string(hash) + ":" + smart::encode(choice);
    for (const auto& [hash, choice] : g_smartSettingsVK.saved())
        data += "|SmartVulkan=" + std::to_string(hash) + ":" + smart::encode(choice);
    for (const auto& [hash, choice] : g_smartSettingsGL.saved())
        data += "|SmartOpenGL=" + std::to_string(hash) + ":" + smart::encode(choice);
 return toHex64(fnv1a64(data));
}
static bool fileContainsTopAndBottomWatermark(const std::filesystem::path& filename)
{
 std::ifstream inFile(filename, std::ios::binary);
 if (!inFile.is_open())
  return false;
 std::string content((std::istreambuf_iterator<char>(inFile)),
                     std::istreambuf_iterator<char>());
 inFile.close();
 const bool hasHeader = content.rfind(GT_HEADER, 0) == 0;
 const bool hasFooter = content.find(GT_FOOTER) != std::string::npos;
 return hasHeader && hasFooter;
}
static bool rewriteIniWithTopAndBottomWatermark(const std::filesystem::path& filename)
{
 std::ifstream inFile(filename, std::ios::binary);
 if (!inFile.is_open())
  return false;
 std::string content((std::istreambuf_iterator<char>(inFile)),
                     std::istreambuf_iterator<char>());
 if (inFile.bad()) return false;
 inFile.close();
 if (content.rfind(GT_HEADER, 0) == 0)
 {
  content.erase(0, std::strlen(GT_HEADER));
 }
 size_t footerPos = content.find(GT_FOOTER);
 if (footerPos != std::string::npos)
 {
  content.erase(footerPos, std::strlen(GT_FOOTER));
 }
 std::ofstream outFile(filename, std::ios::binary | std::ios::trunc);
 if (!outFile.is_open())
  return false;
 outFile << GT_HEADER << content << GT_FOOTER;
 outFile.flush();
 const bool written = outFile.good();
 outFile.close();
 return written && !outFile.fail();
}
static uint32_t calculateShaderHash(void* shaderData)
{
 if (nullptr == shaderData)
 {
  return 0;
 }
 const auto shaderDesc = *static_cast<shader_desc *>(shaderData);
 return compute_crc32(static_cast<const uint8_t *>(shaderDesc.code), shaderDesc.code_size);
}
static int normalizeUiScalePercent(int value)
{
    return value>=100 && value<=250?value:0;
}
static float addonUiScale(float fontSize,const ImVec2& displaySize,int percent)
{
    percent=normalizeUiScalePercent(percent);
    if(percent) return percent/100.0f;
    if(!std::isfinite(fontSize) || fontSize<=0) return 1.0f;
    const float shortEdge=std::min(displaySize.x,displaySize.y);
    const float target=shortEdge>=2000?32.0f:shortEdge>=1400?22.0f:16.0f;
    return std::clamp(target/fontSize,1.0f,2.5f);
}
static const sta_ui::ModernFontApi& modernUiFontApi()
{
    static const auto api=sta_ui::ModernFontApi::resolve(
        reinterpret_cast<sta_ui::GetTable>(GetProcAddress(reshade::internal::get_reshade_module_handle(),"ReShadeGetImGuiFunctionTable")),
        ImGui::GetVersion());
    return api;
}
struct ScopedAddonUiScale
{
    float previousScale=1.0f;
    const float sourceFont=ImGui::GetFontSize();
    const float factor=addonUiScale(sourceFont,ImGui::GetIO().DisplaySize,g_uiScalePercent);
    const sta_ui::ModernFontApi& modern=modernUiFontApi();
    bool pushed=false,windowChanged=false;
    float appliedFont=sourceFont;
    ScopedAddonUiScale()
    {
        if(factor==1.0f || !std::isfinite(sourceFont) || sourceFont<=0) return;
        pushed=modern.push(factor);
        if(!pushed)
        {
            ImGui::SetWindowFontScale(1.0f);
            const float unscaled=ImGui::GetFontSize();
            previousScale=unscaled>0?sourceFont/unscaled:1.0f;
            ImGui::SetWindowFontScale(previousScale*factor);
            windowChanged=true;
        }
        appliedFont=ImGui::GetFontSize();
    }
    ~ScopedAddonUiScale()
    {
        if(pushed) modern.pop();
        else if(windowChanged) ImGui::SetWindowFontScale(previousScale);
    }
    float appliedPercent() const { return sourceFont>0?appliedFont/sourceFont*100.0f:100.0f; }
    bool applied() const { return std::abs(appliedFont-sourceFont*factor)<=1.0f; }
    const char* method() const { return pushed?"font-stack":windowChanged?"window-scale":"unchanged"; }
    ScopedAddonUiScale(const ScopedAddonUiScale&)=delete;
    ScopedAddonUiScale& operator=(const ScopedAddonUiScale&)=delete;
};
struct ScopedAddonUiStyle
{
    int colorCount=0;
    void color(ImGuiCol slot,const ImVec4& value) { ImGui::PushStyleColor(slot,value);++colorCount; }
    ScopedAddonUiStyle()
    {
        const float unit=ImGui::GetFontSize();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(unit*0.30f,std::max(1.0f,std::floor(unit/10))));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(unit*0.30f,1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing,ImVec2(unit*0.25f,1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.0f);
        color(ImGuiCol_Text,ImVec4(0.96f,0.98f,1.00f,1));
        color(ImGuiCol_TextDisabled,ImVec4(0.72f,0.78f,0.86f,1));
        color(ImGuiCol_Border,ImVec4(0.40f,0.51f,0.65f,1));
        color(ImGuiCol_Button,ImVec4(0.14f,0.22f,0.34f,1));
        color(ImGuiCol_ButtonHovered,ImVec4(0.22f,0.40f,0.62f,1));
        color(ImGuiCol_ButtonActive,ImVec4(0.18f,0.37f,0.60f,1));
        color(ImGuiCol_FrameBg,ImVec4(0.07f,0.11f,0.17f,1));
        color(ImGuiCol_FrameBgHovered,ImVec4(0.14f,0.23f,0.35f,1));
        color(ImGuiCol_FrameBgActive,ImVec4(0.17f,0.30f,0.46f,1));
        color(ImGuiCol_Header,ImVec4(0.16f,0.29f,0.44f,1));
        color(ImGuiCol_HeaderHovered,ImVec4(0.22f,0.40f,0.60f,1));
        color(ImGuiCol_HeaderActive,ImVec4(0.18f,0.37f,0.60f,1));
        color(ImGuiCol_Tab,ImVec4(0.10f,0.16f,0.25f,1));
        color(ImGuiCol_TabHovered,ImVec4(0.22f,0.40f,0.62f,1));
        color(ImGuiCol_TabActive,ImVec4(0.18f,0.37f,0.60f,1));
        color(ImGuiCol_CheckMark,ImVec4(0.35f,0.96f,0.65f,1));
        color(ImGuiCol_SliderGrab,ImVec4(0.39f,0.72f,1.00f,1));
        color(ImGuiCol_SliderGrabActive,ImVec4(0.65f,0.87f,1.00f,1));
        color(ImGuiCol_Separator,ImVec4(0.32f,0.41f,0.54f,1));
        color(ImGuiCol_PopupBg,ImVec4(0.06f,0.085f,0.13f,0.98f));
        color(ImGuiCol_ScrollbarGrab,ImVec4(0.36f,0.46f,0.60f,1));
        color(ImGuiCol_ScrollbarGrabHovered,ImVec4(0.48f,0.63f,0.81f,1));
        color(ImGuiCol_ScrollbarGrabActive,ImVec4(0.59f,0.77f,0.97f,1));
    }
    ~ScopedAddonUiStyle() { ImGui::PopStyleColor(colorCount);ImGui::PopStyleVar(7); }
};
struct ScopedAddonControlStyle
{
    explicit ScopedAddonControlStyle(bool primary=false)
    {
        const float unit=ImGui::GetFontSize();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(unit*0.55f,unit*0.28f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,unit*0.12f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,std::max(1.0f,std::floor(unit/16)));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(unit*0.35f,unit*0.15f));
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding,unit*0.12f);
        ImGui::PushStyleColor(ImGuiCol_Border,primary?ImVec4(0.40f,0.85f,0.70f,1):ImVec4(0.53f,0.72f,0.95f,1));
        ImGui::PushStyleColor(ImGuiCol_Button,primary?ImVec4(0.06f,0.35f,0.29f,1):ImVec4(0.13f,0.28f,0.47f,1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,primary?ImVec4(0.09f,0.43f,0.34f,1):ImVec4(0.20f,0.40f,0.64f,1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,primary?ImVec4(0.04f,0.28f,0.23f,1):ImVec4(0.09f,0.23f,0.41f,1));
        ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.055f,0.10f,0.18f,1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0.11f,0.22f,0.36f,1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImVec4(0.13f,0.28f,0.47f,1));
        ImGui::PushStyleColor(ImGuiCol_Tab,ImVec4(0.11f,0.22f,0.36f,1));
        ImGui::PushStyleColor(ImGuiCol_TabHovered,ImVec4(0.22f,0.43f,0.67f,1));
        ImGui::PushStyleColor(ImGuiCol_TabActive,ImVec4(0.17f,0.38f,0.63f,1));
    }
    ~ScopedAddonControlStyle() { ImGui::PopStyleColor(10);ImGui::PopStyleVar(5); }
    ScopedAddonControlStyle(const ScopedAddonControlStyle&)=delete;
    ScopedAddonControlStyle& operator=(const ScopedAddonControlStyle&)=delete;
};
static bool beginAddonTabBar()
{
    const ScopedAddonControlStyle controls;
    return ImGui::BeginTabBar("ShaderTogglerAdvancedTabs",ImGuiTabBarFlags_FittingPolicyScroll);
}
static bool beginAddonTabItem(const char* label)
{
    const ScopedAddonControlStyle controls;
    return ImGui::BeginTabItem(label);
}
struct ScopedAddonSectionStyle
{
    ScopedAddonSectionStyle()
    {
        const float unit=ImGui::GetFontSize();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(unit*0.35f,unit*0.18f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,unit*0.10f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,std::max(1.0f,std::floor(unit/16)));
        ImGui::PushStyleColor(ImGuiCol_Header,ImVec4(0.11f,0.25f,0.40f,1));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(0.19f,0.38f,0.58f,1));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,ImVec4(0.08f,0.20f,0.34f,1));
        ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0.45f,0.66f,0.88f,1));
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.98f,0.99f,1.00f,1));
    }
    ~ScopedAddonSectionStyle() { ImGui::PopStyleColor(5);ImGui::PopStyleVar(3); }
    ScopedAddonSectionStyle(const ScopedAddonSectionStyle&)=delete;
    ScopedAddonSectionStyle& operator=(const ScopedAddonSectionStyle&)=delete;
};
static bool addonTreeNode(const char* label)
{
    const ScopedAddonSectionStyle section;
    return ImGui::TreeNodeEx(label,ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth);
}
static bool addonSection(const char* label,ImGuiTreeNodeFlags flags=0)
{
    const ScopedAddonSectionStyle section;
    return ImGui::CollapsingHeader(label,flags);
}
static void addonHeading(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.58f,0.81f,1.00f,1));
    ImGui::TextWrapped("%s",text);
    ImGui::PopStyleColor();
}
static void addonHelpText(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextWrapped("%s",text);
    ImGui::PopStyleColor();
}
static void nextUiAction(const char* label)
{
    const auto& style=ImGui::GetStyle();
    const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;
    const float nextWidth=ImGui::CalcTextSize(label,nullptr,true).x+style.FramePadding.x*2;
    if(ImGui::GetItemRectMax().x+style.ItemSpacing.x+nextWidth<=right)
        ImGui::SameLine();
}
static std::string compactUiLabel(std::string text,float width)
{
    const float ellipsis=ImGui::CalcTextSize("...").x;
    if(ImGui::CalcTextSize(text.c_str()).x<=width) return text;
    while(!text.empty() && ImGui::CalcTextSize(text.c_str()).x+ellipsis>width)
    {
        size_t last=text.size()-1;
        while(last>0 && (static_cast<unsigned char>(text[last])&0xC0)==0x80) --last;
        text.erase(last);
    }
    return text+"...";
}
void addDefaultGroup()
{
 ToggleGroup toAdd("Default", ToggleGroup::getNewGroupId());
 toAdd.setToggleKey(VK_CAPITAL, false, false, false);
 g_toggleGroups.push_back(toAdd);
}
static void loadSmartSettings(CDataFile& ini)
{
    const char* sections[] = {"SmartDisableD3D9", "SmartDisableD3D10", "SmartDisableD3D11", "SmartDisableD3D12", "SmartDisableVulkan", "SmartDisableOpenGL"};
    smart::Settings* settings[] = {&g_smartSettings, &g_smartSettings10, &g_smartSettings11, &g_smartSettings12, &g_smartSettingsVK, &g_smartSettingsGL};
    for (size_t api = 0; api < std::size(settings); ++api)
    {
        const auto* section = sections[api];
        std::map<uint32_t, smart::Choice> choices;
        if (ini.GetInt("Version", section) == 1)
        {
            const int count = ini.GetInt("Count", section);
            if (count >= 0 && count <= 4096)
                for (int i = 0; i < count; ++i)
                {
                    uint32_t hash = 0;
                    const auto choice = smart::decode(ini.GetValue("Choice" + std::to_string(i), section));
                    if (smart::unsignedValue(ini.GetValue("Shader" + std::to_string(i), section), hash) && hash && choice)
                        choices[hash] = *choice;
                    else diagnostics::Write("[ERROR] SMART %s ignored invalid saved choice at index=%d", section, i);
                }
            else diagnostics::Write("[ERROR] SMART %s saved choice count is invalid; ignoring the section.", section);
        }
        settings[api]->replace(std::move(choices));
        diagnostics::Write("[SMART] Loaded %zu saved choices from %s", settings[api]->saved().size(), section);
    }
}
static void saveSmartSettings(CDataFile& ini)
{
    const char* sections[] = {"SmartDisableD3D9", "SmartDisableD3D10", "SmartDisableD3D11", "SmartDisableD3D12", "SmartDisableVulkan", "SmartDisableOpenGL"};
    smart::Settings* settings[] = {&g_smartSettings, &g_smartSettings10, &g_smartSettings11, &g_smartSettings12, &g_smartSettingsVK, &g_smartSettingsGL};
    for (size_t api = 0; api < std::size(settings); ++api)
    {
        const auto choices = settings[api]->saved();
        const auto* section = sections[api];
        ini.SetInt("Version", 1, "", section);
        ini.SetInt("Count", static_cast<int>(choices.size()), "", section);
        int i = 0;
        for (const auto& [hash, choice] : choices)
        {
            ini.SetUInt("Shader" + std::to_string(i), hash, "", section);
            ini.SetValue("Choice" + std::to_string(i++), smart::encode(choice), "", section);
        }
    }
}
void loadShaderTogglerIniFile()
{
 CDataFile iniFile;
 if (!iniFile.Load(g_iniFileName))
 {
  const DWORD attributes = GetFileAttributesW(g_iniFileName.c_str());
  const DWORD code = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : 0;
  if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND)
   diagnostics::Write("[INI] No existing ShaderToggler.ini; starting without saved groups.");
  else
   diagnostics::Write("[ERROR] INI load failed; file_exists=%u Windows_error=%lu", attributes != INVALID_FILE_ATTRIBUTES, static_cast<unsigned long>(code));
  return;
 }
 diagnostics::Write("[INI] Read existing configuration.");
 loadSmartSettings(iniFile);
 int numberOfGroups = iniFile.GetInt("GTAmountGroups", "General");
 bool usingCustomFormat = true;
 if (numberOfGroups == INT_MIN)
 {
  numberOfGroups = iniFile.GetInt("AmountGroups", "General");
  usingCustomFormat = false;
 }
    g_uiScalePercent=normalizeUiScalePercent(iniFile.GetInt("UIScalePercent","General"));
    g_finderMode = iniFile.GetInt("FinderMode", "General") == 1 ? 1 : 0;
    g_originalShaderMatching = iniFile.GetValue("OriginalShaderMatching", "General") != "0";
 const int savedControllerMode = iniFile.GetInt("ControllerLabelMode", "General");
 if (savedControllerMode >= static_cast<int>(KeyData::ControllerLabelMode::Auto) &&
  savedControllerMode <= static_cast<int>(KeyData::ControllerLabelMode::PlayStation))
 {
  KeyData::setControllerLabelMode(static_cast<KeyData::ControllerLabelMode>(savedControllerMode));
 }
 else
 {
  KeyData::setControllerLabelMode(KeyData::ControllerLabelMode::Auto);
 }
 const int savedGlobalModifier = iniFile.GetInt("GlobalHotkeyModifier", "General");
 if (savedGlobalModifier != INT_MIN)
 {
  KeyData::setGlobalHotkeyModifier(KeyData::globalHotkeyModifierFromInt(savedGlobalModifier));
 }
 else
 {
  KeyData::setGlobalHotkeyModifier(KeyData::GlobalHotkeyModifier::None);
 }
 g_globalSuspendHotkeys.clear();
 g_globalRestoreHotkeys.clear();
 const std::vector<uint32_t> savedGlobalSuspendHotkeys =
  iniFile.GetArray("GlobalSuspendHotkeys", "General");
 for (const uint32_t keyValue : savedGlobalSuspendHotkeys)
 {
  KeyData key = KeyData::fromInt(keyValue);
  if (key.isValid() && !globalHotkeyExists(g_globalSuspendHotkeys, key))
   g_globalSuspendHotkeys.push_back(key);
 }
 const std::vector<uint32_t> savedGlobalRestoreHotkeys =
  iniFile.GetArray("GlobalRestoreHotkeys", "General");
 for (const uint32_t keyValue : savedGlobalRestoreHotkeys)
 {
  KeyData key = KeyData::fromInt(keyValue);
  if (key.isValid() && !globalHotkeyExists(g_globalRestoreHotkeys, key))
   g_globalRestoreHotkeys.push_back(key);
 }
 if (g_globalSuspendHotkeys.empty() && g_globalRestoreHotkeys.empty())
 {
  const std::vector<uint32_t> legacyGlobalSuspensionHotkeys =
   iniFile.GetArray("GlobalSuspensionHotkeys", "General");
  for (const uint32_t keyValue : legacyGlobalSuspensionHotkeys)
  {
   KeyData key = KeyData::fromInt(keyValue);
   if (!key.isValid())
    continue;
   if (!globalHotkeyExists(g_globalSuspendHotkeys, key))
    g_globalSuspendHotkeys.push_back(key);
   if (!globalHotkeyExists(g_globalRestoreHotkeys, key))
    g_globalRestoreHotkeys.push_back(key);
  }
 }
 g_allToggleGroupsSuspended = false;
 g_pendingSuspendedGroupToggles.clear();
 g_globalSuspensionStarted = {};
 if (numberOfGroups == INT_MIN)
 {
  addDefaultGroup();
  g_toggleGroups[0].loadState(iniFile, -1, false);
  saveShaderTogglerIniFile();
  g_groupStartupActivationStartTime.clear();
  if (g_toggleGroups[0].isActiveAtStartup() && g_toggleGroups[0].isStartupTimed())
   g_groupStartupActivationStartTime[g_toggleGroups[0].getId()] = std::chrono::steady_clock::now();
  return;
 }
 g_toggleGroups.clear();
 for (int i = 0; i < numberOfGroups; i++)
 {
  g_toggleGroups.push_back(ToggleGroup("", ToggleGroup::getNewGroupId()));
  g_toggleGroups.back().loadState(iniFile, i, usingCustomFormat);
 }
 if (usingCustomFormat)
 {
  const std::string creator = iniFile.GetValue("Creator", "General");
  const std::string savedStamp = iniFile.GetValue(GT_CACHE_KEY, "General");
  const std::string currentStamp = buildIniSignature();
  bool needsRepair = false;
  if (creator != GT_CREATOR)
   needsRepair = true;
  if (savedStamp != currentStamp)
   needsRepair = true;
  if (!fileContainsTopAndBottomWatermark(g_iniFileName))
   needsRepair = true;
  if (needsRepair)
  {
   saveShaderTogglerIniFile();
  }
 }
 g_groupStartupActivationStartTime.clear();
 const auto startupNow = std::chrono::steady_clock::now();
 for (const auto& group : g_toggleGroups)
 {
  if (group.isActiveAtStartup() && group.isStartupTimed())
   g_groupStartupActivationStartTime[group.getId()] = startupNow;
 }
}
bool saveShaderTogglerIniFile()
{
 static std::mutex saveMutex;
 std::lock_guard saveLock(saveMutex);
 publishEffectFilters();
 CDataFile iniFile;
    iniFile.SetInt("UIScalePercent",g_uiScalePercent,"","General");
    iniFile.SetInt("FinderMode", g_finderMode, "", "General");
    iniFile.SetInt("OriginalShaderMatching", g_originalShaderMatching.load() ? 1 : 0, "", "General");
 iniFile.SetInt("GTAmountGroups", static_cast<int>(g_toggleGroups.size()), "", "General");
 iniFile.SetValue("Creator", GT_CREATOR, "", "General");
 iniFile.SetValue(GT_CACHE_KEY, buildIniSignature(), "", "General");
 iniFile.SetInt("ControllerLabelMode", static_cast<int>(KeyData::getControllerLabelMode()), "", "General");
 iniFile.SetInt("GlobalHotkeyModifier", KeyData::globalHotkeyModifierToInt(KeyData::getGlobalHotkeyModifier()), "", "General");
 std::vector<uint32_t> globalSuspendHotkeyValues;
 globalSuspendHotkeyValues.reserve(g_globalSuspendHotkeys.size());
 for (const auto& key : g_globalSuspendHotkeys)
  globalSuspendHotkeyValues.push_back(static_cast<uint32_t>(key.toInt()));
 iniFile.SetArray("GlobalSuspendHotkeys", globalSuspendHotkeyValues, "", "General");
 std::vector<uint32_t> globalRestoreHotkeyValues;
 globalRestoreHotkeyValues.reserve(g_globalRestoreHotkeys.size());
 for (const auto& key : g_globalRestoreHotkeys)
  globalRestoreHotkeyValues.push_back(static_cast<uint32_t>(key.toInt()));
 iniFile.SetArray("GlobalRestoreHotkeys", globalRestoreHotkeyValues, "", "General");
 for (int i = 0; i < static_cast<int>(g_toggleGroups.size()); i++)
 {
  ToggleGroup snapshot = g_toggleGroups[i];
  if (snapshot.getId() == g_toggleGroupIdShaderEditing)
   snapshot.storeCollectedHashes(g_pixelShaderManager.getMarkedShaderHashes(),
    g_vertexShaderManager.getMarkedShaderHashes(), g_computeShaderManager.getMarkedShaderHashes());
  snapshot.saveState(iniFile, i, true);
 }
 saveSmartSettings(iniFile);
 ini_save::WindowsFiles files;
 const auto result=ini_save::save(g_iniFileName,files,[&](const std::filesystem::path& temporary)
 {
  iniFile.SetFileName(temporary);
  const bool written=iniFile.Save();
  iniFile.Clear();
  return written && rewriteIniWithTopAndBottomWatermark(temporary);
 });
 iniFile.Clear();
 if (!result.saved)
 {
  g_uiSaveResult=-1;
  diagnostics::Write("[ERROR] INI save failed at %s; Windows_error=%lu. Existing configuration retained. Check permissions and free disk space.",result.step,static_cast<unsigned long>(result.error));
  return false;
 }
 g_uiSaveResult=1;
 diagnostics::Write("[INI] Saved configuration successfully.");
 logConfiguration();
 return true;
}
static void onInitCommandList(command_list *commandList)
{
 commandList->create_private_data<CommandListDataContainer>();
}
static void onDestroyCommandList(command_list *commandList)
{
 commandList->destroy_private_data<CommandListDataContainer>();
}
static void onResetCommandList(command_list *commandList)
{
 CommandListDataContainer &commandListData = commandList->get_private_data<CommandListDataContainer>();
 commandListData = {};
}
static void onInitPipeline(device *device, pipeline_layout, uint32_t subobjectCount, const pipeline_subobject *subobjects, pipeline pipelineHandle)
{
    if (smart::other::internal()) return;
    smart::other::capture(device, pipelineHandle.handle, subobjectCount, subobjects);
 for (uint32_t i = 0; i < subobjectCount; ++i)
 {
  switch (subobjects[i].type)
  {
  case pipeline_subobject_type::vertex_shader:
   g_vertexShaderManager.addHashHandlePair(calculateShaderHash(subobjects[i].data), pipelineHandle.handle);
   break;
  case pipeline_subobject_type::pixel_shader:
            if (subobjects[i].count && subobjects[i].data)
                smart::modern::capture(device, pipelineHandle.handle, *static_cast<const shader_desc*>(subobjects[i].data));
   g_pixelShaderManager.addHashHandlePair(calculateShaderHash(subobjects[i].data), pipelineHandle.handle);
   break;
  case pipeline_subobject_type::compute_shader:
   g_computeShaderManager.addHashHandlePair(calculateShaderHash(subobjects[i].data), pipelineHandle.handle);
   break;
  default:
   break;
  }
 }
}
static void onDestroyPipeline(device *device, pipeline pipelineHandle)
{
    if (smart::other::internal()) return;
    smart::other::forget(device, pipelineHandle.handle);
    smart::modern::forget(device, pipelineHandle.handle);
    smart::dx12::forget(device, pipelineHandle.handle);
 g_pixelShaderManager.removeHandle(pipelineHandle.handle);
 g_vertexShaderManager.removeHandle(pipelineHandle.handle);
 g_computeShaderManager.removeHandle(pipelineHandle.handle);
}
static void displayIsPartOfToggleGroup()
{
 ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
 ImGui::SameLine();
 ImGui::Text(" Shader is part of this toggle group.");
 ImGui::PopStyleColor();
}
static void displayShaderManagerInfo(ShaderManager& toDisplay, const char* shaderType)
{
 if (toDisplay.isInHuntingMode())
 {
  ImGui::Text("# of %s shaders active: %d. # of %s shaders in group: %d",
   shaderType, toDisplay.getAmountShaderHashesCollected(), shaderType, toDisplay.getMarkedShaderCount());
  ImGui::Text("Current selected %s shader: %d / %d.",
   shaderType, toDisplay.getActiveHuntedShaderIndex(), toDisplay.getAmountShaderHashesCollected());
  if (toDisplay.isHuntedShaderMarked())
  {
   displayIsPartOfToggleGroup();
  }
 }
}
static void displayShaderManagerStats(ShaderManager& toDisplay, const char* shaderType)
{
 ImGui::Text("# of pipelines with %s shaders: %d. # of different %s shaders gathered: %d.",
  shaderType, toDisplay.getPipelineCount(), shaderType, toDisplay.getShaderCount());
}
static bool keepFinderFilter(bool finish = true)
{
    const auto rule = g_effectFinder.confirmed();
    if (!rule) return false;
    for (auto& group : g_toggleGroups)
    {
        if (group.getId() != g_finderGroup) continue;
        const auto& saved = group.getEffectFilters();
        if (std::find(saved.begin(), saved.end(), *rule) == saved.end())
        {
            const size_t previousCount=saved.size();
            if (!group.addEffectFilter(*rule)) return false;
            if (!saveShaderTogglerIniFile())
            {
                group.removeEffectFilter(previousCount);
                publishEffectFilters();
                diagnostics::Write("[ERROR] FINDER filter save failed group=%d; previous filters restored, preview kept for retry.",group.getId());
                return false;
            }
            diagnostics::Write("[FINDER] Confirmed individual filter group=%d rule=%s", group.getId(), finder::encode(*rule).c_str());
        }
        else if(g_uiSaveResult<0 && !saveShaderTogglerIniFile()) return false;
        if (finish) closeEffectFinder();
        else
        {
            const auto view = g_effectFinder.view(GetTickCount64());
            g_effectFinder.select((view.selected + 1) % view.candidates.size(), true);
        }
        publishEffectFilters();
        return true;
    }
    return false;
}
static void answerFinder(bool gone)
{
    const auto view = g_effectFinder.view(GetTickCount64());
    if (gone && view.testing == 1)
    {
        keepFinderFilter();
        return;
    }
    if (g_effectFinder.answer(gone))
        diagnostics::Write("[FINDER] Batch answer group=%d gone=%u tested=%zu remaining_before=%zu", view.group, gone, view.testing, view.remainingCandidates);
}
static void finderKeys(effect_runtime* runtime)
{
    const int keys[] = {VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD5};
    bool pressed[5]{};
    for (size_t i = 0; i < 5; ++i)
    {
        const bool down = runtime->is_key_down(keys[i]);
        pressed[i] = down && !g_finderKeysDown[i];
        g_finderKeysDown[i] = down;
    }
    if (g_finderKeyboardCaptured) return;
    if (pressed[4]) { closeEffectFinder(); publishEffectFilters(); return; }
    const auto view = g_effectFinder.view(GetTickCount64());
    if (view.stage != finder::Stage::Results) return;
    if (pressed[0]) g_effectFinder.compare();
    else if (pressed[1])
    {
        if (view.quick) g_effectFinder.back();
        else if (!view.candidates.empty()) g_effectFinder.select((view.selected + view.candidates.size() - 1) % view.candidates.size(), true);
    }
    else if (pressed[2])
    {
        if (view.quick) answerFinder(false);
        else if (!view.candidates.empty()) g_effectFinder.select((view.selected + 1) % view.candidates.size(), true);
    }
    else if (pressed[3]) answerFinder(true);
}
static void finderStatus(const finder::View& view, bool showShortcuts=true)
{
    using finder::Stage;
    switch (view.stage)
    {
    case Stage::FrozenDelay:
        ImGui::Text("Keep the game paused - scan starts in %.1f s", view.remaining / 1000.0f);
        ImGui::TextWrapped("Close ReShade. Keep both the unwanted and wanted effects visible.");
        break;
    case Stage::FrozenCapture:
        ImGui::Text("SCANNING PAUSED SCENE: %.1f s", view.remaining / 1000.0f);
        ImGui::TextUnformatted("Keep the scene frozen. No attacks needed.");
        break;
    case Stage::BaselineDelay:
        ImGui::Text("Get ready - stay idle: %.1f s", view.remaining / 1000.0f);
        ImGui::TextWrapped("Close ReShade. Keep the effects you want to preserve visible.");
        break;
    case Stage::Baseline:
        ImGui::Text("STAY IDLE - recording: %.1f s", view.remaining / 1000.0f);
        ImGui::TextWrapped("Do not produce the effect you want removed yet.");
        break;
    case Stage::ActionDelay:
        ImGui::Text("Get ready to repeat the effect: %.1f s", view.remaining / 1000.0f);
        break;
    case Stage::Action:
        ImGui::Text("REPEAT THE EFFECT NOW - recording: %.1f s", view.remaining / 1000.0f);
        break;
    case Stage::BaselineDone:
        ImGui::TextWrapped(view.baselineCalls ? "Baseline ready. Start the action capture from the finder controls." :
            "No usable draws captured. Return to gameplay and restart the finder.");
        break;
    case Stage::Results:
        if(showShortcuts || view.includeCompute)
            addonHelpText(view.includeCompute ? "Graphics + compute candidates (advanced)" : "Graphics candidates only");
        if (view.candidates.empty() && view.focused && view.followOriginal)
            ImGui::TextWrapped("No eligible graphics candidates matched this toggle. The paused scene may reuse earlier work, or its graphics calls may not be supported. You can scan during gameplay as well.");
        else if (view.candidates.empty() && view.excludedCompute)
            ImGui::TextWrapped("Only compute candidates matched. Compute testing is off by default because skipping these operations can destabilize the game. Select a graphics shader group or use Manual controls and details.");
        else if (view.candidates.empty()) ImGui::TextWrapped(view.frozen ?
            "No matching recurring draws found. Check the shader selection. If the pause menu only reuses an image, capture during gameplay instead." :
            "No candidates found. Try the captures again in the same scene.");
        else if (view.quick && !view.testing) ImGui::TextWrapped("No match in this search. Undo the last answer or reopen the finder controls to retry.");
        else
        {
            if (view.testing > 1)
            {
                ImGui::Text("Testing %u candidates together (%u left)", static_cast<unsigned>(view.testing), static_cast<unsigned>(view.remainingCandidates));
                ImGui::TextWrapped(view.frozen ? "Stay paused. Other effects may disappear during this temporary test." :
                    "Repeat the effect. Other effects may disappear during this temporary test.");
                if(showShortcuts) ImGui::TextUnformatted("Num 2: Still visible    Num 3: Effect gone");
            }
            else
            {
                ImGui::Text("Testing one candidate (%u of %u)", static_cast<unsigned>(view.selected + 1), static_cast<unsigned>(view.candidates.size()));
                ImGui::TextWrapped(view.frozen ? "Stay paused. Check that only the unwanted effect disappears." :
                    "Check that only the unwanted effect disappears.");
                if(showShortcuts) ImGui::TextUnformatted("Num 2: Try another    Num 3: Keep and finish");
                if (view.selected < view.candidates.size() && view.candidates[view.selected].baseline)
                    ImGui::TextWrapped("Weaker match: also appeared in the idle capture.");
                if (view.selected < view.candidates.size() && finder::compute(view.candidates[view.selected].signature))
                    ImGui::TextWrapped("Compute test: this operation may produce data needed by later rendering.");
            }
            ImGui::TextColored(view.preview ? ImVec4(0.35f,0.96f,0.65f,1) : ImVec4(0.58f,0.81f,1.00f,1),
                "%s - matched %llu calls", view.preview ? "Preview ON" : "Original view", static_cast<unsigned long long>(view.previewHits));
            if (!view.previewHits && view.preview) ImGui::TextWrapped(view.frozen ?
                "Waiting for matching draws. This call may not run while paused." : "Repeat the action to exercise this test.");
        }
        if(showShortcuts) ImGui::TextUnformatted(view.quick ? "Num 0: Original / preview    Num 1: Undo" : "Num 0: Original / preview    Num 1: Previous");
        break;
    default: ImGui::TextUnformatted(view.frozen ? "Click Scan paused scene when ready." : "Start guided capture to find an effect."); break;
    }
    if(showShortcuts) ImGui::TextUnformatted("Num 5: Stop finder and restore normal groups");
}
static void finderHud(effect_runtime* runtime)
{
    if (runtime != g_finderRuntime) return;
    const auto view = g_effectFinder.view(GetTickCount64());
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 10.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    if (ImGui::Begin("ShaderTogglerEffectFinder", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoInputs))
    {
        const ScopedAddonUiScale uiScale;
        const ScopedAddonUiStyle uiStyle;
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 38.0f);
        addonHeading("Shader Toggler Advanced - Find effect");
        finderStatus(view);
        ImGui::PopTextWrapPos();
    }
    ImGui::End();
}
static void onReshadeOverlay(reshade::api::effect_runtime *runtime)
{
    finderHud(runtime);
    if (runtime == g_finderRuntime) g_finderKeyboardCaptured = ImGui::GetIO().WantCaptureKeyboard;
 if (ImGui::GetIO().WantCaptureMouse)
  g_overlayMouseCaptureLastSeen = std::chrono::steady_clock::now();
 if (g_toggleGroupIdShaderEditing >= 0 && g_overlayOpacity > 0.0f)
 {
  std::string editingGroupName = "";
  for (auto& group : g_toggleGroups)
  {
   if (group.getId() == g_toggleGroupIdShaderEditing)
   {
    editingGroupName = group.getName();
    break;
   }
  }
  ImGui::SetNextWindowBgAlpha(g_overlayOpacity);
  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 10.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
  if (!ImGui::Begin("ShaderTogglerInfo", nullptr,
   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
   ImGuiWindowFlags_NoNav))
  {
   ImGui::End();
   return;
  }
        {
        const ScopedAddonUiScale uiScale;
        const ScopedAddonUiStyle uiStyle;
  displayShaderManagerStats(g_vertexShaderManager, "vertex");
  displayShaderManagerStats(g_pixelShaderManager, "pixel");
  displayShaderManagerStats(g_computeShaderManager, "compute");
  if (g_activeCollectorFrameCounter > 0)
  {
   const uint32_t counterValue = g_activeCollectorFrameCounter;
   ImGui::Text("Collecting active shaders... frames to go: %d", counterValue);
  }
  else
  {
   if (g_vertexShaderManager.isInHuntingMode() || g_pixelShaderManager.isInHuntingMode() || g_computeShaderManager.isInHuntingMode())
   {
    ImGui::Text("Editing the shaders for group: %s", editingGroupName.c_str());
   }
   displayShaderManagerInfo(g_vertexShaderManager, "vertex");
   displayShaderManagerInfo(g_pixelShaderManager, "pixel");
   displayShaderManagerInfo(g_computeShaderManager, "compute");
  }
        }
  ImGui::End();
 }
}
static void onBindPipeline(command_list* commandList, pipeline_stage stages, pipeline pipelineHandle)
{
    if (smart::other::internal()) return;
 if (nullptr == commandList)
 {
  return;
 }
 CommandListDataContainer& commandListData = commandList->get_private_data<CommandListDataContainer>();
    const bool collecting = g_activeCollectorFrameCounter > 0;
    const bool originalMatching = g_originalShaderMatching.load();
    const auto trackOriginal = [&](ShaderManager& manager, uint64_t& handle)
    {
        if (!pipelineHandle.handle || !manager.isKnownHandle(pipelineHandle.handle)) return;
        handle = pipelineHandle.handle;
        if (collecting && originalMatching) manager.addActivePipelineHandle(handle);
    };
    trackOriginal(g_pixelShaderManager, commandListData.originalPixelShaderPipeline);
    trackOriginal(g_vertexShaderManager, commandListData.originalVertexShaderPipeline);
    trackOriginal(g_computeShaderManager, commandListData.originalComputeShaderPipeline);
    smart::other::bound(commandList, stages, pipelineHandle.handle);
    if (smart::dx12::supported(commandList->get_device()))
    {
        smart::dx12::bound(commandList, pipelineHandle.handle);
        commandListData.activePixelShaderPipeline = 0;
        commandListData.activeVertexShaderPipeline = 0;
        commandListData.activeComputeShaderPipeline = 0;
    }
 const auto updateStage = [&](pipeline_stage stage, ShaderManager& manager, uint64_t& activeHandle)
 {
  if ((stages & stage) != stage)
   return;
  activeHandle = manager.isKnownHandle(pipelineHandle.handle) ? pipelineHandle.handle : 0;
  if (collecting && !originalMatching && activeHandle != 0)
  {
   manager.addActivePipelineHandle(activeHandle);
  }
 };
 updateStage(pipeline_stage::pixel_shader, g_pixelShaderManager, commandListData.activePixelShaderPipeline);
 updateStage(pipeline_stage::vertex_shader, g_vertexShaderManager, commandListData.activeVertexShaderPipeline);
 updateStage(pipeline_stage::compute_shader, g_computeShaderManager, commandListData.activeComputeShaderPipeline);
}
static uint32_t finderApi(device_api api)
{
    switch(api) {
    case device_api::d3d9:return 1;case device_api::d3d10:return 2;case device_api::d3d11:return 3;
    case device_api::d3d12:return 4;case device_api::vulkan:return 5;case device_api::opengl:return 6;
    default:return 0;
    }
}
static bool observeEffect(command_list* commands,finder::Kind kind,uint32_t a,uint32_t b,uint32_t c,bool canSkip=true)
{
    if(!commands || !g_effectFinder.needsEvents() || smart::other::internal()) return false;
    auto* dev=commands->get_device();
    if(smart::other::isGL(dev)) canSkip &= smart::other::safeGLDraw(commands) || kind==finder::Kind::Dispatch || kind==finder::Kind::IndirectDispatch;
    smart::dx12::restore(commands);smart::other::restore(commands);
    const auto& data=commands->get_private_data<CommandListDataContainer>();
    finder::Signature signature{finderApi(dev->get_api()),static_cast<uint32_t>(kind),0,0,0,a,b,c,0,0};
    if(finder::compute(signature)) signature[4]=g_computeShaderManager.getShaderHash(data.activeComputeShaderPipeline);
    else
    {
        const auto pixel=smart::pixelHandle(commands,data.activePixelShaderPipeline);
        signature[2]=g_vertexShaderManager.getShaderHash(data.activeVertexShaderPipeline);
        signature[3]=g_pixelShaderManager.getShaderHash(pixel);
        const uint64_t key=smart::dx12::drawKey(dev,pixel);
        signature[8]=static_cast<uint32_t>(key);signature[9]=static_cast<uint32_t>(key>>32);
    }
    if(signature[1]>=4 && !signature[6]) signature[6]=kind==finder::Kind::IndirectIndexed?20:kind==finder::Kind::IndirectDispatch?12:16;
    const finder::History history{
        g_pixelShaderManager.getShaderHash(data.originalPixelShaderPipeline),
        g_vertexShaderManager.getShaderHash(data.originalVertexShaderPipeline),
        g_computeShaderManager.getShaderHash(data.originalComputeShaderPipeline)};
    const bool skip=g_effectFinder.observe(reinterpret_cast<uintptr_t>(dev),signature,GetTickCount64(),canSkip,history);
    if(skip) smart::restore(dev);
    return skip;
}
static bool observeD3D12Indirect(command_list* commands,uint32_t kind,uint32_t count,uint32_t stride,bool canSkip)
{
    const bool bindings=(kind&0x100)!=0;
    if(kind&~0x1FFu) return false;
    kind&=0xFF;
    if(kind<1 || kind>3 || (bindings && kind==3)) return false;
    return observeEffect(commands,static_cast<finder::Kind>(kind+3),count,stride,bindings?2:1,canSkip);
}
static bool originalShaderBlocked(uint32_t hash, ShaderManager& manager, pipeline_stage stage)
{
    if (!hash) return false;
    if (manager.isBlockedShader(hash)) return true;
    if (g_allToggleGroupsSuspended) return false;
    for (const auto& group : g_toggleGroups)
    {
        if (!group.isActive() || group.getId() == g_toggleGroupIdShaderEditing) continue;
        const auto& hashes = stage == pipeline_stage::pixel_shader ? group.getPixelShaderHashes() :
            stage == pipeline_stage::vertex_shader ? group.getVertexShaderHashes() : group.getComputeShaderHashes();
        if (hashes.count(hash)) return true;
    }
    return false;
}
static bool originalPassBlocked(command_list* commands)
{
    if (!commands || !g_originalShaderMatching.load() || g_effectFinder.isolates(reinterpret_cast<uintptr_t>(commands->get_device()))) return false;
    const auto& data = commands->get_private_data<CommandListDataContainer>();
    return originalShaderBlocked(g_pixelShaderManager.getShaderHash(data.originalPixelShaderPipeline), g_pixelShaderManager, pipeline_stage::pixel_shader) ||
        originalShaderBlocked(g_vertexShaderManager.getShaderHash(data.originalVertexShaderPipeline), g_vertexShaderManager, pipeline_stage::vertex_shader) ||
        originalShaderBlocked(g_computeShaderManager.getShaderHash(data.originalComputeShaderPipeline), g_computeShaderManager, pipeline_stage::compute_shader);
}
bool blockDrawCallForCommandList(command_list* commandList, SmartDrawRequest* replacement = nullptr)
{
    if (!commandList) return false;
    smart::dx12::restore(commandList);
    smart::other::restore(commandList);
    const bool finding=g_effectFinder.isolates(reinterpret_cast<uintptr_t>(commandList->get_device()));
    const auto kept=finding?g_effectFinder.context():nullptr;
    auto& data = commandList->get_private_data<CommandListDataContainer>();
    const uint64_t pixelHandle = smart::pixelHandle(commandList, data.activePixelShaderPipeline);
    const uint32_t pixelHash = g_pixelShaderManager.getShaderHash(pixelHandle);
    const uint32_t vertexHash = g_vertexShaderManager.getShaderHash(data.activeVertexShaderPipeline);
    const bool originalMatching = !finding && g_originalShaderMatching.load();
    bool pixelBlocked = pixelHash && (finding?kept->pixel.count(pixelHash)!=0:g_pixelShaderManager.isBlockedShader(pixelHash));
    bool vertexBlocked = vertexHash && (finding?kept->vertex.count(vertexHash)!=0:g_vertexShaderManager.isBlockedShader(vertexHash));
    if (originalMatching)
    {
        const auto originalPixel = g_pixelShaderManager.getShaderHash(data.originalPixelShaderPipeline);
        pixelBlocked = originalShaderBlocked(originalPixel, g_pixelShaderManager, pipeline_stage::pixel_shader);
        vertexBlocked = originalShaderBlocked(g_vertexShaderManager.getShaderHash(data.originalVertexShaderPipeline), g_vertexShaderManager, pipeline_stage::vertex_shader);
        const bool computeBlocked = originalShaderBlocked(g_computeShaderManager.getShaderHash(data.originalComputeShaderPipeline), g_computeShaderManager, pipeline_stage::compute_shader);
        if (computeBlocked || (pixelBlocked && originalPixel != pixelHash))
        { smart::restore(commandList->get_device());return true; }
    }
    for (const auto& group : g_toggleGroups)
    {
        if (originalMatching || finding || g_allToggleGroupsSuspended || !group.isActive() || group.getId() == g_toggleGroupIdShaderEditing) continue;
        pixelBlocked |= pixelHash && group.getPixelShaderHashes().count(pixelHash) != 0;
        vertexBlocked |= vertexHash && group.getVertexShaderHashes().count(vertexHash) != 0;
    }
    auto* device = commandList->get_device();
    const uint32_t huntedHash = finding?0:g_pixelShaderManager.getActiveHuntedShaderHash();
    const bool modern = smart::modern::supported(device);
    const bool dx12 = smart::dx12::supported(device);
    const bool other = smart::other::supported(device);
    auto& settings = smartSettingsFor(device);
    if (pixelHash && pixelHash == huntedHash)
    {
        if (other) smart::other::observe(commandList, pixelHandle, pixelHash);
        else if (dx12) smart::dx12::observe(commandList, pixelHandle, pixelHash);
        else if (modern) smart::modern::observe(commandList, pixelHash);
        else smart::observe(device, pixelHash);
    }
    const auto preview = settings.preview();
    if ((other || dx12 || modern || smart::native(device)) && preview.hash && preview.hash == pixelHash && preview.hash == huntedHash && preview.original)
        pixelBlocked = false;
    if (vertexBlocked || !pixelBlocked)
    {
        smart::restore(device);
        return vertexBlocked;
    }
    if (other || dx12 || modern || smart::native(device))
    {
        const auto choice = settings.choice(pixelHash, huntedHash, !other && !dx12 && !modern && g_syndicateProfile);
        if (choice && choice->method != smart::Method::Skip)
        {
            if (smart::other::isVK(device))
            {
                smart::other::applyVK(commandList, pixelHandle, pixelHash, *choice);
                return false;
            }
            if (dx12)
            {
                smart::dx12::apply(commandList, pixelHandle, pixelHash, *choice);
                return false;
            }
            if (modern || smart::other::isGL(device))
            {
                if (replacement) *replacement = {pixelHandle, pixelHash, *choice};
                return false;
            }
            smart::apply(device, pixelHandle, pixelHash, *choice);
            return false;
        }
    }
    smart::restore(device);
    return true;
}
static bool prepareD3D12Indirect(command_list* commandList, bool graphics)
{
    if (g_originalShaderMatching.load() && !g_effectFinder.isolates(reinterpret_cast<uintptr_t>(commandList->get_device())))
        return graphics ? blockDrawCallForCommandList(commandList) : originalPassBlocked(commandList);
    if (graphics) (void)blockDrawCallForCommandList(commandList);
    return false;
}
static bool blockDispatchForCommandList(command_list* commandList)
{
    smart::dx12::restore(commandList);
    smart::other::restore(commandList);
 if (nullptr == commandList)
 {
  return false;
 }
    const bool finding=g_effectFinder.isolates(reinterpret_cast<uintptr_t>(commandList->get_device()));
 const auto& commandListData = commandList->get_private_data<CommandListDataContainer>();
 const uint32_t shaderHash = g_computeShaderManager.getShaderHash(commandListData.activeComputeShaderPipeline);
 if (shaderHash == 0)
  return false;
 if (finding) return g_effectFinder.context()->compute.count(shaderHash)!=0;
 bool blockCall = g_computeShaderManager.isBlockedShader(shaderHash);
 for (auto& group : g_toggleGroups)
 {
  for (auto hash : group.getComputeShaderHashes())
  {
   if (!g_allToggleGroupsSuspended && group.isActive() && group.getId() != g_toggleGroupIdShaderEditing && hash == shaderHash)
   {
    blockCall = true;
    break;
   }
  }
 }
 return blockCall;
}
template<class Draw> static bool processDraw(command_list* commands, Draw&& issueDraw)
{
    SmartDrawRequest replacement;
    if (blockDrawCallForCommandList(commands, &replacement)) return true;
    if (!replacement.hash) return false;
    if (smart::other::isGL(commands->get_device()))
    {
        smart::other::applyGLNative(commands, replacement.handle, replacement.hash, replacement.choice);
        return false;
    }
    return smart::modern::draw(commands, replacement.handle, replacement.hash, replacement.choice, issueDraw);
}
static bool onDraw(command_list* commandList, uint32_t count, uint32_t instances, uint32_t first, uint32_t firstInstance)
{
    if(observeEffect(commandList,finder::Kind::Draw,count,instances,0)) return true;
    return processDraw(commandList, [&] { commandList->draw(count, instances, first, firstInstance); });
}
static bool onDrawIndexed(command_list* commandList, uint32_t count, uint32_t instances, uint32_t first, int32_t vertexOffset, uint32_t firstInstance)
{
    if(observeEffect(commandList,finder::Kind::Indexed,count,instances,0)) return true;
    return processDraw(commandList, [&] { commandList->draw_indexed(count, instances, first, vertexOffset, firstInstance); });
}
static bool onDispatch(command_list* commandList, uint32_t x, uint32_t y, uint32_t z)
{
    if(observeEffect(commandList,finder::Kind::Dispatch,x,y,z)) return true;
    if (g_originalShaderMatching.load() && commandList && !g_effectFinder.isolates(reinterpret_cast<uintptr_t>(commandList->get_device())))
    {
        smart::dx12::restore(commandList);
        smart::other::restore(commandList);
        return false;
    }
 return blockDispatchForCommandList(commandList);
}
static bool onDrawOrDispatchIndirect(command_list* commandList, indirect_command type, resource buffer, uint64_t offset, uint32_t count, uint32_t stride)
{
    smart::dx12::restore(commandList);
    smart::other::restore(commandList);
    const bool originalMatching = commandList && g_originalShaderMatching.load() && !g_effectFinder.isolates(reinterpret_cast<uintptr_t>(commandList->get_device()));
    if (originalMatching && smart::dx12::canHandleIndirect(commandList)) return false;
    if(commandList && !smart::dx12::supported(commandList->get_device()))
    {
        if(type==indirect_command::draw && observeEffect(commandList,finder::Kind::IndirectDraw,count,stride,1)) return true;
        if(type==indirect_command::draw_indexed && observeEffect(commandList,finder::Kind::IndirectIndexed,count,stride,1)) return true;
        if(type==indirect_command::dispatch && observeEffect(commandList,finder::Kind::IndirectDispatch,count,stride,1)) return true;
    }
 switch (type)
 {
 case indirect_command::draw:
 case indirect_command::draw_indexed:
  return processDraw(commandList, [&] { commandList->draw_or_dispatch_indirect(type, buffer, offset, count, stride); });
 case indirect_command::dispatch:
        return originalMatching ? originalPassBlocked(commandList) : blockDispatchForCommandList(commandList);
 case indirect_command::unknown:
        return originalMatching && originalPassBlocked(commandList);
 default:
  return false;
 }
}
static void onExecuteSecondary(command_list* commandList, command_list*)
{
    if (!commandList || (!smart::dx12::supported(commandList->get_device()) && !smart::other::isVK(commandList->get_device()))) return;
    smart::dx12::restore(commandList);
    smart::other::restore(commandList);
    smart::dx12::bound(commandList, 0);
    smart::other::bound(commandList, pipeline_stage::all_graphics, 0);
    auto& data = commandList->get_private_data<CommandListDataContainer>();
    data.activePixelShaderPipeline = data.activeVertexShaderPipeline = data.activeComputeShaderPipeline = 0;
}
static void onReshadePresent(effect_runtime* runtime)
{
    const bool wasFinding = runtime == g_finderRuntime;
    if (runtime == g_finderRuntime && g_effectFinder.tick(GetTickCount64()))
    {
        const auto result = g_effectFinder.view(GetTickCount64());
        diagnostics::Write("[FINDER] Capture complete group=%d phase=%s baseline=%llu action=%llu omitted=%llu candidates=%zu total=%zu compute_excluded=%zu",
            result.group, result.frozen ? "paused-scene" : result.stage == finder::Stage::Results ? "with-effect" : "without-effect",
            static_cast<unsigned long long>(result.baselineCalls), static_cast<unsigned long long>(result.actionCalls),
            static_cast<unsigned long long>(result.omitted), result.candidates.size(),result.totalCandidates,result.excludedCompute);
        if(result.focused) diagnostics::Write("[FINDER] Source toggle group=%d original_matching=%u graphics_calls_followed=%llu",
            g_finderFocusGroup,result.followOriginal,static_cast<unsigned long long>(result.followedGraphics));
        for (size_t i = 0; i < std::min<size_t>(result.candidates.size(), 12); ++i)
            diagnostics::Write("[FINDER] Candidate %zu rule=%s baseline=%llu action=%llu", i + 1,
                finder::encode(result.candidates[i].signature).c_str(),
                static_cast<unsigned long long>(result.candidates[i].baseline),
                static_cast<unsigned long long>(result.candidates[i].action));
    }
    if (wasFinding) finderKeys(runtime);
    for (auto* settings : {&g_smartSettings, &g_smartSettings10, &g_smartSettings11, &g_smartSettings12, &g_smartSettingsVK, &g_smartSettingsGL})
    {
        const auto preview = settings->preview();
        if (preview.hash && (g_toggleGroupIdShaderEditing < 0 || preview.hash != g_pixelShaderManager.getActiveHuntedShaderHash()))
            settings->cancel();
    }
 const auto mouseCaptureNow = std::chrono::steady_clock::now();
 const bool mouseCapturedByOverlay =
  g_overlayMouseCaptureLastSeen.time_since_epoch().count() != 0 &&
  std::chrono::duration_cast<std::chrono::milliseconds>(
   mouseCaptureNow - g_overlayMouseCaptureLastSeen).count() <= 100;
 KeyData::setMouseHotkeysBlocked(mouseCapturedByOverlay);
 logPresentDiagnostics(runtime, mouseCaptureNow);
    if (wasFinding)
    {
        for (const auto& group : g_toggleGroups)
            g_groupHotkeyWasDown[group.getId()] = group.getToggleKey().isKeyDown(runtime);
        publishEffectFilters();
        return;
    }
 if (g_activeCollectorFrameCounter > 0)
 {
  if (--g_activeCollectorFrameCounter == 0)
   diagnostics::Write("[HUNT] Collection finished: P=%u V=%u C=%u",
    g_pixelShaderManager.getAmountShaderHashesCollected(), g_vertexShaderManager.getAmountShaderHashesCollected(), g_computeShaderManager.getAmountShaderHashesCollected());
 }
 bool suspensionToggledThisFrame = false;
 if (g_allToggleGroupsSuspended)
 {
  if (isAnyGlobalHotkeyPressed(g_globalRestoreHotkeys, runtime))
  {
   restoreAllToggleGroups();
   suspensionToggledThisFrame = true;
  }
 }
 else
 {
  if (isAnyGlobalHotkeyPressed(g_globalSuspendHotkeys, runtime))
  {
   suspendAllToggleGroups();
   suspensionToggledThisFrame = true;
  }
 }
 if (g_allToggleGroupsSuspended)
 {
  for (auto& group : g_toggleGroups)
  {
   const bool isDownNow = group.getToggleKey().isKeyDown(runtime);
   bool& wasDownLastFrame = g_groupHotkeyWasDown[group.getId()];
   if (!group.isTimedMode() &&
    !group.isHoldMode() &&
    isDownNow &&
    !wasDownLastFrame &&
    !suspensionToggledThisFrame)
   {
    const int groupId = group.getId();
    auto pendingIt = g_pendingSuspendedGroupToggles.find(groupId);
    if (pendingIt == g_pendingSuspendedGroupToggles.end())
     g_pendingSuspendedGroupToggles.insert(groupId);
    else
     g_pendingSuspendedGroupToggles.erase(pendingIt);
   }
   wasDownLastFrame = isDownNow;
  }
 }
 else if (!suspensionToggledThisFrame)
 {
 for (auto& group : g_toggleGroups)
 {
  const auto nowTime = std::chrono::steady_clock::now();
        if(keepGroupMenuState(group,runtime)) continue;
  auto startupTimerIt = g_groupStartupActivationStartTime.find(group.getId());
  if (startupTimerIt != g_groupStartupActivationStartTime.end())
  {
   const auto startupElapsedMs =
    std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - startupTimerIt->second).count();
   if (startupElapsedMs >= group.getStartupDurationMs())
   {
    setGroupActiveWithEditRefresh(group, false);
    g_groupStartupActivationStartTime.erase(startupTimerIt);
   }
  }
  const bool isDownNow = group.getToggleKey().isKeyDown(runtime);
  const bool timedSuppressionKeyDownNow = isAnyTimedSuppressionKeyDown(group, runtime);
  if (timedSuppressionKeyDownNow)
  {
   g_groupLastTimedSuppressionInputTime[group.getId()] = nowTime;
  }
  bool timedSuppressedNow = timedSuppressionKeyDownNow;
  if (!timedSuppressedNow)
  {
   auto suppressionIt = g_groupLastTimedSuppressionInputTime.find(group.getId());
   if (suppressionIt != g_groupLastTimedSuppressionInputTime.end())
   {
    const auto elapsedSinceSuppressionMs =
     std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - suppressionIt->second).count();
    if (elapsedSinceSuppressionMs <= group.getTimedSuppressionLingerMs())
    {
     timedSuppressedNow = true;
    }
    else
    {
     g_groupLastTimedSuppressionInputTime.erase(suppressionIt);
    }
   }
  }
  const bool timedTriggerActiveNow = !timedSuppressedNow && isAnyTimedTriggerActive(group, runtime);
  if (group.isTimedMode())
  {
   const bool timedTargetActiveState = !group.isTimedModeInverted();
   const bool timedRestingActiveState = group.isTimedModeInverted();
   if (timedSuppressedNow)
   {
    setGroupActiveWithEditRefresh(group, timedRestingActiveState);
    g_groupLastTimedTriggerTime.erase(group.getId());
    g_groupTimedVisibleSince.erase(group.getId());
    g_groupTimedFadeOutStart.erase(group.getId());
    g_groupHotkeyWasDown[group.getId()] = isDownNow;
    continue;
   }
   if (timedTriggerActiveNow)
   {
    if (group.isActive() != timedTargetActiveState)
    {
     g_groupTimedVisibleSince[group.getId()] = nowTime;
    }
    setGroupActiveWithEditRefresh(group, timedTargetActiveState);
    g_groupLastTimedTriggerTime[group.getId()] = nowTime;
    g_groupTimedFadeOutStart.erase(group.getId());
   }
   if (group.isActive() == timedTargetActiveState)
   {
    auto lastTriggerIt = g_groupLastTimedTriggerTime.find(group.getId());
    auto visibleSinceIt = g_groupTimedVisibleSince.find(group.getId());
    auto fadeOutIt = g_groupTimedFadeOutStart.find(group.getId());
    if (lastTriggerIt != g_groupLastTimedTriggerTime.end())
    {
     const auto elapsedSinceLastTriggerMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastTriggerIt->second).count();
     long long elapsedVisibleMs = 0;
     if (visibleSinceIt != g_groupTimedVisibleSince.end())
     {
      elapsedVisibleMs =
       std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - visibleSinceIt->second).count();
     }
     const bool hideDelayExpired = elapsedSinceLastTriggerMs >= group.getTimedModeDelayMs();
     const bool minVisibleExpired = elapsedVisibleMs >= group.getTimedModeMinVisibleMs();
     if (hideDelayExpired && minVisibleExpired)
     {
      if (group.getTimedModeFadeOutMs() <= 0)
      {
       setGroupActiveWithEditRefresh(group, timedRestingActiveState);
       g_groupTimedVisibleSince.erase(group.getId());
       g_groupTimedFadeOutStart.erase(group.getId());
      }
      else
      {
       if (fadeOutIt == g_groupTimedFadeOutStart.end())
       {
        g_groupTimedFadeOutStart[group.getId()] = nowTime;
       }
       else
       {
        const auto fadeElapsedMs =
         std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - fadeOutIt->second).count();
        if (fadeElapsedMs >= group.getTimedModeFadeOutMs())
        {
         setGroupActiveWithEditRefresh(group, timedRestingActiveState);
         g_groupTimedVisibleSince.erase(group.getId());
         g_groupTimedFadeOutStart.erase(group.getId());
        }
       }
      }
     }
     else
     {
      g_groupTimedFadeOutStart.erase(group.getId());
     }
    }
   }
   else
   {
    g_groupTimedVisibleSince.erase(group.getId());
    g_groupTimedFadeOutStart.erase(group.getId());
   }
   g_groupHotkeyWasDown[group.getId()] = isDownNow;
   continue;
  }
  if (group.isHoldMode())
  {
   const bool desiredActive = group.isHoldInverted() ? !isDownNow : isDownNow;
   setGroupActiveWithEditRefresh(group, desiredActive);
   g_groupHotkeyWasDown[group.getId()] = isDownNow;
   continue;
  }
  bool& wasDownLastFrame = g_groupHotkeyWasDown[group.getId()];
  auto& lastToggleTime = g_groupHotkeyLastToggleTime[group.getId()];
  const auto msSinceLastToggle =
   std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastToggleTime).count();
  if (isDownNow && !wasDownLastFrame && msSinceLastToggle > g_groupHotkeyDebounceMs)
  {
   setGroupActiveWithEditRefresh(group, !group.isActive());
   lastToggleTime = nowTime;
  }
  wasDownLastFrame = isDownNow;
 }
 }
 publishEffectFilters();
 const bool ctrlDown = runtime->is_key_down(VK_CONTROL);
 auto now = std::chrono::steady_clock::now();
 const int NP1 = VK_NUMPAD1;
 const int NP2 = VK_NUMPAD2;
 const int NP3 = VK_NUMPAD3;
 const int NP4 = VK_NUMPAD4;
 const int NP5 = VK_NUMPAD5;
 const int NP6 = VK_NUMPAD6;
 const int NP7 = VK_NUMPAD7;
 const int NP8 = VK_NUMPAD8;
 const int NP9 = VK_NUMPAD9;
 bool np1Down = is_key_down_numpad_only(runtime, NP1);
 bool np1Pressed = np1Down && !s_prevNP1Down;
 if (np1Pressed)
 {
  g_pixelShaderManager.huntPreviousShader(ctrlDown);
  s_lastNP1 = now;
  s_holdStartNP1 = now;
  s_np1Held = true;
 }
 else if (np1Down && s_np1Held &&
  std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP1).count() >= getAcceleratedRepeatMs(s_holdStartNP1, now))
 {
  g_pixelShaderManager.huntPreviousShader(ctrlDown);
  s_lastNP1 = now;
 }
 else if (!np1Down)
 {
  s_np1Held = false;
 }
 s_prevNP1Down = np1Down;
 bool np2Down = is_key_down_numpad_only(runtime, NP2);
 bool np2Pressed = np2Down && !s_prevNP2Down;
 if (np2Pressed)
 {
  g_pixelShaderManager.huntNextShader(ctrlDown);
  s_lastNP2 = now;
  s_holdStartNP2 = now;
  s_np2Held = true;
 }
 else if (np2Down && s_np2Held &&
  std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP2).count() >= getAcceleratedRepeatMs(s_holdStartNP2, now))
 {
  g_pixelShaderManager.huntNextShader(ctrlDown);
  s_lastNP2 = now;
 }
 else if (!np2Down)
 {
  s_np2Held = false;
 }
 s_prevNP2Down = np2Down;
 bool np3Down = is_key_down_numpad_only(runtime, NP3);
 bool np3Pressed = np3Down && !s_prevNP3Down;
 if (np3Pressed)
 {
  g_pixelShaderManager.toggleMarkOnHuntedShader();
 }
 s_prevNP3Down = np3Down;
 bool np4Down = is_key_down_numpad_only(runtime, NP4);
 bool np4Pressed = np4Down && !s_prevNP4Down;
 if (np4Pressed)
 {
  g_vertexShaderManager.huntPreviousShader(ctrlDown);
  s_lastNP4 = now;
  s_holdStartNP4 = now;
  s_np4Held = true;
 }
 else if (np4Down && s_np4Held &&
  std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP4).count() >= getAcceleratedRepeatMs(s_holdStartNP4, now))
 {
  g_vertexShaderManager.huntPreviousShader(ctrlDown);
  s_lastNP4 = now;
 }
 else if (!np4Down)
 {
  s_np4Held = false;
 }
 s_prevNP4Down = np4Down;
 bool np5Down = is_key_down_numpad_only(runtime, NP5);
 bool np5Pressed = np5Down && !s_prevNP5Down;
 if (np5Pressed)
 {
  g_vertexShaderManager.huntNextShader(ctrlDown);
  s_lastNP5 = now;
  s_holdStartNP5 = now;
  s_np5Held = true;
 }
 else if (np5Down && s_np5Held &&
  std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP5).count() >= getAcceleratedRepeatMs(s_holdStartNP5, now))
 {
  g_vertexShaderManager.huntNextShader(ctrlDown);
  s_lastNP5 = now;
 }
 else if (!np5Down)
 {
  s_np5Held = false;
 }
 s_prevNP5Down = np5Down;
 bool np6Down = is_key_down_numpad_only(runtime, NP6);
 bool np6Pressed = np6Down && !s_prevNP6Down;
 if (np6Pressed)
 {
  g_vertexShaderManager.toggleMarkOnHuntedShader();
 }
 s_prevNP6Down = np6Down;
 bool np7Down = is_key_down_numpad_only(runtime, NP7);
 bool np7Pressed = np7Down && !s_prevNP7Down;
 if (np7Pressed)
 {
  g_computeShaderManager.huntPreviousShader(ctrlDown);
  s_lastNP7 = now;
  s_holdStartNP7 = now;
  s_np7Held = true;
 }
 else if (np7Down && s_np7Held &&
  std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP7).count() >= getAcceleratedRepeatMs(s_holdStartNP7, now))
 {
  g_computeShaderManager.huntPreviousShader(ctrlDown);
  s_lastNP7 = now;
 }
 else if (!np7Down)
 {
  s_np7Held = false;
 }
 s_prevNP7Down = np7Down;
 bool np8Down = is_key_down_numpad_only(runtime, NP8);
 bool np8Pressed = np8Down && !s_prevNP8Down;
 if (np8Pressed)
 {
  g_computeShaderManager.huntNextShader(ctrlDown);
  s_lastNP8 = now;
  s_holdStartNP8 = now;
  s_np8Held = true;
 }
 else if (np8Down && s_np8Held &&
  std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP8).count() >= getAcceleratedRepeatMs(s_holdStartNP8, now))
 {
  g_computeShaderManager.huntNextShader(ctrlDown);
  s_lastNP8 = now;
 }
 else if (!np8Down)
 {
  s_np8Held = false;
 }
 s_prevNP8Down = np8Down;
 bool np9Down = is_key_down_numpad_only(runtime, NP9);
 bool np9Pressed = np9Down && !s_prevNP9Down;
 if (np9Pressed)
 {
  g_computeShaderManager.toggleMarkOnHuntedShader();
 }
 s_prevNP9Down = np9Down;
}
static void cancelAllKeyBindingEditors()
{
 g_toggleGroupIdKeyBindingEditing = -1;
 g_toggleGroupIdTimedTriggerKeyEditing = -1;
 g_toggleGroupTimedTriggerKeySlotEditing = -1;
 g_toggleGroupIdTimedSuppressionKeyEditing = -1;
 g_toggleGroupTimedSuppressionKeySlotEditing = -1;
 g_globalSuspendHotkeySlotEditing = -1;
 g_globalRestoreHotkeySlotEditing = -1;
 g_keyCollector.clear();
}
static void endGlobalHotkeyEditing(
 bool editingSuspendList,
 bool acceptCollectedBinding)
{
 std::vector<KeyData>& hotkeys =
  editingSuspendList ? g_globalSuspendHotkeys : g_globalRestoreHotkeys;
 std::atomic_int& editingSlot =
  editingSuspendList ? g_globalSuspendHotkeySlotEditing : g_globalRestoreHotkeySlotEditing;
 const int slotIndex = editingSlot;
 if (acceptCollectedBinding && slotIndex >= 0 && g_keyCollector.isValid())
 {
  if (!globalHotkeyExists(hotkeys, g_keyCollector, slotIndex))
  {
   if (slotIndex < static_cast<int>(hotkeys.size()))
    hotkeys[static_cast<size_t>(slotIndex)] = g_keyCollector;
   else if (slotIndex == static_cast<int>(hotkeys.size()))
    hotkeys.push_back(g_keyCollector);
   saveShaderTogglerIniFile();
  }
 }
 editingSlot = -1;
 g_keyCollector.clear();
}
static void startGlobalHotkeyEditing(bool editingSuspendList, int slotIndex)
{
 std::atomic_int& editingSlot =
  editingSuspendList ? g_globalSuspendHotkeySlotEditing : g_globalRestoreHotkeySlotEditing;
 if (editingSlot == slotIndex)
  return;
 cancelAllKeyBindingEditors();
 editingSlot = slotIndex;
 g_keyCollector.prepareForBindingCollection();
}
void endKeyBindingEditing(bool acceptCollectedBinding, ToggleGroup& groupEditing)
{
 if (acceptCollectedBinding && g_toggleGroupIdKeyBindingEditing == groupEditing.getId() && g_keyCollector.isValid())
 {
  groupEditing.setToggleKey(g_keyCollector);
 }
 g_toggleGroupIdKeyBindingEditing = -1;
 g_keyCollector.clear();
}
void startKeyBindingEditing(ToggleGroup& groupEditing)
{
 if (g_globalSuspendHotkeySlotEditing >= 0 ||
  g_globalRestoreHotkeySlotEditing >= 0)
 {
  g_globalSuspendHotkeySlotEditing = -1;
  g_globalRestoreHotkeySlotEditing = -1;
  g_keyCollector.clear();
 }
 if (g_toggleGroupIdKeyBindingEditing == groupEditing.getId())
 {
  return;
 }
 if (g_toggleGroupIdKeyBindingEditing >= 0)
 {
  endKeyBindingEditing(false, groupEditing);
 }
 if (g_toggleGroupIdTimedTriggerKeyEditing >= 0)
 {
  g_toggleGroupIdTimedTriggerKeyEditing = -1;
  g_toggleGroupTimedTriggerKeySlotEditing = -1;
  g_keyCollector.clear();
 }
 if (g_toggleGroupIdTimedSuppressionKeyEditing >= 0)
 {
  g_toggleGroupIdTimedSuppressionKeyEditing = -1;
  g_toggleGroupTimedSuppressionKeySlotEditing = -1;
  g_keyCollector.clear();
 }
 g_toggleGroupIdKeyBindingEditing = groupEditing.getId();
 g_keyCollector.prepareForBindingCollection();
}
void endTimedTriggerKeyBindingEditing(bool acceptCollectedBinding, ToggleGroup& groupEditing)
{
 if (acceptCollectedBinding &&
  g_toggleGroupIdTimedTriggerKeyEditing == groupEditing.getId() &&
  g_toggleGroupTimedTriggerKeySlotEditing >= 0 &&
  g_keyCollector.isValid())
 {
  if (g_toggleGroupTimedTriggerKeySlotEditing < static_cast<int>(groupEditing.getTimedTriggerKeyCount()))
  {
   auto binding = groupEditing.getTimedTriggerBindingAt(static_cast<size_t>(g_toggleGroupTimedTriggerKeySlotEditing));
   binding.key = g_keyCollector;
   groupEditing.setTimedTriggerBindingAt(static_cast<size_t>(g_toggleGroupTimedTriggerKeySlotEditing), binding);
  }
  else
  {
   groupEditing.addTimedTriggerKey(g_keyCollector, ToggleGroup::TimedTriggerMode::OnPress);
  }
 }
 g_toggleGroupIdTimedTriggerKeyEditing = -1;
 g_toggleGroupTimedTriggerKeySlotEditing = -1;
 g_keyCollector.clear();
}
void startTimedTriggerKeyBindingEditing(ToggleGroup& groupEditing, int slotIndex)
{
 if (g_globalSuspendHotkeySlotEditing >= 0 ||
  g_globalRestoreHotkeySlotEditing >= 0)
 {
  g_globalSuspendHotkeySlotEditing = -1;
  g_globalRestoreHotkeySlotEditing = -1;
  g_keyCollector.clear();
 }
 if (g_toggleGroupIdTimedTriggerKeyEditing == groupEditing.getId() &&
  g_toggleGroupTimedTriggerKeySlotEditing == slotIndex)
 {
  return;
 }
 if (g_toggleGroupIdTimedTriggerKeyEditing >= 0)
 {
  endTimedTriggerKeyBindingEditing(false, groupEditing);
 }
 if (g_toggleGroupIdKeyBindingEditing >= 0)
 {
  g_toggleGroupIdKeyBindingEditing = -1;
  g_keyCollector.clear();
 }
 if (g_toggleGroupIdTimedSuppressionKeyEditing >= 0)
 {
  g_toggleGroupIdTimedSuppressionKeyEditing = -1;
  g_toggleGroupTimedSuppressionKeySlotEditing = -1;
  g_keyCollector.clear();
 }
 g_toggleGroupIdTimedTriggerKeyEditing = groupEditing.getId();
 g_toggleGroupTimedTriggerKeySlotEditing = slotIndex;
 g_keyCollector.prepareForBindingCollection();
}
void endTimedSuppressionKeyBindingEditing(bool acceptCollectedBinding, ToggleGroup& groupEditing)
{
 if (acceptCollectedBinding &&
  g_toggleGroupIdTimedSuppressionKeyEditing == groupEditing.getId() &&
  g_toggleGroupTimedSuppressionKeySlotEditing >= 0 &&
  g_keyCollector.isValid())
 {
  groupEditing.setTimedSuppressionKeyAt(
   static_cast<size_t>(g_toggleGroupTimedSuppressionKeySlotEditing),
   g_keyCollector);
 }
 g_toggleGroupIdTimedSuppressionKeyEditing = -1;
 g_toggleGroupTimedSuppressionKeySlotEditing = -1;
 g_keyCollector.clear();
}
void startTimedSuppressionKeyBindingEditing(ToggleGroup& groupEditing, int slotIndex)
{
 if (g_globalSuspendHotkeySlotEditing >= 0 ||
  g_globalRestoreHotkeySlotEditing >= 0)
 {
  g_globalSuspendHotkeySlotEditing = -1;
  g_globalRestoreHotkeySlotEditing = -1;
  g_keyCollector.clear();
 }
 if (g_toggleGroupIdTimedSuppressionKeyEditing == groupEditing.getId() &&
  g_toggleGroupTimedSuppressionKeySlotEditing == slotIndex)
 {
  return;
 }
 if (g_toggleGroupIdTimedSuppressionKeyEditing >= 0)
 {
  endTimedSuppressionKeyBindingEditing(false, groupEditing);
 }
 if (g_toggleGroupIdKeyBindingEditing >= 0)
 {
  g_toggleGroupIdKeyBindingEditing = -1;
  g_keyCollector.clear();
 }
 if (g_toggleGroupIdTimedTriggerKeyEditing >= 0)
 {
  g_toggleGroupIdTimedTriggerKeyEditing = -1;
  g_toggleGroupTimedTriggerKeySlotEditing = -1;
  g_keyCollector.clear();
 }
 g_toggleGroupIdTimedSuppressionKeyEditing = groupEditing.getId();
 g_toggleGroupTimedSuppressionKeySlotEditing = slotIndex;
 g_keyCollector.prepareForBindingCollection();
}
void endShaderEditing(bool acceptCollectedShaderHashes, ToggleGroup& groupEditing)
{
 diagnostics::Write("[HUNT] End group=%d accept=%u", groupEditing.getId(), acceptCollectedShaderHashes && g_toggleGroupIdShaderEditing == groupEditing.getId());
 if (acceptCollectedShaderHashes && g_toggleGroupIdShaderEditing == groupEditing.getId())
 {
  groupEditing.storeCollectedHashes(
   g_pixelShaderManager.getMarkedShaderHashes(),
   g_vertexShaderManager.getMarkedShaderHashes(),
   g_computeShaderManager.getMarkedShaderHashes());
  logGroupConfiguration(groupEditing);
 }
 cancelSmartPreviews();
 g_pixelShaderManager.stopHuntingMode();
 g_vertexShaderManager.stopHuntingMode();
 g_computeShaderManager.stopHuntingMode();
 g_toggleGroupIdShaderEditing = -1;
}
void startShaderEditing(ToggleGroup& groupEditing)
{
    closeEffectFinder();
 if (g_toggleGroupIdShaderEditing == groupEditing.getId())
 {
  return;
 }
 if (g_toggleGroupIdShaderEditing >= 0)
 {
  endShaderEditing(false, groupEditing);
 }
 g_toggleGroupIdShaderEditing = groupEditing.getId();
 diagnostics::Write("[HUNT] Begin group=%d name=%.120s collection_frames=%d", groupEditing.getId(), groupEditing.getName().c_str(), g_startValueFramecountCollectionPhase);
 g_activeCollectorFrameCounter = g_startValueFramecountCollectionPhase;
 g_pixelShaderManager.startHuntingMode(groupEditing.getPixelShaderHashes());
 g_vertexShaderManager.startHuntingMode(groupEditing.getVertexShaderHashes());
 g_computeShaderManager.startHuntingMode(groupEditing.getComputeShaderHashes());
 cancelSmartPreviews();
}
static void showHelpMarker(const char* desc)
{
 ImGui::TextDisabled("(?)");
 if (ImGui::IsItemHovered())
 {
  ImGui::BeginTooltip();
  ImGui::PushTextWrapPos(ImGui::GetFontSize()*30.0f);
  ImGui::TextUnformatted(desc);
  ImGui::PopTextWrapPos();
  ImGui::EndTooltip();
 }
}
static void forgetGroupRuntimeState(int id)
{
    g_groupHotkeyWasDown.erase(id);
    g_groupMenuInputs.erase(id);
    g_groupHotkeyLastToggleTime.erase(id);
    g_groupLastTimedTriggerTime.erase(id);
    g_groupTimedVisibleSince.erase(id);
    g_groupTimedFadeOutStart.erase(id);
    g_groupLastTimedSuppressionInputTime.erase(id);
    g_groupStartupActivationStartTime.erase(id);
    g_pendingSuspendedGroupToggles.erase(id);
}
static bool removeToggleGroups(const std::vector<int>& ids)
{
    const auto removing=[&](int id){return std::find(ids.begin(),ids.end(),id)!=ids.end();};
    if(!std::any_of(g_toggleGroups.begin(),g_toggleGroups.end(),[&](const auto& group){return removing(group.getId());})) return false;
    for(int id:ids)
        if(id==g_finderGroup || id==g_finderFocusGroup || g_finderKeepGroups.count(id)) {closeEffectFinder();break;}
    if(removing(g_toggleGroupIdKeyBindingEditing) || removing(g_toggleGroupIdTimedTriggerKeyEditing) || removing(g_toggleGroupIdTimedSuppressionKeyEditing))
        cancelAllKeyBindingEditors();
    for(auto& group:g_toggleGroups)
        if(group.getId()==g_toggleGroupIdShaderEditing && removing(group.getId()))
        {
            endShaderEditing(false,group);
            g_activeCollectorFrameCounter=0;
            break;
        }
    g_toggleGroups.erase(std::remove_if(g_toggleGroups.begin(),g_toggleGroups.end(),[&](const auto& group){return removing(group.getId());}),g_toggleGroups.end());
    for(int id:ids) forgetGroupRuntimeState(id);
    return saveShaderTogglerIniFile();
}
static bool mergeToggleGroups(int destination,const std::set<int>& sources,bool removeSources)
{
    if(g_finderRuntime || g_toggleGroupIdShaderEditing>=0) return false;
    auto prepared=prepareGroupMerge(g_toggleGroups,destination,sources);
    if(!prepared.group)
    {
        diagnostics::Write("[MERGE] Not applied target=%d sources=%zu reason=%u unique_filters=%zu",
            destination,sources.size(),static_cast<unsigned>(prepared.error),prepared.filterCount);
        return false;
    }
    auto next=g_toggleGroups;
    for(auto& group:next) if(group.getId()==destination) { group=*prepared.group;break; }
    if(removeSources)
        next.erase(std::remove_if(next.begin(),next.end(),[&](const auto& g){return sources.count(g.getId())!=0;}),next.end());
    cancelAllKeyBindingEditors();
    g_toggleGroups.swap(next);
    if(removeSources) for(int id:sources) forgetGroupRuntimeState(id);
    publishEffectFilters();
    diagnostics::Write("[MERGE] Applied target=%d sources=%zu remove_sources=%u shaders(P/V/C)=%zu/%zu/%zu unique_filters=%zu",
        destination,sources.size(),removeSources,prepared.group->getPixelShaderHashes().size(),
        prepared.group->getVertexShaderHashes().size(),prepared.group->getComputeShaderHashes().size(),prepared.filterCount);
    for(int id:sources) diagnostics::Write("[MERGE] Source group=%d target=%d",id,destination);
    saveShaderTogglerIniFile();
    return true;
}
static void displayGroupMergeControls()
{
    if(!g_mergePanelOpen) return;
    const auto exists=[](int id){return std::any_of(g_toggleGroups.begin(),g_toggleGroups.end(),[&](const auto& g){return g.getId()==id;});};
    if(!exists(g_mergeDestination))
    {
        const auto refined=std::find_if(g_toggleGroups.begin(),g_toggleGroups.end(),[](const auto& g){return !g.getEffectFilters().empty();});
        g_mergeDestination=refined!=g_toggleGroups.end()?refined->getId():g_toggleGroups.empty()?-1:g_toggleGroups.front().getId();
    }
    for(auto it=g_mergeSources.begin();it!=g_mergeSources.end();)
        if(*it==g_mergeDestination || !exists(*it)) it=g_mergeSources.erase(it);else ++it;
    std::string destinationName="Select a group";
    for(const auto& group:g_toggleGroups) if(group.getId()==g_mergeDestination) destinationName=group.getName();
    ImGui::Separator();
    addonHeading("Merge toggle groups");
    {
        const ScopedAddonControlStyle controls;
        ImGui::AlignTextToFramePadding();ImGui::TextUnformatted("Merge into");
        if(ImGui::GetContentRegionAvail().x>ImGui::CalcTextSize("Merge into").x+ImGui::GetStyle().ItemSpacing.x+ImGui::GetFontSize()*10)
            ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        const bool open=ImGui::BeginCombo("##MergeDestination",destinationName.c_str());
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("Merge into: %s\nClick to choose the destination group.",destinationName.c_str());
        if(open)
        {
            for(const auto& group:g_toggleGroups)
            {
                ImGui::PushID(group.getId());
                const bool selected=g_mergeDestination==group.getId();
                if(ImGui::Selectable(group.getName().c_str(),selected))
                { g_mergeDestination=group.getId();g_mergeSources.erase(group.getId());g_mergeMessage.clear(); }
                if(selected) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
    }
    ImGui::TextUnformatted("Select the groups to combine:");
    ImGui::SameLine();showHelpMarker("The destination keeps its name, hotkey, active state and startup/hold/timed settings. Only shaders and precise filters are combined.");
    ImGui::BeginChild("Merge source groups",ImVec2(0,ImGui::GetTextLineHeightWithSpacing()*6),true);
    for(const auto& group:g_toggleGroups)
    {
        if(group.getId()==g_mergeDestination) continue;
        ImGui::PushID(group.getId());
        bool selected=g_mergeSources.count(group.getId())!=0;
        if(ImGui::Checkbox(group.getName().c_str(),&selected))
        { if(selected) g_mergeSources.insert(group.getId());else g_mergeSources.erase(group.getId());g_mergeMessage.clear(); }
        ImGui::SameLine();ImGui::TextDisabled("(%zu filters, %zu shaders)",group.getEffectFilters().size(),
            group.getPixelShaderHashes().size()+group.getVertexShaderHashes().size()+group.getComputeShaderHashes().size());
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::Checkbox("Remove merged source groups",&g_mergeRemoveSources);
    if(!g_mergeRemoveSources) addonHelpText("Source groups keep their current hotkeys and settings.");
    const auto prepared=prepareGroupMerge(g_toggleGroups,g_mergeDestination,g_mergeSources);
    if(prepared.group)
    {
        const size_t shaders=prepared.group->getPixelShaderHashes().size()+prepared.group->getVertexShaderHashes().size()+prepared.group->getComputeShaderHashes().size();
        ImGui::TextWrapped("Result: %zu groups into one, %zu unique filters, %zu shaders.",g_mergeSources.size()+1,prepared.filterCount,shaders);
        if(shaders) ImGui::TextWrapped("This selection includes whole-shader rules. They will remain broad toggles in the merged group.");
    }
    else if(prepared.error==GroupMergeError::TooManyFilters)
        ImGui::TextWrapped("The combined result has %zu unique filters; the limit is %zu. Select fewer groups. Nothing has been merged.",prepared.filterCount,finder::maxRules);
    else if(prepared.error==GroupMergeError::InvalidFilter) ImGui::TextWrapped("A selected group contains an invalid filter. Nothing has been merged.");
    else ImGui::TextWrapped("Choose a destination and at least one other group.");
    const bool busy=g_finderRuntime || g_toggleGroupIdShaderEditing>=0;
    if(busy) ImGui::TextWrapped("Finish shader hunting or close the effect finder before merging.");
    {
        const ScopedAddonControlStyle controls;
        ImGui::BeginDisabled(busy || !prepared.group);
        {
            const ScopedAddonControlStyle primary(true);
            if(ImGui::Button("Merge selected groups"))
            {
                if(mergeToggleGroups(g_mergeDestination,g_mergeSources,g_mergeRemoveSources))
                { g_mergeMessage="Merged into "+prepared.group->getName()+".";g_mergeSources.clear(); }
                else g_mergeMessage="Merge could not be applied. Your groups were not changed.";
            }
        }
        ImGui::EndDisabled();
        nextUiAction("Close merge controls");
        if(ImGui::Button("Close merge controls")) { g_mergePanelOpen=false;g_mergeSources.clear();g_mergeMessage.clear(); }
    }
    if(!g_mergeMessage.empty()) ImGui::TextWrapped("%s",g_mergeMessage.c_str());
    ImGui::Separator();
}
static void addFinderGroup(finder::Match& match, const ToggleGroup& group)
{
    match.pixel.insert(group.getPixelShaderHashes().begin(), group.getPixelShaderHashes().end());
    match.vertex.insert(group.getVertexShaderHashes().begin(), group.getVertexShaderHashes().end());
    match.compute.insert(group.getComputeShaderHashes().begin(), group.getComputeShaderHashes().end());
    match.rules.insert(group.getEffectFilters().begin(), group.getEffectFilters().end());
}
static void refreshFinderScene()
{
    finder::Match kept, focus;
    for (const auto& group : g_toggleGroups)
    {
        if (group.getId() == g_finderFocusGroup) addFinderGroup(focus, group);
        if (group.getId() != g_finderGroup && group.getId() != g_finderFocusGroup && g_finderKeepGroups.count(group.getId()))
            addFinderGroup(kept, group);
    }
    const bool original=g_originalShaderMatching.load();
    diagnostics::Write("[FINDER] Paused scene settings group=%d focus_group=%d kept_groups=%zu original_matching=%u source(P/V/C/rules)=%zu/%zu/%zu/%zu; capture reset.",
        g_finderGroup,g_finderFocusGroup,g_finderKeepGroups.size(),original,focus.pixel.size(),focus.vertex.size(),focus.compute.size(),focus.rules.size());
    g_effectFinder.configureScene(std::move(kept), std::move(focus),original);
}
static void displayFinderSceneSettings()
{
    if (!addonTreeNode("HUD visibility and shader selection")) return;
    std::string focusName = "All visible shaders";
    for (const auto& group : g_toggleGroups)
        if (group.getId() == g_finderFocusGroup) focusName = group.getName();
    bool changed = false;
    {
        ImGui::TextUnformatted("Look for shaders from");
        ImGui::SameLine();showHelpMarker("Select a working toggle to narrow the scan. With original shader matching enabled, the finder follows its affected graphics calls, including remembered compute matches. Its effects stay visible during capture.");
        const ScopedAddonControlStyle controls;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##FinderSource", focusName.c_str()))
        {
            if (ImGui::Selectable("All visible shaders", g_finderFocusGroup < 0)) { g_finderFocusGroup = -1; changed = true; }
            if(g_finderFocusGroup<0) ImGui::SetItemDefaultFocus();
            for (const auto& group : g_toggleGroups)
            {
                finder::Match match;addFinderGroup(match, group);
                if (match.empty()) continue;
                ImGui::PushID(group.getId());
                const bool selected=group.getId()==g_finderFocusGroup;
                if (ImGui::Selectable(group.getName().c_str(), selected))
                {
                    g_finderFocusGroup = group.getId();g_finderKeepGroups.erase(group.getId());changed = true;
                }
                if(selected) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
    }
    ImGui::TextUnformatted("Keep these groups enabled while finding:");
    ImGui::SameLine();showHelpMarker("Check your HUD/pause-menu hiding group. This only affects this finder session; normal activation settings are preserved. Changing the source or these checkboxes clears the current scan.");
    const auto count=std::count_if(g_toggleGroups.begin(),g_toggleGroups.end(),[](const auto& group){return group.getId()!=g_finderGroup && group.getId()!=g_finderFocusGroup;});
    ImGui::BeginChild("Finder kept groups",ImVec2(0,ImGui::GetFrameHeightWithSpacing()*std::clamp(static_cast<int>(count),1,6)+ImGui::GetStyle().WindowPadding.y*2),true);
    for (const auto& group : g_toggleGroups)
    {
        if (group.getId() == g_finderGroup || group.getId() == g_finderFocusGroup) continue;
        ImGui::PushID(group.getId());
        bool keep = g_finderKeepGroups.count(group.getId()) != 0;
        if (ImGui::Checkbox(group.getName().c_str(), &keep))
        {
            if (keep) g_finderKeepGroups.insert(group.getId());else g_finderKeepGroups.erase(group.getId());
            changed = true;
        }
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s",group.getName().c_str());
        ImGui::PopID();
    }
    if(!count) addonHelpText("No other groups available.");
    ImGui::EndChild();
    addonHelpText("After changing these settings, click Scan paused scene again.");
    if (changed) refreshFinderScene();
    ImGui::TreePop();
}
static void openEffectFinder(effect_runtime* runtime, ToggleGroup& group, int sourceGroup = -1)
{
    if (runtime == g_finderRuntime && group.getId() == g_finderGroup) return;
    if (g_toggleGroupIdShaderEditing >= 0) endShaderEditing(false, group);
    g_activeCollectorFrameCounter = 0;
    cancelSmartPreviews();
    cancelAllKeyBindingEditors();
    g_finderGroup = group.getId();
    g_finderRuntime = runtime;
    g_finderKeyboardCaptured = ImGui::GetIO().WantCaptureKeyboard;
    const int keys[] = {VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD5};
    for (size_t i = 0; i < 5; ++i) g_finderKeysDown[i] = runtime->is_key_down(keys[i]);
    g_effectFinder.open(reinterpret_cast<uintptr_t>(runtime->get_device()), group.getId());
    g_finderKeepGroups.clear();g_finderFocusGroup = -1;
    if (g_finderMode == 1 || sourceGroup >= 0)
    {
        finder::Match selection;addFinderGroup(selection, group);
        if (sourceGroup >= 0) g_finderFocusGroup = sourceGroup;
        else if (!selection.empty()) g_finderFocusGroup = group.getId();
        if (!g_allToggleGroupsSuspended)
            for (const auto& other : g_toggleGroups)
                if (other.getId() != group.getId() && other.isActive()) g_finderKeepGroups.insert(other.getId());
        refreshFinderScene();
        g_effectFinder.beginFrozen(GetTickCount64());
        diagnostics::Write("[FINDER] Paused scene capture group=%d; 2s preparation, 2s scan, then individual browsing.", group.getId());
    }
    else
    {
        g_effectFinder.begin(false, GetTickCount64(), true);
        diagnostics::Write("[FINDER] Guided capture group=%d; idle 8s, prepare 4s, action 8s, then bounded batch tests.", group.getId());
    }
    publishEffectFilters();
}
static void refineEffect(effect_runtime* runtime, int sourceGroup)
{
    if(runtime==g_finderRuntime && g_finderFocusGroup==sourceGroup && g_finderGroup!=sourceGroup) return;
    const auto source=std::find_if(g_toggleGroups.begin(),g_toggleGroups.end(),[&](const auto& g){return g.getId()==sourceGroup;});
    if(source==g_toggleGroups.end()) return;
    finder::Match match;addFinderGroup(match,*source);
    if(match.empty()) return;
    const std::string prefix=source->getName()+" - Refined";
    std::string name=prefix;
    for(unsigned i=2;std::any_of(g_toggleGroups.begin(),g_toggleGroups.end(),[&](const auto& g){return g.getName()==name;});++i)
        name=prefix+" "+std::to_string(i);
    ToggleGroup destination(name,ToggleGroup::getNewGroupId());
    destination.setEditing(true);
    g_toggleGroups.push_back(std::move(destination));
    openEffectFinder(runtime,g_toggleGroups.back(),sourceGroup);
    diagnostics::Write("[FINDER] Refine source_group=%d destination_group=%d; paused graphics scan.",sourceGroup,g_finderGroup);
}
static void displayEffectFinder(effect_runtime* runtime, ToggleGroup& group)
{
    if (runtime != g_finderRuntime || group.getId() != g_finderGroup) return;
    auto view = g_effectFinder.view(GetTickCount64());
    ImGui::Separator();
    addonHeading(view.frozen ? (g_finderFocusGroup>=0?"Refine effect - paused scene":"Effect finder - paused scene") : "Effect finder - live comparison");
    if (view.frozen)
    {
        displayFinderSceneSettings();
        view = g_effectFinder.view(GetTickCount64());
        if(g_finderFocusGroup>=0 && !view.followOriginal)
            ImGui::TextWrapped("Strict matching: enable Use original shader matching before rescanning to follow compute-based toggles into graphics draws.");
    }
    if (!group.getPixelShaderHashes().empty() || !group.getVertexShaderHashes().empty() || !group.getComputeShaderHashes().empty())
        ImGui::TextWrapped("This group also disables entire shaders. Use a new, empty group if those rules hide effects you want to keep.");
    finderStatus(view,false);
    if (view.stage == finder::Stage::Results && !view.candidates.empty())
    {
        ImGui::Separator();
        if (view.quick)
        {
            ImGui::BeginDisabled(!view.preview || !view.testing);
            if (ImGui::Button(view.testing == 1 ? "Try another (Num 2)" : "Still visible (Num 2)"))
                answerFinder(false);
            ImGui::EndDisabled();
            nextUiAction(view.testing == 1 ? "Keep and finish (Num 3)" : "Effect gone (Num 3)");
            ImGui::BeginDisabled(!view.preview || !view.previewHits || !view.testing);
            const bool accept = ImGui::Button(view.testing == 1 ? "Keep and finish (Num 3)" : "Effect gone (Num 3)");
            ImGui::EndDisabled();
            if (accept)
            {
                answerFinder(true);
                if (g_finderRuntime != runtime) return;
            }
            ImGui::BeginDisabled(!view.undoSteps);
            if (ImGui::Button("Undo answer (Num 1)")) g_effectFinder.back();
            ImGui::EndDisabled();
        }
        else
        {
            if (ImGui::Button("Previous (Num 1)")) g_effectFinder.select((view.selected + view.candidates.size() - 1) % view.candidates.size(), true);
            nextUiAction("Next (Num 2)");
            if (ImGui::Button("Next (Num 2)")) g_effectFinder.select((view.selected + 1) % view.candidates.size(), true);
            ImGui::BeginDisabled(!view.preview || !view.previewHits);
            const bool accept = ImGui::Button("Keep and finish (Num 3)");
            ImGui::EndDisabled();
            if (accept)
            {
                keepFinderFilter();
                if (g_finderRuntime != runtime) return;
            }
        }
        nextUiAction(view.preview ? "Show original (Num 0)" : "Show preview (Num 0)");
        if (ImGui::Button(view.preview ? "Show original (Num 0)" : "Show preview (Num 0)")) g_effectFinder.compare();
        view = g_effectFinder.view(GetTickCount64());
        if (view.quick && !view.testing && ImGui::Button("Restart quick testing")) g_effectFinder.quickTest();
        if (group.getEffectFilters().size() >= finder::maxRules)
            ImGui::TextWrapped("This group has reached its 64-filter limit. Remove an old filter before saving another.");
    }
    if (view.totalCandidates > view.candidates.size())
        ImGui::TextWrapped(view.frozen ? "Showing the first %u of %u candidates. Narrow the scan using HUD visibility and shader selection, then scan again." :
            "Showing the first %u of %u candidates. Repeat the captures with fewer unrelated effects visible to narrow the list.", static_cast<unsigned>(view.candidates.size()), static_cast<unsigned>(view.totalCandidates));
    if (view.omitted)
        ImGui::TextWrapped("Some calls could not be classified or exceeded the capture limit. The shortlist may be incomplete.");
    if (view.frozen)
    {
        if (ImGui::Button("Scan paused scene")) g_effectFinder.beginFrozen(GetTickCount64());
    }
    else if (ImGui::Button("Restart guided captures")) g_effectFinder.begin(false, GetTickCount64(), true);
    nextUiAction("Stop finder (Num 5)");
    if (ImGui::Button("Stop finder (Num 5)"))
    {
        closeEffectFinder();publishEffectFilters();return;
    }
    if (addonTreeNode("Manual controls and details"))
    {
        if (view.excludedCompute)
            ImGui::TextWrapped("%u compute candidates excluded from browsing.",static_cast<unsigned>(view.excludedCompute));
        if(view.focused && view.followOriginal && view.stage==finder::Stage::Results)
            ImGui::TextWrapped("Graphics calls followed from source toggle: %llu",static_cast<unsigned long long>(view.followedGraphics));
        bool includeCompute=view.includeCompute;
        if (ImGui::Checkbox("Include compute candidates (advanced)", &includeCompute))
            g_effectFinder.setComputeCandidates(includeCompute);
        ImGui::TextWrapped("Skipping compute can remove data needed by later rendering and may crash the game. Off at the start of each finder session. Compute candidates are tested individually. Changing this option restores the original view.");
        view=g_effectFinder.view(GetTickCount64());
        if (!view.frozen)
        {
            if (ImGui::Button("Capture idle only")) g_effectFinder.begin(false, GetTickCount64());
            ImGui::SameLine();
            ImGui::BeginDisabled(!view.baselineComplete || !view.baselineCalls);
            if (ImGui::Button("Capture effect only")) g_effectFinder.begin(true, GetTickCount64());
            ImGui::EndDisabled();
        }
        view = g_effectFinder.view(GetTickCount64());
        if (view.stage == finder::Stage::Results && !view.candidates.empty())
        {
            if (view.quick)
            {
                if (ImGui::Button("Test one at a time")) g_effectFinder.select(view.selected, true);
            }
            else if (ImGui::Button("Use quick batch testing")) g_effectFinder.quickTest();
            ImGui::BeginDisabled(!g_effectFinder.confirmed());
            const bool keepMore = ImGui::Button("Keep and test more");
            ImGui::EndDisabled();
            if (keepMore) keepFinderFilter(false);
            const auto& candidate = view.candidates[view.selected];
            ImGui::Text("Selected candidate %u: VS %08X  PS %08X  CS %08X", static_cast<unsigned>(view.selected + 1),
                candidate.signature[2], candidate.signature[3], candidate.signature[4]);
            if(candidate.signature[10]) ImGui::Text("Remembered %s: %08X (saved with filter)",
                candidate.signature[10]==1?"pixel shader":candidate.signature[10]==2?"vertex shader":"compute shader",candidate.signature[11]);
            if (view.frozen) ImGui::Text("Calls in paused scan: %llu", static_cast<unsigned long long>(candidate.action));
            else ImGui::Text("Calls idle / action: %llu / %llu", static_cast<unsigned long long>(candidate.baseline), static_cast<unsigned long long>(candidate.action));
        }
        addonHelpText("Quick tests hide up to eight graphics candidates and narrow the batch from your answers. Compute is tested individually. Only a matched individual candidate can be saved. Undo an answer or test individually if a batch gives no result.");
        ImGui::TreePop();
    }
    if(addonTreeNode("How to use the finder"))
    {
        addonHelpText(view.frozen ? "Pause with the effect visible. Close ReShade during the scan. Num 1 / 2 browse candidates; Num 0 compares the original and preview. Selected HUD groups stay enabled." :
            "Close ReShade and follow the on-screen prompts. Idle and action captures run automatically. Use the number pad or the buttons to test the results. Existing toggles are paused while finding.");
        addonHelpText("Keep a filter only when the unwanted effect disappears cleanly. Use Keep and test more under Manual controls and details if the effect needs several filters.");
        if(view.frozen && g_finderFocusGroup>=0 && g_finderFocusGroup!=group.getId())
            addonHelpText("Assign a hotkey to the refined group and keep the original toggle off.");
        addonHelpText(view.frozen ? "Test saved filters after unpausing; draw dimensions can change during animation." :
            "Saved filters use this group's hotkey and activation settings when you finish.");
        ImGui::TreePop();
    }
    ImGui::Separator();
}
static void displayEffectFilters(ToggleGroup& group)
{
    const auto& rules = group.getEffectFilters();
    if (rules.empty() || !addonTreeNode("Saved effect filters")) return;
    addonHelpText("These filters follow this group's hotkey and activation settings.");
    size_t remove = rules.size();
    for (size_t i = 0; i < rules.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::SmallButton("Remove")) remove = i;
        ImGui::SameLine();
        ImGui::Text("%u: %s %08X / %08X", static_cast<unsigned>(i + 1), finder::compute(rules[i]) ? "Compute" : "Graphics",
            finder::compute(rules[i]) ? rules[i][4] : rules[i][2], rules[i][3]);
        if(rules[i][10]) { ImGui::SameLine();ImGui::Text("(source %s %08X)",rules[i][10]==1?"PS":rules[i][10]==2?"VS":"CS",rules[i][11]); }
        ImGui::PopID();
    }
    if (remove < rules.size())
    {
        group.removeEffectFilter(remove);
        saveShaderTogglerIniFile();
    }
    ImGui::TreePop();
}
static void displaySmartDisable(effect_runtime* runtime)
{
    ImGui::Separator();
    addonHeading("Smart disable");
    auto* device = runtime->get_device();
    const bool modern = smart::modern::supported(device);
    const bool dx12 = smart::dx12::supported(device);
    const bool other = smart::other::supported(device);
    if (!other && !dx12 && !modern && !smart::native(device))
    {
        ImGui::TextWrapped("Smart colour replacement supports DirectX 9, 10, 11, 12, Vulkan and OpenGL. Use the usual shader hunting controls with this renderer.");
        return;
    }
    auto& settings = smartSettingsFor(device);
    const uint32_t hash = g_pixelShaderManager.getActiveHuntedShaderHash();
    if (g_activeCollectorFrameCounter > 0 || !hash)
    {
        ImGui::TextWrapped("Wait for collection to finish, then choose a pixel shader with Numpad 1 / 2.");
        return;
    }
    ImGui::Text("Selected pixel shader: %08X", hash);
    const bool known = smart::knownBloom(!other && !dx12 && !modern && g_syndicateProfile, hash);
    const auto saved = settings.saved();
    if (const auto found = saved.find(hash); found != saved.end())
        ImGui::Text("Saved method: %s", smart::name(found->second.method));
    else if (known)
        ImGui::TextWrapped("Known Syndicate bloom fix: transparent black is used automatically when this shader is disabled.");
    else
        ImGui::TextWrapped("Preview a suggested method, then keep it if the effect disappears cleanly.");
    auto preview = settings.preview();
    if (preview.hash && preview.hash != hash)
    {
        settings.cancel();
        preview = {};
    }
    if (!preview.hash)
    {
        if (ImGui::Button("Smart disable##Start"))
        {
            const auto choice = settings.choice(hash, 0, !other && !dx12 && !modern && g_syndicateProfile)
                .value_or(smart::preset((other ? smart::other::suggestion(device, hash) : dx12 ? smart::dx12::suggestion(device, hash) : modern ? smart::modern::suggestion(device, hash) : smart::suggestion(device, hash))));
            g_smartTrialOrder = smart::candidates(choice.method);
            g_smartTrialIndex = 0;
            settings.preview({hash, choice, false});
            std::copy(choice.rgba.begin(), choice.rgba.end(), g_smartCustomColour);
            diagnostics::Write("[SMART] Preview started shader=%08X method=%s known_profile=%u", hash, smart::name(choice.method), known);
        }
        ImGui::SameLine();
        showHelpMarker("The suggestion uses a known game fix or a sample of the pass's blending state. You confirm the result because a game's shader purpose cannot always be inferred automatically.");
        return;
    }
    ImGui::Text("Preview: %s", smart::name(preview.choice.method));
    if (ImGui::Checkbox("Show original for comparison", &preview.original))
        settings.preview(preview);
    const auto status = other ? smart::other::status(device, hash) : dx12 ? smart::dx12::status(device, hash) : modern ? smart::modern::status(device, hash) : smart::status(device, hash);
    const bool confirmedRunning = preview.choice.method == smart::Method::Skip ||
        (status.attempted && status.choice == preview.choice && status.applied);
    if (preview.original)
        ImGui::TextWrapped("Showing this pixel shader's original pass. Other hidden shaders still apply.");
    else if (preview.choice.method == smart::Method::Skip)
        ImGui::TextWrapped("Using the usual draw skipping method.");
    else if (status.attempted && status.choice == preview.choice)
        ImGui::TextWrapped("%s", status.reason);
    else
        ImGui::TextWrapped("Waiting for the selected shader to draw with this method.");
    ImGui::BeginDisabled(preview.original || !confirmedRunning);
    if (ImGui::Button("Looks good"))
    {
        settings.set(hash, preview.choice);
        if (!g_pixelShaderManager.isHuntedShaderMarked()) g_pixelShaderManager.toggleMarkOnHuntedShader();
        settings.cancel();
        saveShaderTogglerIniFile();
        diagnostics::Write("[SMART] Accepted shader=%08X method=%s", hash, smart::name(preview.choice.method));
    }
    ImGui::EndDisabled();
    nextUiAction("Try another method");
    if (ImGui::Button("Try another method"))
    {
        g_smartTrialIndex = (g_smartTrialIndex + 1) % g_smartTrialOrder.size();
        preview.choice = smart::preset(g_smartTrialOrder[g_smartTrialIndex]);
        preview.original = false;
        settings.preview(preview);
        std::copy(preview.choice.rgba.begin(), preview.choice.rgba.end(), g_smartCustomColour);
        diagnostics::Write("[SMART] Trying shader=%08X method=%s", hash, smart::name(preview.choice.method));
    }
    nextUiAction("Cancel preview");
    if (ImGui::Button("Cancel preview"))
    {
        settings.cancel();
        diagnostics::Write("[SMART] Preview cancelled shader=%08X; previous method retained.", hash);
    }
    if(addonTreeNode("How to use Smart disable"))
    {
        addonHelpText("Looks good adds this shader to the group and saves your method. Done hunting finishes the session. Other hidden shaders can still affect the preview.");
        ImGui::TreePop();
    }
    if (addonTreeNode("Advanced colour"))
    {
        ImGui::ColorEdit4("RGBA", g_smartCustomColour, ImGuiColorEditFlags_Float);
        if (ImGui::Button("Preview custom colour"))
        {
            smart::Choice custom{smart::Method::Custom};
            std::copy(std::begin(g_smartCustomColour), std::end(g_smartCustomColour), custom.rgba.begin());
            if (smart::valid(custom)) settings.preview({hash, custom, false});
        }
        ImGui::TreePop();
    }
}
static void displayAddonHelp()
{
    if(addonSection("Choosing a tool",ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped("Hunt: find shaders. Refine: separate shared effects. Merge: combine working groups.");
        addonHelpText("Create a group, assign a hotkey, then use Hunt to select its shaders. Use standalone Finder under Advanced tools when you have no source toggle.");
    }
    if(addonSection("Hunting hotkeys",ImGuiTreeNodeFlags_DefaultOpen))
    {
        if(ImGui::BeginTable("Hunting shortcuts",3,ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn("Shader");ImGui::TableSetupColumn("Previous / next");ImGui::TableSetupColumn("Mark / unmark");ImGui::TableHeadersRow();
            const char* rows[][3]={{"Pixel","Num 1 / 2","Num 3"},{"Vertex","Num 4 / 5","Num 6"},{"Compute","Num 7 / 8","Num 9"}};
            for(const auto& row:rows)
            {
                ImGui::TableNextRow();
                for(int col=0;col<3;++col) { ImGui::TableSetColumnIndex(col);ImGui::TextWrapped("%s",row[col]); }
            }
            ImGui::EndTable();
        }
        addonHelpText("Hold Ctrl while browsing to visit marked shaders only. Hold a browsing key to scroll faster.");
    }
    if(addonSection("Diagnostics"))
    {
        ImGui::TextWrapped("Log: ShaderTogglerAdvanced.log");
        addonHelpText("Located beside the game executable. Attach it when reporting a problem.");
        if (const DWORD logError = diagnostics::error.load(std::memory_order_relaxed))
            ImGui::Text("Log unavailable: Windows error %lu", static_cast<unsigned long>(logError));
        else if (diagnostics::capped.load(std::memory_order_relaxed))
            ImGui::TextUnformatted("Log size limit reached for this run.");
    }
}
static void displayAddonSettings()
{
 if (addonSection("Hunting and controls", ImGuiTreeNodeFlags_DefaultOpen))
 {
  ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
        bool originalMatching = g_originalShaderMatching.load();
        if (ImGui::Checkbox("Use original shader matching", &originalMatching))
        {
            g_originalShaderMatching = originalMatching;
            if(g_finderRuntime && g_effectFinder.view(GetTickCount64()).frozen) refreshFinderScene();
            saveShaderTogglerIniFile();
        }
        ImGui::SameLine();
        showHelpMarker("Enabled by default to retain ordinary Hunt Shaders and toggle behavior from the original add-on, including last-known shader matching across passes. Disable for strict stage matching and direct compute suppression. Effect Finder keeps its separate precise matching rules.");
  ImGui::SliderFloat("Overlay opacity", &g_overlayOpacity, 0.0f, 1.0f);
  ImGui::SameLine();
  showHelpMarker("Set this to 0.0 to make the hunting overlay fully invisible.");
  ImGui::SliderInt("Frames to collect", &g_startValueFramecountCollectionPhase, 10, 1000);
  ImGui::SameLine();
  showHelpMarker("Increase this if the shader you want only appears occasionally.");
  int controllerMode = static_cast<int>(KeyData::getControllerLabelMode());
  const char* controllerModeItems[] = { "Auto", "Xbox", "PlayStation" };
  if (ImGui::Combo("Controller labels", &controllerMode, controllerModeItems, IM_ARRAYSIZE(controllerModeItems)))
  {
   KeyData::setControllerLabelMode(static_cast<KeyData::ControllerLabelMode>(controllerMode));
   saveShaderTogglerIniFile();
  }
  ImGui::SameLine();
  showHelpMarker("Changes how gamepad buttons are shown in hotkey text. Auto tries to detect PlayStation controllers.");
  if (KeyData::getControllerLabelMode() == KeyData::ControllerLabelMode::Auto)
  {
   KeyData::refreshControllerTypeDetection();
   if (KeyData::isPlayStationControllerDetected())
   {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.90f, 0.45f, 1.0f));
    ImGui::TextUnformatted("Detected controller labels: PlayStation");
    ImGui::PopStyleColor();
   }
   else
   {
    ImGui::TextUnformatted("Detected controller labels: Xbox");
   }
  }
  else if (KeyData::getControllerLabelMode() == KeyData::ControllerLabelMode::PlayStation)
  {
   ImGui::TextUnformatted("Controller labels are forced to PlayStation");
  }
  else
  {
   ImGui::TextUnformatted("Controller labels are forced to Xbox");
  }
  int globalHotkeyModifier = KeyData::globalHotkeyModifierToInt(KeyData::getGlobalHotkeyModifier());
  const char* globalModifierItems[] = {
   "None",
   "Ctrl",
   "Alt",
   "Shift",
   "Ctrl + Alt",
   "Ctrl + Shift",
   "Alt + Shift",
   "Ctrl + Alt + Shift"
  };
  if (ImGui::Combo("Global hotkey modifier", &globalHotkeyModifier, globalModifierItems, IM_ARRAYSIZE(globalModifierItems)))
  {
   KeyData::setGlobalHotkeyModifier(KeyData::globalHotkeyModifierFromInt(globalHotkeyModifier));
   saveShaderTogglerIniFile();
  }
  ImGui::SameLine();
  showHelpMarker("Applies the selected modifier(s) to all defined keyboard and mouse hotkeys at runtime without changing the stored bindings.");
  ImGui::PopItemWidth();
 }
 ImGui::Separator();
 if (addonSection("Menu suspension"))
 {
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.78f, 0.25f, 1.00f));
  ImGui::TextWrapped("Configure while the menu is closed and gameplay is visible.");
  ImGui::PopStyleColor();
  if (ImGui::Button("Set Current State as Gameplay"))
  {
   if (g_allToggleGroupsSuspended)
    restoreAllToggleGroups();
   g_pendingSuspendedGroupToggles.clear();
   g_globalSuspensionStarted = {};
  }
  ImGui::SameLine();
  showHelpMarker("Restores normal operation and clears any queued toggles. Use this while gameplay is visible.");
  ImGui::TextUnformatted("Status:");
  ImGui::SameLine();
  if (g_allToggleGroupsSuspended)
  {
   ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.65f, 0.20f, 1.00f));
   ImGui::TextUnformatted("Suspended");
   ImGui::PopStyleColor();
  }
  else
  {
   ImGui::TextUnformatted("Gameplay");
  }
  if (g_allToggleGroupsSuspended)
  {
   if (ImGui::Button("Restore Toggle Groups"))
    restoreAllToggleGroups();
   if (!g_pendingSuspendedGroupToggles.empty())
   {
    ImGui::SameLine();
    ImGui::Text("(%d queued toggle%s)",
     static_cast<int>(g_pendingSuspendedGroupToggles.size()),
     g_pendingSuspendedGroupToggles.size() == 1 ? "" : "s");
   }
  }
  auto drawGlobalHotkeyList =
   [&](const char* label,
    std::vector<KeyData>& hotkeys,
    std::atomic_int& editingSlot,
    bool suspendList)
   {
    ImGui::Separator();
    ImGui::TextUnformatted(label);
    for (size_t hotkeyIndex = 0; hotkeyIndex < hotkeys.size(); ++hotkeyIndex)
    {
     ImGui::PushID(suspendList ? static_cast<int>(hotkeyIndex) :
      10000 + static_cast<int>(hotkeyIndex));
     const bool editingThisHotkey =
      editingSlot == static_cast<int>(hotkeyIndex);
     const std::string hotkeyText =
      editingThisHotkey ? g_keyCollector.getKeyAsString() :
      hotkeys[hotkeyIndex].getKeyAsString();
     char hotkeyBuffer[128] = {};
     strncpy_s(hotkeyBuffer, sizeof(hotkeyBuffer), hotkeyText.c_str(), _TRUNCATE);
     ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.45f);
     ImGui::InputText("##GlobalHotkey", hotkeyBuffer, sizeof(hotkeyBuffer), ImGuiInputTextFlags_ReadOnly);
     if (ImGui::IsItemClicked())
      startGlobalHotkeyEditing(suspendList, static_cast<int>(hotkeyIndex));
     ImGui::PopItemWidth();
     if (editingThisHotkey)
     {
      ImGui::SameLine();
      if (ImGui::Button("OK"))
       endGlobalHotkeyEditing(suspendList, true);
      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
       endGlobalHotkeyEditing(suspendList, false);
     }
     else
     {
      ImGui::SameLine();
      if (ImGui::Button("Remove"))
      {
       hotkeys.erase(
        hotkeys.begin() + static_cast<std::ptrdiff_t>(hotkeyIndex));
       saveShaderTogglerIniFile();
       ImGui::PopID();
       break;
      }
     }
     ImGui::PopID();
    }
    const int addHotkeyIndex = static_cast<int>(hotkeys.size());
    const bool editingNewHotkey = editingSlot == addHotkeyIndex;
    const std::string addHotkeyText =
     editingNewHotkey ? g_keyCollector.getKeyAsString() : "Add hotkey";
    char addHotkeyBuffer[128] = {};
    strncpy_s(addHotkeyBuffer, sizeof(addHotkeyBuffer), addHotkeyText.c_str(), _TRUNCATE);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.45f);
    ImGui::InputText("##AddGlobalHotkey", addHotkeyBuffer, sizeof(addHotkeyBuffer), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::IsItemClicked())
     startGlobalHotkeyEditing(suspendList, addHotkeyIndex);
    ImGui::PopItemWidth();
    if (editingNewHotkey)
    {
     ImGui::SameLine();
     if (ImGui::Button("OK##AddGlobalHotkey"))
      endGlobalHotkeyEditing(suspendList, true);
     ImGui::SameLine();
     if (ImGui::Button("Cancel##AddGlobalHotkey"))
      endGlobalHotkeyEditing(suspendList, false);
    }
   };
  drawGlobalHotkeyList(
   "Suspend Hotkeys (menu open)",
   g_globalSuspendHotkeys,
   g_globalSuspendHotkeySlotEditing,
   true);
  ImGui::SameLine();
  showHelpMarker("Only checked during gameplay.");
  drawGlobalHotkeyList(
   "Restore Hotkeys (menu close/back)",
   g_globalRestoreHotkeys,
   g_globalRestoreHotkeySlotEditing,
   false);
  ImGui::SameLine();
  showHelpMarker("Only checked while groups are suspended. Back/B therefore cannot affect normal gameplay.");
  if (!g_globalSuspendHotkeys.empty() || !g_globalRestoreHotkeys.empty())
  {
   ImGui::Separator();
   if (ImGui::Button("Clear all suspension hotkeys"))
   {
    g_globalSuspendHotkeys.clear();
    g_globalRestoreHotkeys.clear();
    g_globalSuspendHotkeySlotEditing = -1;
    g_globalRestoreHotkeySlotEditing = -1;
    g_keyCollector.clear();
    if (g_allToggleGroupsSuspended)
     restoreAllToggleGroups();
    saveShaderTogglerIniFile();
   }
  }
 }
 ImGui::Separator();
}
static void displayGroupSort()
{
    if(ImGui::BeginPopup("Sort groups"))
    {
        if(ImGui::MenuItem("Hotkey layout")) sortToggleGroupsByHotkey();
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("End, Numpad / * - +, Backspace/Page keys, arrows, remaining Numpad keys, Insert/Delete.");
        if(ImGui::MenuItem("Name A-Z")) sortToggleGroupsByNameAZ();
        if(ImGui::MenuItem("Name length")) sortToggleGroupsByNameLength();
        ImGui::EndPopup();
    }
}
static bool reorderToggleGroup(int sourceId,int targetId,bool after)
{
    if(sourceId==targetId) return false;
    const auto find=[&](int id){return std::find_if(g_toggleGroups.begin(),g_toggleGroups.end(),[&](const auto& g){return g.getId()==id;});};
    const auto source=find(sourceId),target=find(targetId);
    if(source==g_toggleGroups.end() || target==g_toggleGroups.end()) return false;
    const auto insertion=target+(after?1:0);
    if(insertion==source || insertion==source+1) return false;
    if(source<insertion) std::rotate(source,source+1,insertion);
    else std::rotate(insertion,source,source+1);
    diagnostics::Write("[GROUPS] Reordered group=%d %s group=%d",sourceId,after?"after":"before",targetId);
    saveShaderTogglerIniFile();
    return true;
}
static void displayGroupDragSource(const ToggleGroup& group)
{
    if(ImGui::BeginDragDropSource())
    {
        const int id=group.getId();
        ImGui::SetDragDropPayload("STA_GROUP_ID",&id,sizeof(id));
        ImGui::Text("Move: %s",group.getName().c_str());
        ImGui::EndDragDropSource();
    }
}
static void displayGroupDragHandle(const ToggleGroup& group,float width)
{
    const auto start=ImGui::GetCursorScreenPos();
    const float height=ImGui::GetFrameHeight(),unit=ImGui::GetFontSize();
    ImGui::InvisibleButton("##MoveGroup",ImVec2(width,height));
    const bool hovered=ImGui::IsItemHovered(),active=ImGui::IsItemActive();
    auto* draw=ImGui::GetWindowDrawList();
    const ImU32 color=ImGui::GetColorU32(hovered || active?ImGuiCol_CheckMark:ImGuiCol_TextDisabled);
    for(int row=0;row<3;++row) for(int col=0;col<2;++col)
        draw->AddCircleFilled(ImVec2(start.x+width*0.5f+(col-0.5f)*unit*0.22f,start.y+height*0.5f+(row-1)*unit*0.22f),std::max(1.0f,unit*0.055f),color);
    if(hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    if(hovered && !ImGui::GetDragDropPayload()) ImGui::SetTooltip("Drag to reorder. You can also drag the group name.");
    displayGroupDragSource(group);
}
static void displayGroupState(ToggleGroup& group,float width,effect_runtime* runtime)
{
    const bool suspended=g_allToggleGroupsSuspended;
    const bool active=group.isActive();
    const bool disabled=suspended || g_finderRuntime || !runtime;
    const char* label=suspended?"SUS###GroupState":active?"ON###GroupState":"OFF###GroupState";
    const ImVec4 background=suspended?ImVec4(1.00f,0.73f,0.27f,1):active?ImVec4(0.35f,0.96f,0.65f,1):ImVec4(0.18f,0.23f,0.31f,1);
    ImGui::PushStyleColor(ImGuiCol_Button,background);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,active?ImVec4(0.52f,1.00f,0.76f,1):ImVec4(0.28f,0.42f,0.60f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,active?ImVec4(0.22f,0.76f,0.49f,1):ImVec4(0.15f,0.34f,0.55f,1));
    ImGui::PushStyleColor(ImGuiCol_Text,suspended || active?ImVec4(0.025f,0.065f,0.075f,1):ImVec4(0.96f,0.98f,1.00f,1));
    ImGui::PushStyleColor(ImGuiCol_Border,active?ImVec4(0.57f,1.00f,0.80f,1):ImVec4(0.48f,0.65f,0.84f,1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.0f);
    ImGui::BeginDisabled(disabled);
    if(ImGui::Button(label,ImVec2(width,ImGui::GetFrameHeight()))) toggleGroupFromMenu(group,runtime);
    ImGui::EndDisabled();
    ImGui::PopStyleVar();ImGui::PopStyleColor(5);
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        if(!disabled) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if(suspended) ImGui::SetTooltip("All groups are suspended. Click Restore before changing a group.");
        else if(g_finderRuntime) ImGui::SetTooltip("Finder controls the preview. Use HUD visibility and shader selection, or stop Finder to toggle groups.");
        else
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(group.isActive()?"ON: this group's effects are disabled. Click to turn OFF.":"OFF: this group's effects are allowed. Click to turn ON.");
            if(group.isHoldMode() || group.isTimedMode()) ImGui::TextUnformatted("Your menu choice stays until the next input from this group's assigned controls.");
            ImGui::EndTooltip();
        }
    }
}
static void displayGroupDropTarget(int targetId,int& sourceId,int& destinationId,bool& after)
{
    const auto start=ImGui::GetItemRectMin(),end=ImGui::GetItemRectMax();
    if(ImGui::BeginDragDropTarget())
    {
        if(const ImGuiPayload* payload=ImGui::AcceptDragDropPayload("STA_GROUP_ID",ImGuiDragDropFlags_AcceptBeforeDelivery|ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            int dragged=-1;
            if(payload->Data && payload->DataSize==sizeof(dragged))
            {
                std::memcpy(&dragged,payload->Data,sizeof(dragged));
                if(dragged!=targetId)
                {
                    const bool below=ImGui::GetIO().MousePos.y>(start.y+end.y)*0.5f;
                    const float y=below?end.y:start.y;
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(start.x,y),ImVec2(end.x,y),ImGui::GetColorU32(ImGuiCol_CheckMark),std::max(2.0f,ImGui::GetFontSize()*0.075f));
                    if(payload->IsDelivery()) { sourceId=dragged;destinationId=targetId;after=below; }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}
static void displayToggleGroups(effect_runtime* runtime)
{
        if(ImGui::Button("New"))
        {
            addDefaultGroup();g_toggleGroups.back().setEditing(true);g_groupSearch[0]=0;g_groupListFilter=0;
        }
        nextUiAction("Merge");
        ImGui::BeginDisabled(g_toggleGroups.size()<2);
        if(ImGui::Button("Merge")) { g_mergePanelOpen=!g_mergePanelOpen;g_mergeMessage.clear(); }
        ImGui::EndDisabled();
        nextUiAction("Sort");
        if(ImGui::Button("Sort")) ImGui::OpenPopup("Sort groups");
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("Sort all groups. For manual ordering, drag a row handle or group name.");
        displayGroupSort();
        const float gap=ImGui::GetStyle().ItemSpacing.x;
        const float filterWidth=ImGui::CalcTextSize("Filter*").x+ImGui::GetStyle().FramePadding.x*2;
        const float remaining=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x-ImGui::GetItemRectMax().x-gap;
        if(remaining>=ImGui::GetFontSize()*8+filterWidth+gap) ImGui::SameLine();
        ImGui::SetNextItemWidth(std::max(ImGui::GetFontSize()*5,ImGui::GetContentRegionAvail().x-filterWidth-gap));
        ImGui::InputTextWithHint("##GroupSearch","Search groups",g_groupSearch,sizeof(g_groupSearch));
        ImGui::SameLine();
        if(ImGui::Button(g_groupListFilter?"Filter*":"Filter")) ImGui::OpenPopup("Group filters");
        if(ImGui::BeginPopup("Group filters"))
        {
            if(ImGui::MenuItem("All groups",nullptr,g_groupListFilter==0)) g_groupListFilter=0;
            if(ImGui::MenuItem("Refined groups",nullptr,g_groupListFilter==1)) g_groupListFilter=1;
            if(ImGui::MenuItem("Active groups",nullptr,g_groupListFilter==2)) g_groupListFilter=2;
            ImGui::Separator();
            if(ImGui::MenuItem("Clear search and filter")) { g_groupSearch[0]=0;g_groupListFilter=0; }
            ImGui::EndPopup();
        }
        displayGroupMergeControls();
        ImGui::Separator();
  std::vector<int> idsToRemove;
        std::vector<ToggleGroup> groupsToDuplicate;
        int groupToRefine=-1,groupToMove=-1,groupMoveTarget=-1;
        bool groupMoveAfter=false;
        const std::string query=toLowerCopy(g_groupSearch);
        size_t shown=0;
        for (auto& group : g_toggleGroups)
        {
            const bool pinned=group.isEditing() || group.getId()==g_toggleGroupIdShaderEditing || group.getId()==g_finderGroup;
            const bool matchesText=query.empty() || toLowerCopy(group.getName()).find(query)!=std::string::npos ||
                toLowerCopy(group.getNotice()).find(query)!=std::string::npos || toLowerCopy(group.getToggleKeyAsString()).find(query)!=std::string::npos;
            const bool matchesType=g_groupListFilter==0 || (g_groupListFilter==1 && !group.getEffectFilters().empty()) || (g_groupListFilter==2 && group.isActive());
            if(!pinned && (!matchesText || !matchesType)) continue;
            ++shown;
            ImGui::PushID(group.getId());
            const size_t shaders=group.getPixelShaderHashes().size()+group.getVertexShaderHashes().size()+group.getComputeShaderHashes().size();
            const size_t filters=group.getEffectFilters().size();
            const bool huntingThis=group.getId()==g_toggleGroupIdShaderEditing;
            const bool canHunt=!g_finderRuntime && (g_toggleGroupIdShaderEditing<0 || huntingThis);
            const bool canRefine=(shaders || filters) && !g_finderRuntime && g_toggleGroupIdShaderEditing<0;
            const float unit=ImGui::GetFontSize();
            const float available=ImGui::GetContentRegionAvail().x;
            const float rowLeft=ImGui::GetCursorPosX();
            const float gripWidth=unit*0.65f;
            const float gap=ImGui::GetStyle().ItemSpacing.x;
            const float padding=ImGui::GetStyle().FramePadding.x*2;
            const float stateWidth=std::max(ImGui::CalcTextSize("OFF").x,ImGui::CalcTextSize("SUS").x)+padding;
            const float left=rowLeft+gripWidth+gap;
            const float contentWidth=available-gripWidth-gap;
            const float menuWidth=ImGui::CalcTextSize("...").x+padding;
            const float fullActionWidth=menuWidth+ImGui::CalcTextSize("HuntRefineEdit").x+padding*3+gap*3;
            const float minNameWidth=unit*8;
            const bool showActions=contentWidth>=stateWidth+minNameWidth+fullActionWidth+gap*2;
            const float actionWidth=showActions?fullActionWidth:menuWidth;
            const bool showHotkey=contentWidth>=stateWidth+minNameWidth+actionWidth+unit*8+gap*3;
            const float keyWidth=showHotkey?unit*8:0;
            const bool showCounts=showHotkey && contentWidth>=stateWidth+minNameWidth+actionWidth+keyWidth+unit*4.5f+gap*4;
            const float countsWidth=showCounts?unit*4.5f:0;
            const float nameWidth=std::max(unit*3,contentWidth-stateWidth-keyWidth-countsWidth-actionWidth-gap*(2+showHotkey+showCounts));
            ImGui::BeginGroup();
            displayGroupDragHandle(group,gripWidth);
            ImGui::SameLine();ImGui::SetCursorPosX(left);
            displayGroupState(group,stateWidth,runtime);
            ImGui::SameLine();ImGui::SetCursorPosX(left+stateWidth+gap);
            const std::string name=group.getName()+(group.getNotice().empty()?"":" [!]");
            const std::string nameLabel=compactUiLabel(name,std::max(unit,nameWidth-unit*0.3f))+"###GroupName";
            const bool edit=ImGui::Selectable(nameLabel.c_str(),group.isEditing(),0,ImVec2(nameWidth,ImGui::GetFrameHeight()));
            const bool nameHovered=ImGui::IsItemHovered();
            displayGroupDragSource(group);
            if(edit && !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) group.setEditing(true);
            if(nameHovered && !ImGui::GetDragDropPayload())
            {
                ImGui::BeginTooltip();ImGui::PushTextWrapPos(unit*38);
                ImGui::TextUnformatted(group.getName().c_str());
                ImGui::Text("Hotkey: %s",group.getToggleKey().getKeyCode()?group.getToggleKeyAsString().c_str():"None");
                ImGui::Text("%zu shaders / %zu refined filters",shaders,filters);
                if(group.isTimedMode()) ImGui::TextUnformatted(group.isTimedModeInverted()?"Auto-show":"Auto-hide");
                else if(group.isHoldMode()) ImGui::TextUnformatted(group.isHoldInverted()?"Inverted hold":"Hold");
                if(group.isActiveAtStartup()) ImGui::TextUnformatted("Active at startup");
                if(!group.getNotice().empty()) ImGui::TextUnformatted(group.getNotice().c_str());
                ImGui::TextDisabled("Click to edit, or drag to reorder. Use ... for group actions.");
                ImGui::PopTextWrapPos();ImGui::EndTooltip();
            }
            float column=left+stateWidth+gap+nameWidth+gap;
            if(showHotkey)
            {
                ImGui::SameLine();ImGui::SetCursorPosX(column);
                const auto pos=ImGui::GetCursorScreenPos();
                ImGui::PushClipRect(pos,ImVec2(pos.x+keyWidth,pos.y+ImGui::GetFrameHeight()),true);
                ImGui::TextDisabled("%s",group.getToggleKey().getKeyCode()?group.getToggleKeyAsString().c_str():"--");
                ImGui::PopClipRect();
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s",group.getToggleKeyAsString().c_str());
                column+=keyWidth+gap;
            }
            if(showCounts)
            {
                ImGui::SameLine();ImGui::SetCursorPosX(column);
                if(filters && shaders) ImGui::TextDisabled("%zuF+S",filters);
                else if(filters) ImGui::TextDisabled("%zuF",filters);
                else ImGui::TextDisabled("%zuS",shaders);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("%zu shaders / %zu refined filters",shaders,filters);
                column+=countsWidth+gap;
            }
            ImGui::SameLine();ImGui::SetCursorPosX(column);
            if(showActions)
            {
                ImGui::BeginDisabled(!canHunt);
                if(ImGui::Button(huntingThis?"Done":"Hunt"))
                {
                    if(huntingThis) { endShaderEditing(true,group);saveShaderTogglerIniFile(); }
                    else startShaderEditing(group);
                }
                ImGui::EndDisabled();ImGui::SameLine();
                ImGui::BeginDisabled(!canRefine);
                if(ImGui::Button("Refine")) groupToRefine=group.getId();
                ImGui::EndDisabled();
                if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip(shaders || filters?"Refine this toggle in a paused scene to separate shared effects.":"Hunt shaders and save a selection first.");
                ImGui::SameLine();if(ImGui::Button("Edit")) group.setEditing(true);
                ImGui::SameLine();
            }
            if(ImGui::Button("...")) ImGui::OpenPopup("Group actions");
            if(ImGui::BeginPopup("Group actions"))
            {
                ImGui::TextDisabled("Group %d",group.getId());
                if(ImGui::MenuItem("Edit")) group.setEditing(true);
                if(ImGui::MenuItem(huntingThis?"Done hunting":"Hunt Shaders",nullptr,false,canHunt))
                {
                    if(huntingThis) { endShaderEditing(true,group);saveShaderTogglerIniFile(); }
                    else startShaderEditing(group);
                }
                if(ImGui::MenuItem("Refine effect",nullptr,false,canRefine)) groupToRefine=group.getId();
                ImGui::Separator();
                if(ImGui::MenuItem("Duplicate"))
                {
                    auto duplicate=group.makeDuplicate();
                    if(huntingThis) duplicate.storeCollectedHashes(g_pixelShaderManager.getMarkedShaderHashes(),
                        g_vertexShaderManager.getMarkedShaderHashes(),g_computeShaderManager.getMarkedShaderHashes());
                    groupsToDuplicate.push_back(std::move(duplicate));
                }
                ImGui::Separator();
                if(ImGui::MenuItem("Delete group")) idsToRemove.push_back(group.getId());
                ImGui::EndPopup();
            }
            ImGui::EndGroup();
            displayGroupDropTarget(group.getId(),groupToMove,groupMoveTarget,groupMoveAfter);
   if (group.isEditing())
   {
    ImGui::Separator();
    ImGui::TextUnformatted("Group settings");
                if(g_groupTimedFadeOutStart.count(group.getId())) ImGui::TextDisabled("Linger timer running");
                if(group.hasTimedSuppressionKeys()) ImGui::TextDisabled("Timed suppression configured");
    char tmpBuffer[150] = {};
    strncpy_s(tmpBuffer, sizeof(tmpBuffer), group.getName().c_str(), _TRUNCATE);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
    ImGui::Text("Name");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##Name", tmpBuffer, 149);
    group.setName(tmpBuffer);
    ImGui::PopItemWidth();
    char noticeBuffer[512] = {};
    strncpy_s(noticeBuffer, sizeof(noticeBuffer), group.getNotice().c_str(), _TRUNCATE);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
    ImGui::Text("Notice");
                ImGui::SameLine();showHelpMarker("Shown when hovering [!] next to the group name.");
    ImGui::InputTextMultiline(
     "##Notice",
     noticeBuffer,
     sizeof(noticeBuffer),
     ImVec2(0.0f, ImGui::GetTextLineHeight() * 3.0f));
    group.setNotice(noticeBuffer);
    ImGui::PopItemWidth();
    bool isKeyEditing = false;
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    ImGui::Text("Key shortcut");
    std::string textBoxContents = (g_toggleGroupIdKeyBindingEditing == group.getId()) ? g_keyCollector.getKeyAsString() : group.getToggleKeyAsString();
    char keyBuf[128] = {};
    strncpy_s(keyBuf, sizeof(keyBuf), textBoxContents.c_str(), _TRUNCATE);
    ImGui::InputText("##Key shortcut", keyBuf, sizeof(keyBuf), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::IsItemClicked())
    {
     startKeyBindingEditing(group);
    }
    ImGui::SameLine();
    showHelpMarker("Mouse only: Release, move away, then click.");
    if (g_toggleGroupIdKeyBindingEditing == group.getId())
    {
     isKeyEditing = true;
     nextUiAction("OK");
     if (ImGui::Button("OK"))
     {
      endKeyBindingEditing(true, group);
     }
     nextUiAction("Cancel");
     if (ImGui::Button("Cancel"))
     {
      endKeyBindingEditing(false, group);
     }
    }
    ImGui::PopItemWidth();
                const bool timedKeyEditing=g_toggleGroupIdTimedTriggerKeyEditing==group.getId() || g_toggleGroupIdTimedSuppressionKeyEditing==group.getId();
                if(timedKeyEditing) { ImGui::SetNextItemOpen(true);isKeyEditing=true; }
                if(addonTreeNode("Timed triggers and suppression"))
                {
    ImGui::Text("Timed triggers");
    ImGui::SameLine();
    showHelpMarker("Each trigger can either activate on press, refresh while held, or do both. If none are set, timed mode falls back to the main hotkey.");
    for (size_t triggerIndex = 0; triggerIndex < group.getTimedTriggerKeyCount(); ++triggerIndex)
    {
     ImGui::PushID(static_cast<int>(triggerIndex));
     ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
     std::string timedTriggerTextBoxContents =
      (g_toggleGroupIdTimedTriggerKeyEditing == group.getId() &&
       g_toggleGroupTimedTriggerKeySlotEditing == static_cast<int>(triggerIndex))
       ? g_keyCollector.getKeyAsString()
       : group.getTimedTriggerKeyAsString(triggerIndex);
     char timedTriggerBuf[128] = {};
     strncpy_s(timedTriggerBuf, sizeof(timedTriggerBuf), timedTriggerTextBoxContents.c_str(), _TRUNCATE);
     ImGui::InputText("##TimedTrigger", timedTriggerBuf, sizeof(timedTriggerBuf), ImGuiInputTextFlags_ReadOnly);
     if (ImGui::IsItemClicked())
     {
      startTimedTriggerKeyBindingEditing(group, static_cast<int>(triggerIndex));
     }
     if (g_toggleGroupIdTimedTriggerKeyEditing == group.getId() &&
      g_toggleGroupTimedTriggerKeySlotEditing == static_cast<int>(triggerIndex))
     {
      isKeyEditing = true;
      nextUiAction("OK##TimedTrigger");
      if (ImGui::Button("OK##TimedTrigger"))
      {
       endTimedTriggerKeyBindingEditing(true, group);
      }
      nextUiAction("Cancel##TimedTrigger");
      if (ImGui::Button("Cancel##TimedTrigger"))
      {
       endTimedTriggerKeyBindingEditing(false, group);
      }
     }
     ImGui::SameLine();
     int triggerMode = ToggleGroup::timedTriggerModeToInt(group.getTimedTriggerModeAt(triggerIndex));
     const char* modeItems[] = { "On press", "While held", "Press + hold" };
     ImGui::SetNextItemWidth(140.0f);
     if (ImGui::Combo("##TimedTriggerMode", &triggerMode, modeItems, IM_ARRAYSIZE(modeItems)))
     {
      group.setTimedTriggerModeAt(triggerIndex, ToggleGroup::timedTriggerModeFromInt(triggerMode));
     }
     nextUiAction("Remove");
     if (ImGui::Button("Remove"))
     {
      group.removeTimedTriggerKeyAt(triggerIndex);
      if (g_toggleGroupIdTimedTriggerKeyEditing == group.getId())
      {
       if (g_toggleGroupTimedTriggerKeySlotEditing == static_cast<int>(triggerIndex))
       {
        g_toggleGroupIdTimedTriggerKeyEditing = -1;
        g_toggleGroupTimedTriggerKeySlotEditing = -1;
        g_keyCollector.clear();
       }
       else if (g_toggleGroupTimedTriggerKeySlotEditing > static_cast<int>(triggerIndex))
       {
        --g_toggleGroupTimedTriggerKeySlotEditing;
       }
      }
      ImGui::PopItemWidth();
      ImGui::PopID();
      break;
     }
     ImGui::PopItemWidth();
     ImGui::PopID();
    }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    std::string addTriggerText =
     (g_toggleGroupIdTimedTriggerKeyEditing == group.getId() &&
      g_toggleGroupTimedTriggerKeySlotEditing == static_cast<int>(group.getTimedTriggerKeyCount()))
      ? g_keyCollector.getKeyAsString()
      : std::string("Add timed trigger");
    char addTriggerBuf[128] = {};
    strncpy_s(addTriggerBuf, sizeof(addTriggerBuf), addTriggerText.c_str(), _TRUNCATE);
    ImGui::InputText("##AddTimedTrigger", addTriggerBuf, sizeof(addTriggerBuf), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::IsItemClicked())
    {
     startTimedTriggerKeyBindingEditing(group, static_cast<int>(group.getTimedTriggerKeyCount()));
    }
    if (g_toggleGroupIdTimedTriggerKeyEditing == group.getId() &&
     g_toggleGroupTimedTriggerKeySlotEditing == static_cast<int>(group.getTimedTriggerKeyCount()))
    {
     isKeyEditing = true;
     nextUiAction("OK##AddTimedTrigger");
     if (ImGui::Button("OK##AddTimedTrigger"))
     {
      endTimedTriggerKeyBindingEditing(true, group);
     }
     nextUiAction("Cancel##AddTimedTrigger");
     if (ImGui::Button("Cancel##AddTimedTrigger"))
     {
      endTimedTriggerKeyBindingEditing(false, group);
     }
    }
    ImGui::PopItemWidth();
    ImGui::Text("Timed suppression keys");
    ImGui::SameLine();
    showHelpMarker("These keys prevent timed mode from activating while held. Useful for combo inputs like RT + Y / RT + B where the base trigger should be ignored.");
    for (size_t suppressionIndex = 0; suppressionIndex < group.getTimedSuppressionKeyCount(); ++suppressionIndex)
    {
     ImGui::PushID(static_cast<int>(10000 + suppressionIndex));
     ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
     std::string suppressionTextBoxContents =
      (g_toggleGroupIdTimedSuppressionKeyEditing == group.getId() &&
       g_toggleGroupTimedSuppressionKeySlotEditing == static_cast<int>(suppressionIndex))
       ? g_keyCollector.getKeyAsString()
       : group.getTimedSuppressionKeyAsString(suppressionIndex);
     char suppressionBuf[128] = {};
     strncpy_s(suppressionBuf, sizeof(suppressionBuf), suppressionTextBoxContents.c_str(), _TRUNCATE);
     ImGui::InputText("##TimedSuppression", suppressionBuf, sizeof(suppressionBuf), ImGuiInputTextFlags_ReadOnly);
     if (ImGui::IsItemClicked())
     {
      startTimedSuppressionKeyBindingEditing(group, static_cast<int>(suppressionIndex));
     }
     if (g_toggleGroupIdTimedSuppressionKeyEditing == group.getId() &&
      g_toggleGroupTimedSuppressionKeySlotEditing == static_cast<int>(suppressionIndex))
     {
      isKeyEditing = true;
      nextUiAction("OK##TimedSuppression");
      if (ImGui::Button("OK##TimedSuppression"))
      {
       endTimedSuppressionKeyBindingEditing(true, group);
      }
      nextUiAction("Cancel##TimedSuppression");
      if (ImGui::Button("Cancel##TimedSuppression"))
      {
       endTimedSuppressionKeyBindingEditing(false, group);
      }
     }
     nextUiAction("Remove##TimedSuppression");
     if (ImGui::Button("Remove##TimedSuppression"))
     {
      group.removeTimedSuppressionKeyAt(suppressionIndex);
      if (g_toggleGroupIdTimedSuppressionKeyEditing == group.getId())
      {
       if (g_toggleGroupTimedSuppressionKeySlotEditing == static_cast<int>(suppressionIndex))
       {
        g_toggleGroupIdTimedSuppressionKeyEditing = -1;
        g_toggleGroupTimedSuppressionKeySlotEditing = -1;
        g_keyCollector.clear();
       }
       else if (g_toggleGroupTimedSuppressionKeySlotEditing > static_cast<int>(suppressionIndex))
       {
        --g_toggleGroupTimedSuppressionKeySlotEditing;
       }
      }
      ImGui::PopItemWidth();
      ImGui::PopID();
      break;
     }
     ImGui::PopItemWidth();
     ImGui::PopID();
    }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    std::string addSuppressionText =
     (g_toggleGroupIdTimedSuppressionKeyEditing == group.getId() &&
      g_toggleGroupTimedSuppressionKeySlotEditing == static_cast<int>(group.getTimedSuppressionKeyCount()))
      ? g_keyCollector.getKeyAsString()
      : std::string("Add suppression key");
    char addSuppressionBuf[128] = {};
    strncpy_s(addSuppressionBuf, sizeof(addSuppressionBuf), addSuppressionText.c_str(), _TRUNCATE);
    ImGui::InputText("##AddTimedSuppression", addSuppressionBuf, sizeof(addSuppressionBuf), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::IsItemClicked())
    {
     startTimedSuppressionKeyBindingEditing(group, static_cast<int>(group.getTimedSuppressionKeyCount()));
    }
    if (g_toggleGroupIdTimedSuppressionKeyEditing == group.getId() &&
     g_toggleGroupTimedSuppressionKeySlotEditing == static_cast<int>(group.getTimedSuppressionKeyCount()))
    {
     isKeyEditing = true;
     nextUiAction("OK##AddTimedSuppression");
     if (ImGui::Button("OK##AddTimedSuppression"))
     {
      endTimedSuppressionKeyBindingEditing(true, group);
     }
     nextUiAction("Cancel##AddTimedSuppression");
     if (ImGui::Button("Cancel##AddTimedSuppression"))
     {
      endTimedSuppressionKeyBindingEditing(false, group);
     }
    }
    ImGui::PopItemWidth();
    int suppressionLingerMs = group.getTimedSuppressionLingerMs();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
    ImGui::Text("Suppression linger");
    if (ImGui::SliderInt("##TimedSuppressionLingerMs", &suppressionLingerMs, 0, 1000, "%d ms"))
    {
     group.setTimedSuppressionLingerMs(suppressionLingerMs);
    }
    ImGui::SameLine();
    showHelpMarker("Keeps suppression active for a short time after the suppression button is released. This helps with controller triggers that release slightly later than face buttons.");
    ImGui::PopItemWidth();
                    ImGui::TreePop();
                }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
    bool isDefaultActive = group.isActiveAtStartup();
    if (ImGui::Checkbox("Is active at startup", &isDefaultActive))
     group.setIsActiveAtStartup(isDefaultActive);
    ImGui::SameLine();
    showHelpMarker("Activates this group automatically when the game starts.");
    ImGui::PopItemWidth();
    if (group.isActiveAtStartup())
    {
     ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
     bool startupTimed = group.isStartupTimed();
     if (ImGui::Checkbox("Deactivate automatically", &startupTimed))
      group.setStartupTimed(startupTimed);
     ImGui::SameLine();
     showHelpMarker("When enabled, the startup activation ends automatically after the configured duration.");
     ImGui::PopItemWidth();
     if (group.isStartupTimed())
     {
      ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.25f);
      ImGui::Text("Startup duration");
      float startupDurationSeconds = group.getStartupDurationMs() / 1000.0f;
      if (ImGui::InputFloat("##Startup duration", &startupDurationSeconds, 1.0f, 5.0f, "%.1f"))
      {
       if (startupDurationSeconds < 0.1f)
        startupDurationSeconds = 0.1f;
       if (startupDurationSeconds > 3600.0f)
        startupDurationSeconds = 3600.0f;
       group.setStartupDurationMs(static_cast<int>(startupDurationSeconds * 1000.0f));
      }
      ImGui::SameLine();
      ImGui::TextUnformatted("seconds");
      ImGui::SameLine();
      showHelpMarker("The group starts active and turns off once this amount of time has passed. The timer runs only once per game launch.");
      ImGui::PopItemWidth();
     }
    }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
    bool holdMode = group.isHoldMode();
    if (ImGui::Checkbox("Only active while holding key", &holdMode))
    {
     group.setHoldMode(holdMode);
    }
    ImGui::SameLine();
    showHelpMarker("Useful for ADS / HUD hide behavior. The group stays active only while the hotkey is held.");
    ImGui::PopItemWidth();
    if (group.isHoldMode())
    {
     ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
     bool holdInverted = group.isHoldInverted();
     ImGui::Checkbox("Invert hold behavior", &holdInverted);
     group.setHoldInverted(holdInverted);
     ImGui::SameLine();
     showHelpMarker("When enabled, the group is active normally and turns off only while the hotkey is held.");
     ImGui::PopItemWidth();
    }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
    bool timedMode = group.isTimedMode();
    if (ImGui::Checkbox("Auto-hide mode", &timedMode))
    {
     group.setTimedMode(timedMode);
    }
    ImGui::SameLine();
    showHelpMarker("Timed mode temporarily switches the group state when a trigger key is used. Suppression keys can prevent this during combo inputs.");
    ImGui::PopItemWidth();
    if (group.isTimedMode())
    {
     ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
     bool timedModeInverted = group.isTimedModeInverted();
     if (ImGui::Checkbox("Invert auto-hide behavior", &timedModeInverted))
     {
      group.setTimedModeInverted(timedModeInverted);
     }
     ImGui::SameLine();
     showHelpMarker("Useful for HUD hide groups. When enabled, the group stays active normally and temporarily turns off when triggered.");
     ImGui::PopItemWidth();
     int delayMs = group.getTimedModeDelayMs();
     ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
     ImGui::Text(group.isTimedModeInverted() ? "Show time" : "Hide delay");
     if (ImGui::SliderInt("##TimedModeDelayMs", &delayMs, 100, 10000, "%d ms"))
     {
      group.setTimedModeDelayMs(delayMs);
     }
     ImGui::PopItemWidth();
     int minVisibleMs = group.getTimedModeMinVisibleMs();
     ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
     ImGui::Text(group.isTimedModeInverted() ? "Minimum shown" : "Minimum visible");
     if (ImGui::SliderInt("##TimedModeMinVisibleMs", &minVisibleMs, 0, 5000, "%d ms"))
     {
      group.setTimedModeMinVisibleMs(minVisibleMs);
     }
     ImGui::SameLine();
     showHelpMarker("Prevents the temporary timed state from ending too quickly.");
     ImGui::PopItemWidth();
     int fadeOutMs = group.getTimedModeFadeOutMs();
     ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
     ImGui::Text("Fade-out linger");
     if (ImGui::SliderInt("##TimedModeFadeOutMs", &fadeOutMs, 0, 5000, "%d ms"))
     {
      group.setTimedModeFadeOutMs(fadeOutMs);
     }
     ImGui::SameLine();
     showHelpMarker("The temporary timed state stays a bit longer before returning to the resting state.");
     ImGui::PopItemWidth();
    }
    if (!isKeyEditing)
    {
     if (ImGui::Button("Save group"))
     {
      group.setEditing(false);
      g_toggleGroupIdKeyBindingEditing = -1;
      g_toggleGroupIdTimedTriggerKeyEditing = -1;
      g_toggleGroupTimedTriggerKeySlotEditing = -1;
      g_toggleGroupIdTimedSuppressionKeyEditing = -1;
      g_toggleGroupTimedSuppressionKeySlotEditing = -1;
      g_keyCollector.clear();
      saveShaderTogglerIniFile();
     }
    }
    ImGui::Separator();
   }
            if(group.isEditing()) { displayEffectFilters(group);ImGui::Separator(); }
   ImGui::PopID();
  }
        if(!shown) ImGui::TextWrapped(g_toggleGroups.empty()?"Create a group to begin hunting shaders.":"No groups match this search. Clear the search or change the filter.");
        if(groupToMove>=0) reorderToggleGroup(groupToMove,groupMoveTarget,groupMoveAfter);
        if (!groupsToDuplicate.empty())
        {
            for (auto& duplicate : groupsToDuplicate) g_toggleGroups.push_back(std::move(duplicate));
            saveShaderTogglerIniFile();
        }
        if (!idsToRemove.empty()) removeToggleGroups(idsToRemove);
        if(groupToRefine>=0) refineEffect(runtime,groupToRefine);
}
static void displayAdvancedTools(effect_runtime* runtime)
{
    addonHeading("Standalone effect finder");
    addonHelpText("Already have a toggle for the effect? Use Refine on that group to narrow the search.");
    ImGui::SetNextItemWidth(std::max(ImGui::GetFontSize()*10,ImGui::GetContentRegionAvail().x*0.55f));
    if(ImGui::Combo("Finder mode",&g_finderMode,"Live comparison\0Paused scene\0")) saveShaderTogglerIniFile();
    addonHelpText(g_finderMode==1?"Pause with the effect visible, then start the scan.":"Capture idle gameplay, then repeat the effect. Follow the on-screen prompts.");
    if(g_finderRuntime) ImGui::TextWrapped("A session is running. Its controls remain above the tabs; stop it before starting another.");
    ImGui::BeginDisabled(g_finderRuntime || g_toggleGroupIdShaderEditing>=0);
    if(ImGui::Button("Find effect in new group"))
    {
        ToggleGroup destination("Found effect",ToggleGroup::getNewGroupId());
        destination.setEditing(true);
        g_toggleGroups.push_back(std::move(destination));
        g_standaloneFinderDestination=g_toggleGroups.back().getId();
        openEffectFinder(runtime,g_toggleGroups.back());
    }
    ImGui::SameLine();showHelpMarker("Results go into a new, empty group. Assign its name and hotkey under Groups after saving a filter.");
    if(addonTreeNode("Use an existing destination"))
    {
        const auto selected=std::find_if(g_toggleGroups.begin(),g_toggleGroups.end(),[](const auto& group){return group.getId()==g_standaloneFinderDestination;});
        const char* name=selected==g_toggleGroups.end()?"Select a group":selected->getName().c_str();
        ImGui::SetNextItemWidth(-1);
        if(ImGui::BeginCombo("##FinderDestination",name))
        {
            for(const auto& group:g_toggleGroups)
            {
                ImGui::PushID(group.getId());
                if(ImGui::Selectable(group.getName().c_str(),group.getId()==g_standaloneFinderDestination)) g_standaloneFinderDestination=group.getId();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::BeginDisabled(selected==g_toggleGroups.end());
        if(ImGui::Button("Find effect"))
            for(auto& group:g_toggleGroups) if(group.getId()==g_standaloneFinderDestination) { openEffectFinder(runtime,group);break; }
        ImGui::EndDisabled();
        ImGui::TextWrapped("Existing whole-shader rules remain in this group. Use a new destination when those rules also hide wanted effects.");
        ImGui::TreePop();
    }
    ImGui::EndDisabled();
}
static void displayActiveTool(effect_runtime* runtime)
{
    for(auto& group:g_toggleGroups)
    {
        if(group.getId()==g_toggleGroupIdShaderEditing)
        {
            ImGui::TextWrapped("Hunting: %s",group.getName().c_str());
            if(ImGui::Button("Done hunting##ActiveTool")) { endShaderEditing(true,group);saveShaderTogglerIniFile(); }
            if(group.getId()==g_toggleGroupIdShaderEditing)
            { ImGui::PushID(group.getId());displaySmartDisable(runtime);ImGui::PopID(); }
            ImGui::Separator();
            break;
        }
        if(runtime==g_finderRuntime && group.getId()==g_finderGroup)
        {
            ImGui::TextWrapped("Finding for: %s",group.getName().c_str());
            ImGui::PushID(group.getId());displayEffectFinder(runtime,group);ImGui::PopID();
            break;
        }
    }
}
static void displaySettings(reshade::api::effect_runtime* runtime)
{
    const ScopedAddonUiScale uiScale;
    const ScopedAddonUiStyle scopedStyle;
    static int loggedPercent=-1;
    static float loggedFont=0;
    if(loggedPercent!=g_uiScalePercent || std::abs(loggedFont-ImGui::GetFontSize())>0.1f)
    {
        diagnostics::Write("[UI] scale_percent=%d requested_percent=%.0f applied_percent=%.0f source_font_pixels=%.1f font_pixels=%.1f method=%s imgui=%s font_api=%u display=%.0fx%.0f",
            g_uiScalePercent,uiScale.factor*100,uiScale.appliedPercent(),uiScale.sourceFont,uiScale.appliedFont,
            uiScale.method(),ImGui::GetVersion(),uiScale.modern.version,ImGui::GetIO().DisplaySize.x,ImGui::GetIO().DisplaySize.y);
        if(!uiScale.applied()) diagnostics::Write("[WARN] UI size was not applied by this ReShade host. Use ReShade Settings to adjust the overlay font size.");
        loggedPercent=g_uiScalePercent;loggedFont=ImGui::GetFontSize();
    }
    const auto active=std::count_if(g_toggleGroups.begin(),g_toggleGroups.end(),[](const auto& g){return g.isActive();});
    if(ImGui::Button("Save all")) saveShaderTogglerIniFile();
    if(ImGui::IsItemHovered()) ImGui::SetTooltip(g_uiSaveResult<0?"Last save failed. Check ShaderTogglerAdvanced.log.":g_uiSaveResult>0?"Last save successful: ShaderToggler.ini":"Save all groups to ShaderToggler.ini.");
    nextUiAction("Size");
    if(ImGui::Button("Size")) ImGui::OpenPopup("Interface size");
    if(ImGui::BeginPopup("Interface size"))
    {
        ImGui::TextUnformatted("Interface size");
        ImGui::TextDisabled("Current: %.0f%% (%.0f px)",uiScale.appliedPercent(),uiScale.appliedFont);
        if(!uiScale.applied()) ImGui::TextWrapped("This ReShade build did not apply the size. Adjust the overlay font size in ReShade Settings.");
        if(ImGui::MenuItem("Auto",nullptr,g_uiScalePercent==0)) { g_uiScalePercent=0;saveShaderTogglerIniFile(); }
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("Targets 32 px at 4K, 22 px at 1440p and 16 px at 1080p. Keeps an already larger font.");
        const int sizes[]={100,125,150,175,200,250};
        for(int size:sizes)
        {
            char label[16];std::snprintf(label,sizeof(label),"%d%%",size);
            if(ImGui::MenuItem(label,nullptr,g_uiScalePercent==size)) { g_uiScalePercent=size;saveShaderTogglerIniFile(); }
        }
        ImGui::EndPopup();
    }
    nextUiAction("20 groups / 20 active");
    ImGui::TextDisabled("%zu groups / %zu active%s",g_toggleGroups.size(),static_cast<size_t>(active),g_allToggleGroupsSuspended?" (suspended)":"");
    if(g_uiSaveResult<0) ImGui::TextWrapped("Save failed. Check ShaderTogglerAdvanced.log.");
    if(g_allToggleGroupsSuspended)
    {
        nextUiAction("Restore");
        if(ImGui::Button("Restore")) restoreAllToggleGroups();
    }
    displayActiveTool(runtime);
    if(beginAddonTabBar())
    {
        static int previousTab=-1;
        const auto enterTab=[&](int tab){if(previousTab>=0 && previousTab!=tab) cancelAllKeyBindingEditors();previousTab=tab;};
        if(beginAddonTabItem("Groups")) { enterTab(0);displayToggleGroups(runtime);ImGui::EndTabItem(); }
        if(beginAddonTabItem("Settings")) { enterTab(1);displayAddonSettings();ImGui::EndTabItem(); }
        if(beginAddonTabItem("Advanced tools")) { enterTab(2);displayAdvancedTools(runtime);ImGui::EndTabItem(); }
        if(beginAddonTabItem("Help")) { enterTab(3);displayAddonHelp();ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
 if (g_toggleGroupIdKeyBindingEditing >= 0 ||
  g_toggleGroupIdTimedTriggerKeyEditing >= 0 ||
  g_toggleGroupIdTimedSuppressionKeyEditing >= 0 ||
  g_globalSuspendHotkeySlotEditing >= 0 ||
  g_globalRestoreHotkeySlotEditing >= 0)
 {
  const bool mouseIsInteractingWithInterface =
   ImGui::IsAnyItemHovered() ||
   ImGui::IsAnyItemActive();
  g_keyCollector.collectKeysPressed(
   runtime,
   !mouseIsInteractingWithInterface);
 }
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID reserved)
{
 switch (fdwReason)
 {
 case DLL_PROCESS_ATTACH:
 {
  diagnostics::Start();
  diagnostics::Write("[INIT] Registering ReShade add-on API %u.", RESHADE_API_VERSION);
  if (!reshade::register_addon(hModule))
  {
   diagnostics::Write("[ERROR] ReShade rejected add-on registration. Check ReShade version and full add-on support.");
   diagnostics::Stop(false);
   return FALSE;
  }
  g_addonRegistered = true;
  diagnostics::Write("[INIT] ReShade add-on registration succeeded.");
  if (diagnostics::error.load())
   reshade::log_message(2, "ShaderToggler Advanced could not create ShaderTogglerAdvanced.log beside the game executable; diagnostic logging is disabled.");
  WCHAR buf[MAX_PATH] = {};
  if (GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf)) != 0)
  {
   const std::filesystem::path executablePath(buf);
   g_syndicateProfile = _wcsicmp(executablePath.filename().c_str(), L"Syndicate.exe") == 0;
   g_iniFileName = executablePath.parent_path() / HASH_FILE_NAME;
  }
  else
  {
   g_iniFileName = HASH_FILE_NAME;
  }
  diagnostics::WritePath("INI", g_iniFileName.c_str());
  if (GetModuleFileNameW(hModule, buf, ARRAYSIZE(buf)) != 0)
   diagnostics::WritePath("Add-on", buf);
  KeyData::refreshControllerTypeDetection();
  diagnostics::Write("[INIT] Controller detection finished.");
  reshade::register_event<reshade::addon_event::init_device>(onDiagnosticInitDevice);
        reshade::register_event<reshade::addon_event::init_device>(smart::initDevice);
        reshade::register_event<reshade::addon_event::destroy_device>(smart::destroyDevice);
        reshade::register_event<reshade::addon_event::init_device>(smart::modern::initDevice);
        reshade::register_event<reshade::addon_event::destroy_device>(smart::modern::destroyDevice);
        smart::dx12::setIndirectCallback(prepareD3D12Indirect);
        smart::dx12::setFinderIndirectCallback(observeD3D12Indirect);
        reshade::register_event<reshade::addon_event::init_device>(smart::dx12::initDevice);
        reshade::register_event<reshade::addon_event::destroy_device>(smart::dx12::destroyDevice);
        reshade::register_event<reshade::addon_event::init_command_queue>(smart::dx12::initQueue);
        reshade::register_event<reshade::addon_event::destroy_command_queue>(smart::dx12::destroyQueue);
        reshade::register_event<reshade::addon_event::init_command_list>(smart::dx12::initCommands);
        reshade::register_event<reshade::addon_event::destroy_command_list>(smart::dx12::destroyCommands);
        reshade::register_event<reshade::addon_event::reset_command_list>(smart::dx12::resetCommands);
        reshade::register_event<reshade::addon_event::close_command_list>(smart::dx12::restore);
        reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(smart::dx12::targets);
        reshade::register_event<reshade::addon_event::begin_render_pass>(smart::dx12::beginPass);
        reshade::register_event<reshade::addon_event::end_render_pass>(smart::dx12::endPass);
        reshade::register_event<reshade::addon_event::init_device>(smart::other::initDevice);
        reshade::register_event<reshade::addon_event::destroy_device>(smart::other::destroyDevice);
        reshade::register_event<reshade::addon_event::init_command_list>(smart::other::initCommands);
        reshade::register_event<reshade::addon_event::destroy_command_list>(smart::other::destroyCommands);
        reshade::register_event<reshade::addon_event::reset_command_list>(smart::other::resetCommands);
        reshade::register_event<reshade::addon_event::close_command_list>(smart::other::restore);
        reshade::register_event<reshade::addon_event::begin_render_pass>(smart::other::beginPass);
        reshade::register_event<reshade::addon_event::end_render_pass>(smart::other::endPass);
        reshade::register_event<reshade::addon_event::execute_secondary_command_list>(onExecuteSecondary);
        reshade::register_event<reshade::addon_event::present>(smart::present);
  reshade::register_event<reshade::addon_event::init_effect_runtime>(onDiagnosticInitRuntime);
        reshade::register_event<reshade::addon_event::destroy_effect_runtime>(onFinderDestroyRuntime);
        reshade::register_event<reshade::addon_event::destroy_device>(onFinderDestroyDevice);
  reshade::register_event<reshade::addon_event::init_pipeline>(onInitPipeline);
  reshade::register_event<reshade::addon_event::init_command_list>(onInitCommandList);
  reshade::register_event<reshade::addon_event::destroy_command_list>(onDestroyCommandList);
  reshade::register_event<reshade::addon_event::reset_command_list>(onResetCommandList);
  reshade::register_event<reshade::addon_event::destroy_pipeline>(onDestroyPipeline);
  reshade::register_event<reshade::addon_event::reshade_overlay>(onReshadeOverlay);
  reshade::register_event<reshade::addon_event::reshade_present>(onReshadePresent);
  reshade::register_event<reshade::addon_event::bind_pipeline>(onBindPipeline);
  reshade::register_event<reshade::addon_event::draw>(onDraw);
  reshade::register_event<reshade::addon_event::draw_indexed>(onDrawIndexed);
  reshade::register_event<reshade::addon_event::dispatch>(onDispatch);
  reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(onDrawOrDispatchIndirect);
  reshade::register_overlay(nullptr, &displaySettings);
  diagnostics::Write("[INIT] Callbacks installed: draw, draw_indexed, direct dispatch, typed indirect commands. Unknown indirect commands pass through.");
  loadShaderTogglerIniFile();
        publishEffectFilters();
  logConfiguration();
  diagnostics::Write("[READY] Initialization finished.");
 }
 break;
 case DLL_PROCESS_DETACH:
  if (!g_addonRegistered)
  {
   diagnostics::Stop(reserved != nullptr);
   break;
  }
  diagnostics::Write("[SHUTDOWN] Removing add-on callbacks.");
        if (!reserved) closeEffectFinder();
        smart::other::shutdown(reserved != nullptr);
        smart::dx12::shutdownCapture(reserved != nullptr);
        smart::other::finishShutdown(reserved != nullptr);
  reshade::unregister_event<reshade::addon_event::init_device>(onDiagnosticInitDevice);
        reshade::unregister_event<reshade::addon_event::init_device>(smart::initDevice);
        reshade::unregister_event<reshade::addon_event::destroy_device>(smart::destroyDevice);
        reshade::unregister_event<reshade::addon_event::init_device>(smart::modern::initDevice);
        reshade::unregister_event<reshade::addon_event::destroy_device>(smart::modern::destroyDevice);
        reshade::unregister_event<reshade::addon_event::init_device>(smart::dx12::initDevice);
        reshade::unregister_event<reshade::addon_event::destroy_device>(smart::dx12::destroyDevice);
        reshade::unregister_event<reshade::addon_event::init_command_queue>(smart::dx12::initQueue);
        reshade::unregister_event<reshade::addon_event::destroy_command_queue>(smart::dx12::destroyQueue);
        reshade::unregister_event<reshade::addon_event::init_command_list>(smart::dx12::initCommands);
        reshade::unregister_event<reshade::addon_event::destroy_command_list>(smart::dx12::destroyCommands);
        reshade::unregister_event<reshade::addon_event::reset_command_list>(smart::dx12::resetCommands);
        reshade::unregister_event<reshade::addon_event::close_command_list>(smart::dx12::restore);
        reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(smart::dx12::targets);
        reshade::unregister_event<reshade::addon_event::begin_render_pass>(smart::dx12::beginPass);
        reshade::unregister_event<reshade::addon_event::end_render_pass>(smart::dx12::endPass);
        reshade::unregister_event<reshade::addon_event::init_device>(smart::other::initDevice);
        reshade::unregister_event<reshade::addon_event::destroy_device>(smart::other::destroyDevice);
        reshade::unregister_event<reshade::addon_event::init_command_list>(smart::other::initCommands);
        reshade::unregister_event<reshade::addon_event::destroy_command_list>(smart::other::destroyCommands);
        reshade::unregister_event<reshade::addon_event::reset_command_list>(smart::other::resetCommands);
        reshade::unregister_event<reshade::addon_event::close_command_list>(smart::other::restore);
        reshade::unregister_event<reshade::addon_event::begin_render_pass>(smart::other::beginPass);
        reshade::unregister_event<reshade::addon_event::end_render_pass>(smart::other::endPass);
        reshade::unregister_event<reshade::addon_event::execute_secondary_command_list>(onExecuteSecondary);
        reshade::unregister_event<reshade::addon_event::present>(smart::present);
  reshade::unregister_event<reshade::addon_event::init_effect_runtime>(onDiagnosticInitRuntime);
        reshade::unregister_event<reshade::addon_event::destroy_effect_runtime>(onFinderDestroyRuntime);
        reshade::unregister_event<reshade::addon_event::destroy_device>(onFinderDestroyDevice);
  g_allToggleGroupsSuspended = false;
  g_pendingSuspendedGroupToggles.clear();
  g_globalSuspensionStarted = {};
  reshade::unregister_event<reshade::addon_event::reshade_present>(onReshadePresent);
  reshade::unregister_event<reshade::addon_event::destroy_pipeline>(onDestroyPipeline);
  reshade::unregister_event<reshade::addon_event::init_pipeline>(onInitPipeline);
  reshade::unregister_event<reshade::addon_event::reshade_overlay>(onReshadeOverlay);
  reshade::unregister_event<reshade::addon_event::bind_pipeline>(onBindPipeline);
  reshade::unregister_event<reshade::addon_event::draw>(onDraw);
  reshade::unregister_event<reshade::addon_event::draw_indexed>(onDrawIndexed);
  reshade::unregister_event<reshade::addon_event::dispatch>(onDispatch);
  reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(onDrawOrDispatchIndirect);
  reshade::unregister_event<reshade::addon_event::init_command_list>(onInitCommandList);
  reshade::unregister_event<reshade::addon_event::destroy_command_list>(onDestroyCommandList);
  reshade::unregister_event<reshade::addon_event::reset_command_list>(onResetCommandList);
  reshade::unregister_overlay(nullptr, &displaySettings);
  reshade::unregister_addon(hModule);
  g_addonRegistered = false;
  diagnostics::Stop(reserved != nullptr);
  break;
 }
 return TRUE;
}
