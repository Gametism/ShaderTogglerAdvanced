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
#include "ShaderManager.h"
#include "CDataFile.h"
#include "ToggleGroup.h"
#include "KeyData.h"
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
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <cstdint>
#include <climits>
#include <cctype>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

void saveShaderTogglerIniFile();

using namespace reshade::api;
using namespace ShaderToggler;

extern "C" __declspec(dllexport) const char *NAME = "Shader Toggler";
extern "C" __declspec(dllexport) const char *DESCRIPTION = "Add-on which allows you to define groups of game shaders to toggle on/off with one key press.";

struct __declspec(uuid("038B03AA-4C75-443B-A695-752D80797037")) CommandListDataContainer
{
	uint64_t activePixelShaderPipeline;
	uint64_t activeVertexShaderPipeline;
	uint64_t activeComputeShaderPipeline;
};

#define FRAMECOUNT_COLLECTION_PHASE_DEFAULT 250
#define HASH_FILE_NAME "ShaderToggler.ini"

static ShaderManager g_pixelShaderManager;
static ShaderManager g_vertexShaderManager;
static ShaderManager g_computeShaderManager;
static KeyData g_keyCollector;
static std::atomic_uint32_t g_activeCollectorFrameCounter = 0;
static std::vector<ToggleGroup> g_toggleGroups;
static std::atomic_int g_toggleGroupIdKeyBindingEditing = -1;
static std::atomic_int g_toggleGroupIdTimedTriggerKeyEditing = -1;
static std::atomic_int g_toggleGroupTimedTriggerKeySlotEditing = -1;
static std::atomic_int g_toggleGroupIdTimedSuppressionKeyEditing = -1;
static std::atomic_int g_toggleGroupTimedSuppressionKeySlotEditing = -1;
static std::atomic_int g_toggleGroupIdShaderEditing = -1;
static float g_overlayOpacity = 1.0f;
static int g_startValueFramecountCollectionPhase = FRAMECOUNT_COLLECTION_PHASE_DEFAULT;
static std::string g_iniFileName = "";

static std::unordered_map<int, bool> g_groupHotkeyWasDown;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupHotkeyLastToggleTime;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupLastTimedTriggerTime;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupTimedVisibleSince;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupTimedFadeOutStart;
static const int g_groupHotkeyDebounceMs = 150;

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

static bool g_uiStyleInitialized = false;

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
	case VK_END:      return 0;

	case VK_DIVIDE:   return 10;
	case VK_MULTIPLY: return 11;
	case VK_SUBTRACT: return 12;
	case VK_ADD:      return 13;

	case VK_BACK:     return 20;
	case VK_PRIOR:    return 21;
	case VK_NEXT:     return 22;

	case VK_UP:       return 30;
	case VK_RIGHT:    return 31;
	case VK_DOWN:     return 32;
	case VK_LEFT:     return 33;

	case VK_NUMPAD7:  return 40;
	case VK_NUMPAD8:  return 41;
	case VK_NUMPAD9:  return 42;
	case VK_NUMPAD0:  return 43;
	case VK_DECIMAL:  return 44;

	case VK_INSERT:   return 50;
	case VK_DELETE:   return 51;

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

			return toLowerCopy(a.getName()) < toLowerCopy(b.getName());
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

	if (group.getId() == g_toggleGroupIdShaderEditing && previousActive != newActive)
	{
		g_vertexShaderManager.toggleHideMarkedShaders();
		g_pixelShaderManager.toggleHideMarkedShaders();
		g_computeShaderManager.toggleHideMarkedShaders();
	}
}

static std::string buildIniSignature()
{
	std::string data;

	data += "Creator=";
	data += GT_CREATOR;
	data += "|AmountGroups=";
	data += std::to_string(g_toggleGroups.size());
	data += "|ControllerMode=";
	data += std::to_string(static_cast<int>(KeyData::getControllerLabelMode()));
	data += "|GlobalHotkeyModifier=";
	data += std::to_string(KeyData::globalHotkeyModifierToInt(KeyData::getGlobalHotkeyModifier()));

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
		data += "|Key=" + std::to_string(group.getToggleKey().toInt());
		data += "|Startup=" + std::to_string(group.isActiveAtStartup() ? 1 : 0);
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
		for (size_t i = 0; i < group.getTimedSuppressionKeyCount(); ++i)
		{
			data += "|TimedSuppressionKey" + std::to_string(i) + "=" + std::to_string(group.getTimedSuppressionKeyAt(i).toInt());
		}

		appendSortedHashesToSignature(data, "P", group.getPixelShaderHashes());
		appendSortedHashesToSignature(data, "V", group.getVertexShaderHashes());
		appendSortedHashesToSignature(data, "C", group.getComputeShaderHashes());
	}

	return toHex64(fnv1a64(data));
}

static bool fileContainsTopAndBottomWatermark(const std::string& filename)
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

static void rewriteIniWithTopAndBottomWatermark(const std::string& filename)
{
	std::ifstream inFile(filename, std::ios::binary);
	if (!inFile.is_open())
		return;

	std::string content((std::istreambuf_iterator<char>(inFile)),
	                    std::istreambuf_iterator<char>());
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
		return;

	outFile << GT_HEADER << content << GT_FOOTER;
	outFile.close();
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

static void applyModernUiStyle()
{
	if (g_uiStyleInitialized)
		return;

	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowPadding = ImVec2(10.0f, 10.0f);
	style.FramePadding = ImVec2(9.0f, 6.0f);
	style.ItemSpacing = ImVec2(8.0f, 7.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
	style.CellPadding = ImVec2(6.0f, 5.0f);

	style.WindowRounding = 8.0f;
	style.ChildRounding = 6.0f;
	style.FrameRounding = 6.0f;
	style.PopupRounding = 6.0f;
	style.ScrollbarRounding = 6.0f;
	style.GrabRounding = 6.0f;
	style.TabRounding = 6.0f;

	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;

	style.ScrollbarSize = 14.0f;
	style.GrabMinSize = 11.0f;
	style.IndentSpacing = 18.0f;

	style.Colors[ImGuiCol_WindowBg]            = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
	style.Colors[ImGuiCol_ChildBg]             = ImVec4(0.13f, 0.14f, 0.17f, 0.55f);
	style.Colors[ImGuiCol_PopupBg]             = ImVec4(0.11f, 0.12f, 0.15f, 0.96f);
	style.Colors[ImGuiCol_Border]              = ImVec4(0.24f, 0.27f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_Separator]           = ImVec4(0.24f, 0.27f, 0.31f, 1.00f);

	style.Colors[ImGuiCol_FrameBg]             = ImVec4(0.17f, 0.19f, 0.23f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.21f, 0.24f, 0.29f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive]       = ImVec4(0.24f, 0.28f, 0.34f, 1.00f);

	style.Colors[ImGuiCol_Button]              = ImVec4(0.21f, 0.33f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered]       = ImVec4(0.27f, 0.40f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive]        = ImVec4(0.18f, 0.28f, 0.43f, 1.00f);

	style.Colors[ImGuiCol_Header]              = ImVec4(0.18f, 0.25f, 0.34f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered]       = ImVec4(0.24f, 0.32f, 0.43f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive]        = ImVec4(0.20f, 0.28f, 0.38f, 1.00f);

	style.Colors[ImGuiCol_CheckMark]           = ImVec4(0.45f, 0.73f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab]          = ImVec4(0.45f, 0.73f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.36f, 0.66f, 1.00f, 1.00f);

	style.Colors[ImGuiCol_Text]                = ImVec4(0.94f, 0.95f, 0.97f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled]        = ImVec4(0.62f, 0.66f, 0.72f, 1.00f);

	g_uiStyleInitialized = true;
}

void addDefaultGroup()
{
	ToggleGroup toAdd("Default", ToggleGroup::getNewGroupId());
	toAdd.setToggleKey(VK_CAPITAL, false, false, false);
	g_toggleGroups.push_back(toAdd);
}

void loadShaderTogglerIniFile()
{
	CDataFile iniFile;
	if (!iniFile.Load(g_iniFileName))
	{
		return;
	}

	int numberOfGroups = iniFile.GetInt("GTAmountGroups", "General");
	bool usingCustomFormat = true;

	if (numberOfGroups == INT_MIN)
	{
		numberOfGroups = iniFile.GetInt("AmountGroups", "General");
		usingCustomFormat = false;
	}

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

	if (numberOfGroups == INT_MIN)
	{
		addDefaultGroup();
		g_toggleGroups[0].loadState(iniFile, -1, false);
		saveShaderTogglerIniFile();
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
}

void saveShaderTogglerIniFile()
{
	CDataFile iniFile;

	iniFile.SetInt("GTAmountGroups", static_cast<int>(g_toggleGroups.size()), "", "General");
	iniFile.SetValue("Creator", GT_CREATOR, "", "General");
	iniFile.SetValue(GT_CACHE_KEY, buildIniSignature(), "", "General");
	iniFile.SetInt("ControllerLabelMode", static_cast<int>(KeyData::getControllerLabelMode()), "", "General");
	iniFile.SetInt("GlobalHotkeyModifier", KeyData::globalHotkeyModifierToInt(KeyData::getGlobalHotkeyModifier()), "", "General");

	for (int i = 0; i < static_cast<int>(g_toggleGroups.size()); i++)
	{
		g_toggleGroups[i].saveState(iniFile, i, true);
	}

	iniFile.SetFileName(g_iniFileName);
	iniFile.Save();

	rewriteIniWithTopAndBottomWatermark(g_iniFileName);
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
	commandListData.activePixelShaderPipeline = static_cast<uint64_t>(-1);
	commandListData.activeVertexShaderPipeline = static_cast<uint64_t>(-1);
	commandListData.activeComputeShaderPipeline = static_cast<uint64_t>(-1);
}

static void onInitPipeline(device *, pipeline_layout, uint32_t subobjectCount, const pipeline_subobject *subobjects, pipeline pipelineHandle)
{
	for (uint32_t i = 0; i < subobjectCount; ++i)
	{
		switch (subobjects[i].type)
		{
		case pipeline_subobject_type::vertex_shader:
			g_vertexShaderManager.addHashHandlePair(calculateShaderHash(subobjects[i].data), pipelineHandle.handle);
			break;
		case pipeline_subobject_type::pixel_shader:
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

static void onDestroyPipeline(device *, pipeline pipelineHandle)
{
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

static void onReshadeOverlay(reshade::api::effect_runtime *runtime)
{
	(void)runtime;

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
		ImGui::End();
	}
}

static void onBindPipeline(command_list* commandList, pipeline_stage stages, pipeline pipelineHandle)
{
	if (nullptr != commandList && pipelineHandle.handle != 0)
	{
		const bool handleHasPixelShaderAttached = g_pixelShaderManager.isKnownHandle(pipelineHandle.handle);
		const bool handleHasVertexShaderAttached = g_vertexShaderManager.isKnownHandle(pipelineHandle.handle);
		const bool handleHasComputeShaderAttached = g_computeShaderManager.isKnownHandle(pipelineHandle.handle);

		if (!handleHasPixelShaderAttached && !handleHasVertexShaderAttached && !handleHasComputeShaderAttached)
		{
			return;
		}

		CommandListDataContainer& commandListData = commandList->get_private_data<CommandListDataContainer>();

		if (g_activeCollectorFrameCounter > 0)
		{
			if (handleHasPixelShaderAttached) g_pixelShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			if (handleHasVertexShaderAttached) g_vertexShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			if (handleHasComputeShaderAttached) g_computeShaderManager.addActivePipelineHandle(pipelineHandle.handle);
		}
		else
		{
			commandListData.activePixelShaderPipeline = handleHasPixelShaderAttached ? pipelineHandle.handle : commandListData.activePixelShaderPipeline;
			commandListData.activeVertexShaderPipeline = handleHasVertexShaderAttached ? pipelineHandle.handle : commandListData.activeVertexShaderPipeline;
			commandListData.activeComputeShaderPipeline = handleHasComputeShaderAttached ? pipelineHandle.handle : commandListData.activeComputeShaderPipeline;
		}

		if ((stages & pipeline_stage::pixel_shader) == pipeline_stage::pixel_shader && handleHasPixelShaderAttached)
		{
			if (g_activeCollectorFrameCounter > 0) g_pixelShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			commandListData.activePixelShaderPipeline = pipelineHandle.handle;
		}
		if ((stages & pipeline_stage::vertex_shader) == pipeline_stage::vertex_shader && handleHasVertexShaderAttached)
		{
			if (g_activeCollectorFrameCounter > 0) g_vertexShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			commandListData.activeVertexShaderPipeline = pipelineHandle.handle;
		}
		if ((stages & pipeline_stage::compute_shader) == pipeline_stage::compute_shader && handleHasComputeShaderAttached)
		{
			if (g_activeCollectorFrameCounter > 0) g_computeShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			commandListData.activeComputeShaderPipeline = pipelineHandle.handle;
		}
	}
}

bool blockDrawCallForCommandList(command_list* commandList)
{
	if (nullptr == commandList)
	{
		return false;
	}

	const CommandListDataContainer &commandListData = commandList->get_private_data<CommandListDataContainer>();

	uint32_t shaderHash = g_pixelShaderManager.getShaderHash(commandListData.activePixelShaderPipeline);
	bool blockCall = g_pixelShaderManager.isBlockedShader(shaderHash);
	for (auto& group : g_toggleGroups)
	{
		for (auto hash : group.getPixelShaderHashes())
		{
			if (group.isActive() && hash == shaderHash)
			{
				blockCall = true;
				break;
			}
		}
	}

	shaderHash = g_vertexShaderManager.getShaderHash(commandListData.activeVertexShaderPipeline);
	blockCall |= g_vertexShaderManager.isBlockedShader(shaderHash);
	for (auto& group : g_toggleGroups)
	{
		for (auto hash : group.getVertexShaderHashes())
		{
			if (group.isActive() && hash == shaderHash)
			{
				blockCall = true;
				break;
			}
		}
	}

	shaderHash = g_computeShaderManager.getShaderHash(commandListData.activeComputeShaderPipeline);
	blockCall |= g_computeShaderManager.isBlockedShader(shaderHash);
	for (auto& group : g_toggleGroups)
	{
		for (auto hash : group.getComputeShaderHashes())
		{
			if (group.isActive() && hash == shaderHash)
			{
				blockCall = true;
				break;
			}
		}
	}

	return blockCall;
}

static bool onDraw(command_list* commandList, uint32_t, uint32_t, uint32_t, uint32_t)
{
	if (g_activeCollectorFrameCounter > 0 && commandList != nullptr)
	{
		const CommandListDataContainer& commandListData = commandList->get_private_data<CommandListDataContainer>();

		if (commandListData.activePixelShaderPipeline != static_cast<uint64_t>(-1) &&
			commandListData.activePixelShaderPipeline != 0)
		{
			g_pixelShaderManager.addActivePipelineHandle(commandListData.activePixelShaderPipeline);
		}

		if (commandListData.activeVertexShaderPipeline != static_cast<uint64_t>(-1) &&
			commandListData.activeVertexShaderPipeline != 0)
		{
			g_vertexShaderManager.addActivePipelineHandle(commandListData.activeVertexShaderPipeline);
		}

		if (commandListData.activeComputeShaderPipeline != static_cast<uint64_t>(-1) &&
			commandListData.activeComputeShaderPipeline != 0)
		{
			g_computeShaderManager.addActivePipelineHandle(commandListData.activeComputeShaderPipeline);
		}
	}

	return blockDrawCallForCommandList(commandList);
}

static bool onDrawIndexed(command_list* commandList, uint32_t, uint32_t, uint32_t, int32_t, uint32_t)
{
	if (g_activeCollectorFrameCounter > 0 && commandList != nullptr)
	{
		const CommandListDataContainer& commandListData = commandList->get_private_data<CommandListDataContainer>();

		if (commandListData.activePixelShaderPipeline != static_cast<uint64_t>(-1) &&
			commandListData.activePixelShaderPipeline != 0)
		{
			g_pixelShaderManager.addActivePipelineHandle(commandListData.activePixelShaderPipeline);
		}

		if (commandListData.activeVertexShaderPipeline != static_cast<uint64_t>(-1) &&
			commandListData.activeVertexShaderPipeline != 0)
		{
			g_vertexShaderManager.addActivePipelineHandle(commandListData.activeVertexShaderPipeline);
		}

		if (commandListData.activeComputeShaderPipeline != static_cast<uint64_t>(-1) &&
			commandListData.activeComputeShaderPipeline != 0)
		{
			g_computeShaderManager.addActivePipelineHandle(commandListData.activeComputeShaderPipeline);
		}
	}

	return blockDrawCallForCommandList(commandList);
}

static bool onDrawOrDispatchIndirect(command_list* commandList, indirect_command type, resource, uint64_t, uint32_t, uint32_t)
{
	switch (type)
	{
	case indirect_command::unknown:
	case indirect_command::draw:
	case indirect_command::draw_indexed:
	case indirect_command::dispatch:
		return blockDrawCallForCommandList(commandList);
	default:
		return false;
	}
}

static void onReshadePresent(effect_runtime* runtime)
{
	if (g_activeCollectorFrameCounter > 0)
	{
		--g_activeCollectorFrameCounter;
	}

	for (auto& group : g_toggleGroups)
	{
		const bool isDownNow = group.getToggleKey().isKeyDown(runtime);
		const bool timedSuppressedNow = isAnyTimedSuppressionKeyDown(group, runtime);
		const bool timedTriggerActiveNow = !timedSuppressedNow && isAnyTimedTriggerActive(group, runtime);
		const auto nowTime = std::chrono::steady_clock::now();

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
}

void endShaderEditing(bool acceptCollectedShaderHashes, ToggleGroup& groupEditing)
{
	if (acceptCollectedShaderHashes && g_toggleGroupIdShaderEditing == groupEditing.getId())
	{
		groupEditing.storeCollectedHashes(
			g_pixelShaderManager.getMarkedShaderHashes(),
			g_vertexShaderManager.getMarkedShaderHashes(),
			g_computeShaderManager.getMarkedShaderHashes());

		g_pixelShaderManager.stopHuntingMode();
		g_vertexShaderManager.stopHuntingMode();
		g_computeShaderManager.stopHuntingMode();
	}
	g_toggleGroupIdShaderEditing = -1;
}

void startShaderEditing(ToggleGroup& groupEditing)
{
	if (g_toggleGroupIdShaderEditing == groupEditing.getId())
	{
		return;
	}
	if (g_toggleGroupIdShaderEditing >= 0)
	{
		endShaderEditing(false, groupEditing);
	}

	g_toggleGroupIdShaderEditing = groupEditing.getId();
	g_activeCollectorFrameCounter = g_startValueFramecountCollectionPhase;
	g_pixelShaderManager.startHuntingMode(groupEditing.getPixelShaderHashes());
	g_vertexShaderManager.startHuntingMode(groupEditing.getVertexShaderHashes());
	g_computeShaderManager.startHuntingMode(groupEditing.getComputeShaderHashes());

	groupEditing.clearHashes();
}

static void showHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(420.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}
static void displaySettings(reshade::api::effect_runtime* runtime)
{
	applyModernUiStyle();

	if (g_toggleGroupIdKeyBindingEditing >= 0 ||
		g_toggleGroupIdTimedTriggerKeyEditing >= 0 ||
		g_toggleGroupIdTimedSuppressionKeyEditing >= 0)
	{
		g_keyCollector.collectKeysPressed(runtime);
	}

	if (ImGui::CollapsingHeader("General info and help"))
	{
		ImGui::PushTextWrapPos();
		ImGui::TextUnformatted("Create groups, assign hotkeys, hunt shaders, and toggle them in-game.");
		ImGui::TextUnformatted("");
		ImGui::TextUnformatted("Hunting hotkeys:");
		ImGui::TextUnformatted("* Numpad 1 / 2 = previous / next pixel shader");
		ImGui::TextUnformatted("* Ctrl + Numpad 1 / 2 = previous / next marked pixel shader");
		ImGui::TextUnformatted("* Numpad 3 = mark / unmark pixel shader");
		ImGui::TextUnformatted("* Numpad 4 / 5 = previous / next vertex shader");
		ImGui::TextUnformatted("* Ctrl + Numpad 4 / 5 = previous / next marked vertex shader");
		ImGui::TextUnformatted("* Numpad 6 = mark / unmark vertex shader");
		ImGui::TextUnformatted("* Numpad 7 / 8 = previous / next compute shader");
		ImGui::TextUnformatted("* Ctrl + Numpad 7 / 8 = previous / next marked compute shader");
		ImGui::TextUnformatted("* Numpad 9 = mark / unmark compute shader");
		ImGui::TextUnformatted("* Hold 1 / 2 / 4 / 5 / 7 / 8 to scroll faster");
		ImGui::PopTextWrapPos();
	}

	if (ImGui::CollapsingHeader("Shader selection parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);

		ImGui::SliderFloat("Overlay opacity", &g_overlayOpacity, 0.0f, 1.0f);
		ImGui::SameLine();
		showHelpMarker("Set this to 0.0 to make the hunting overlay fully invisible.");

		ImGui::SliderInt("# of frames to collect", &g_startValueFramecountCollectionPhase, 10, 1000);
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

	if (ImGui::CollapsingHeader("List of Toggle Groups", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button(" New "))
		{
			addDefaultGroup();
		}
		ImGui::Separator();

		std::vector<int> idsToRemove;
		int idToDuplicate = -1;

		for (auto& group : g_toggleGroups)
		{
			ImGui::PushID(group.getId());
			ImGui::AlignTextToFramePadding();

			if (ImGui::Button("X"))
			{
				idsToRemove.push_back(group.getId());
			}

			ImGui::SameLine();
			ImGui::Text(" %d ", group.getId());

			ImGui::SameLine();
			if (ImGui::Button("Edit"))
			{
				group.setEditing(true);
			}

			ImGui::SameLine();
			if (ImGui::Button("Duplicate"))
			{
				idToDuplicate = group.getId();
			}

			ImGui::SameLine();
			if (g_toggleGroupIdShaderEditing >= 0)
			{
				if (g_toggleGroupIdShaderEditing == group.getId())
				{
					if (ImGui::Button(" Done "))
					{
						endShaderEditing(true, group);
					}
				}
				else
				{
					ImGui::BeginDisabled(true);
					ImGui::Button("      ");
					ImGui::EndDisabled();
				}
			}
			else
			{
				if (ImGui::Button("Hunt Shaders"))
				{
					startShaderEditing(group);
				}
			}

			ImGui::SameLine();
			ImGui::Text(" %s (%s)", group.getName().c_str(), group.getToggleKeyAsString().c_str());

			if (group.isTimedMode())
			{
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.45f, 1.0f));
				ImGui::Text(group.isTimedModeInverted() ? " Auto-show " : " Auto-hide ");
				ImGui::PopStyleColor();

				if (group.hasTimedSuppressionKeys())
				{
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.55f, 0.35f, 1.0f));
					ImGui::Text(" Suppression ");
					ImGui::PopStyleColor();
				}
			}
			else if (group.isHoldMode())
			{
				ImGui::SameLine();
				if (group.isHoldInverted())
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.78f, 0.40f, 1.0f));
					ImGui::Text(" Hold Invert ");
					ImGui::PopStyleColor();
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.82f, 1.0f, 1.0f));
					ImGui::Text(" Hold ");
					ImGui::PopStyleColor();
				}
			}

			if (group.isActive())
			{
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.90f, 0.45f, 1.0f));
				ImGui::Text(" Active ");
				ImGui::PopStyleColor();
			}

			if (group.isActiveAtStartup())
			{
				ImGui::SameLine();
				ImGui::Text(" (Active at startup)");
			}

			if (group.isEditing())
			{
				ImGui::Separator();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.92f, 0.98f, 1.0f));
				ImGui::Text("Edit group %d", group.getId());
				ImGui::PopStyleColor();

				char tmpBuffer[150] = {};
				strncpy_s(tmpBuffer, sizeof(tmpBuffer), group.getName().c_str(), _TRUNCATE);
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
				ImGui::Text("Name");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
				ImGui::InputText("##Name", tmpBuffer, 149);
				group.setName(tmpBuffer);
				ImGui::PopItemWidth();

				bool isKeyEditing = false;
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
				ImGui::Text("Key shortcut");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
				std::string textBoxContents = (g_toggleGroupIdKeyBindingEditing == group.getId()) ? g_keyCollector.getKeyAsString() : group.getToggleKeyAsString();

				char keyBuf[128] = {};
				strncpy_s(keyBuf, sizeof(keyBuf), textBoxContents.c_str(), _TRUNCATE);
				ImGui::InputText("##Key shortcut", keyBuf, sizeof(keyBuf), ImGuiInputTextFlags_ReadOnly);

				if (ImGui::IsItemClicked())
				{
					startKeyBindingEditing(group);
				}
				if (g_toggleGroupIdKeyBindingEditing == group.getId())
				{
					isKeyEditing = true;
					ImGui::SameLine();
					if (ImGui::Button("OK"))
					{
						endKeyBindingEditing(true, group);
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						endKeyBindingEditing(false, group);
					}
				}
				ImGui::PopItemWidth();

				ImGui::Text("Timed triggers");
				ImGui::SameLine();
				showHelpMarker("Each trigger can either activate on press, refresh while held, or do both. If none are set, timed mode falls back to the main hotkey.");

				for (size_t triggerIndex = 0; triggerIndex < group.getTimedTriggerKeyCount(); ++triggerIndex)
				{
					ImGui::PushID(static_cast<int>(triggerIndex));

					ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
					ImGui::Text(" ");
					ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);

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
						ImGui::SameLine();
						if (ImGui::Button("OK##TimedTrigger"))
						{
							endTimedTriggerKeyBindingEditing(true, group);
						}
						ImGui::SameLine();
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

					ImGui::SameLine();
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

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
				ImGui::Text(" ");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);

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
					ImGui::SameLine();
					if (ImGui::Button("OK##AddTimedTrigger"))
					{
						endTimedTriggerKeyBindingEditing(true, group);
					}
					ImGui::SameLine();
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

					ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
					ImGui::Text(" ");
					ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);

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
						ImGui::SameLine();
						if (ImGui::Button("OK##TimedSuppression"))
						{
							endTimedSuppressionKeyBindingEditing(true, group);
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel##TimedSuppression"))
						{
							endTimedSuppressionKeyBindingEditing(false, group);
						}
					}

					ImGui::SameLine();
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

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
				ImGui::Text(" ");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);

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
					ImGui::SameLine();
					if (ImGui::Button("OK##AddTimedSuppression"))
					{
						endTimedSuppressionKeyBindingEditing(true, group);
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel##AddTimedSuppression"))
					{
						endTimedSuppressionKeyBindingEditing(false, group);
					}
				}
				ImGui::PopItemWidth();

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
				ImGui::Text(" ");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
				bool isDefaultActive = group.isActiveAtStartup();
				ImGui::Checkbox("Is active at startup", &isDefaultActive);
				group.setIsActiveAtStartup(isDefaultActive);
				ImGui::PopItemWidth();

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
				ImGui::Text(" ");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
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
					ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
					ImGui::Text(" ");
					ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
					bool holdInverted = group.isHoldInverted();
					ImGui::Checkbox("Invert hold behavior", &holdInverted);
					group.setHoldInverted(holdInverted);
					ImGui::SameLine();
					showHelpMarker("When enabled, the group is active normally and turns off only while the hotkey is held.");
					ImGui::PopItemWidth();
				}

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
				ImGui::Text(" ");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
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
					ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
					ImGui::Text(" ");
					ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
					bool timedModeInverted = group.isTimedModeInverted();
					if (ImGui::Checkbox("Invert auto-hide behavior", &timedModeInverted))
					{
						group.setTimedModeInverted(timedModeInverted);
					}
					ImGui::SameLine();
					showHelpMarker("Useful for HUD hide groups. When enabled, the group stays active normally and temporarily turns off when triggered.");
					ImGui::PopItemWidth();

					int delayMs = group.getTimedModeDelayMs();
					ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f);
					ImGui::Text(group.isTimedModeInverted() ? "Show time" : "Hide delay");
					ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
					if (ImGui::SliderInt("##TimedModeDelayMs", &delayMs, 100, 10000, "%d ms"))
					{
						group.setTimedModeDelayMs(delayMs);
					}
					ImGui::PopItemWidth();

					int minVisibleMs = group.getTimedModeMinVisibleMs();
					ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f);
					ImGui::Text(group.isTimedModeInverted() ? "Minimum shown" : "Minimum visible");
					ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
					if (ImGui::SliderInt("##TimedModeMinVisibleMs", &minVisibleMs, 0, 5000, "%d ms"))
					{
						group.setTimedModeMinVisibleMs(minVisibleMs);
					}
					ImGui::SameLine();
					showHelpMarker("Prevents the temporary timed state from ending too quickly.");
					ImGui::PopItemWidth();

					int fadeOutMs = group.getTimedModeFadeOutMs();
					ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f);
					ImGui::Text("Fade-out linger");
					ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
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
					if (ImGui::Button("OK"))
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

			ImGui::PopID();
		}

		if (idToDuplicate >= 0)
		{
			ToggleGroup duplicate("", ToggleGroup::getNewGroupId());
			bool hasDuplicate = false;

			for (const auto& group : g_toggleGroups)
			{
				if (group.getId() == idToDuplicate)
				{
					duplicate = group.makeDuplicate();
					hasDuplicate = true;
					break;
				}
			}

			if (hasDuplicate)
			{
				g_toggleGroups.push_back(duplicate);
				saveShaderTogglerIniFile();
			}
		}

		if (!idsToRemove.empty())
		{
			g_toggleGroupIdKeyBindingEditing = -1;
			g_toggleGroupIdTimedTriggerKeyEditing = -1;
			g_toggleGroupTimedTriggerKeySlotEditing = -1;
			g_toggleGroupIdTimedSuppressionKeyEditing = -1;
			g_toggleGroupTimedSuppressionKeySlotEditing = -1;
			g_keyCollector.clear();
			g_toggleGroupIdShaderEditing = -1;
			g_pixelShaderManager.stopHuntingMode();
			g_vertexShaderManager.stopHuntingMode();
			g_computeShaderManager.stopHuntingMode();

			g_toggleGroups.erase(
				std::remove_if(g_toggleGroups.begin(), g_toggleGroups.end(),
					[&](const ToggleGroup& g)
					{
						return std::find(idsToRemove.begin(), idsToRemove.end(), g.getId()) != idsToRemove.end();
					}),
				g_toggleGroups.end());

			for (int id : idsToRemove)
			{
				g_groupHotkeyWasDown.erase(id);
				g_groupHotkeyLastToggleTime.erase(id);
				g_groupLastTimedTriggerTime.erase(id);
				g_groupTimedVisibleSince.erase(id);
				g_groupTimedFadeOutStart.erase(id);
			}

			saveShaderTogglerIniFile();
		}

		ImGui::Separator();
		if (!g_toggleGroups.empty())
		{
			if (ImGui::Button("Save all Toggle Groups"))
			{
				saveShaderTogglerIniFile();
			}
		}
	}

	if (ImGui::CollapsingHeader("Group Order", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextUnformatted("Drag and drop to reorder toggle groups");
		ImGui::SameLine();
		showHelpMarker("Manual ordering is preserved unless a sort button is used.");

		if (ImGui::Button("Sort by hotkey layout"))
		{
			sortToggleGroupsByHotkey();
		}
		ImGui::SameLine();
		showHelpMarker("Sorts groups in the Gametism layout order: End, Numpad / * - +, Backspace/Page keys, arrows, Numpad 7/8/9/0/Decimal, Insert/Delete.");

		ImGui::SameLine();
		if (ImGui::Button("Sort A-Z"))
		{
			sortToggleGroupsByNameAZ();
		}
		ImGui::SameLine();
		showHelpMarker("Sorts groups alphabetically by name, ignoring letter case.");

		ImGui::SameLine();
		if (ImGui::Button("Sort by name length"))
		{
			sortToggleGroupsByNameLength();
		}
		ImGui::SameLine();
		showHelpMarker("Sorts groups from shortest name to longest name. Groups with the same name length are sorted alphabetically.");

		ImGui::Separator();

		float computedHeight = 45.0f * static_cast<float>(g_toggleGroups.size()) + 20.0f;
		float childHeight = (computedHeight < 600.0f) ? computedHeight : 600.0f;
		ImGui::BeginChild("##grouporder_inline", ImVec2(0, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (int i = 0; i < static_cast<int>(g_toggleGroups.size()); ++i)
		{
			ImGui::PushID(i);
			const std::string &name = g_toggleGroups[i].getName();
			ImGui::Selectable(name.c_str(), false);

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::SetDragDropPayload("ST_GROUP_INDEX", &i, sizeof(int));
				ImGui::Text("Move: %s", name.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ST_GROUP_INDEX"))
				{
					int src = *(const int*)payload->Data;
					if (src != i)
					{
						auto tmp = g_toggleGroups[src];
						g_toggleGroups.erase(g_toggleGroups.begin() + src);
						if (src < i) i--;
						g_toggleGroups.insert(g_toggleGroups.begin() + i, std::move(tmp));
						saveShaderTogglerIniFile();
					}
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::PopID();
		}

		ImGui::EndChild();
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (!reshade::register_addon(hModule))
		{
			return FALSE;
		}

		WCHAR buf[MAX_PATH];
		const std::filesystem::path dllPath = GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf)) ? buf : std::filesystem::path();
		const std::filesystem::path basePath = dllPath.parent_path();
		g_iniFileName = (basePath / HASH_FILE_NAME).string();

		KeyData::refreshControllerTypeDetection();

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
		reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(onDrawOrDispatchIndirect);
		reshade::register_overlay(nullptr, &displaySettings);

		loadShaderTogglerIniFile();
	}
	break;

	case DLL_PROCESS_DETACH:
		reshade::unregister_event<reshade::addon_event::reshade_present>(onReshadePresent);
		reshade::unregister_event<reshade::addon_event::destroy_pipeline>(onDestroyPipeline);
		reshade::unregister_event<reshade::addon_event::init_pipeline>(onInitPipeline);
		reshade::unregister_event<reshade::addon_event::reshade_overlay>(onReshadeOverlay);
		reshade::unregister_event<reshade::addon_event::bind_pipeline>(onBindPipeline);
		reshade::unregister_event<reshade::addon_event::draw>(onDraw);
		reshade::unregister_event<reshade::addon_event::draw_indexed>(onDrawIndexed);
		reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(onDrawOrDispatchIndirect);
		reshade::unregister_event<reshade::addon_event::init_command_list>(onInitCommandList);
		reshade::unregister_event<reshade::addon_event::destroy_command_list>(onDestroyCommandList);
		reshade::unregister_event<reshade::addon_event::reset_command_list>(onResetCommandList);
		reshade::unregister_overlay(nullptr, &displaySettings);
		reshade::unregister_addon(hModule);
		break;
	}

	return TRUE;
}
