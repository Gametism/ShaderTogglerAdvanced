///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – A shader toggler add-on for ReShade 5+
// which allows you to define groups of shaders to toggle them on/off
// with one key press.
//
// Based on the original ShaderToggler by Frans 'Otis_Inf' Bouma.
// (c) Frans 'Otis_Inf' Bouma. All rights reserved.
//
// https://github.com/FransBouma/ShaderToggler
//
// Modifications (including active-at-startup - x86, group reordering, duplication and UI changes)
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
static std::atomic_int g_toggleGroupIdShaderEditing = -1;
static float g_overlayOpacity = 1.0f;
static int g_startValueFramecountCollectionPhase = FRAMECOUNT_COLLECTION_PHASE_DEFAULT;
static std::string g_iniFileName = "";

// Per-group hotkey edge/debounce state
static std::unordered_map<int, bool> g_groupHotkeyWasDown;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupHotkeyLastToggleTime;
static const int g_groupHotkeyDebounceMs = 150;

// Hold-to-cycle state with acceleration
static std::chrono::steady_clock::time_point s_lastNP1, s_lastNP2, s_lastNP4, s_lastNP5, s_lastNP7, s_lastNP8;
static std::chrono::steady_clock::time_point s_holdStartNP1, s_holdStartNP2, s_holdStartNP4, s_holdStartNP5, s_holdStartNP7, s_holdStartNP8;
static bool s_np1Held = false, s_np2Held = false, s_np4Held = false, s_np5Held = false, s_np7Held = false, s_np8Held = false;

static const int s_holdRepeatStartMs = 200;
static const int s_holdRepeatMidMs = 120;
static const int s_holdRepeatFastMs = 70;
static const int s_holdRepeatVeryFastMs = 35;

// Sign
static const char* GT_CREATOR = "Gametism";
static const char* GT_CACHE_KEY = "CacheStamp";
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

static bool is_key_down_numpad_or_nav(reshade::api::effect_runtime* runtime, int vk_numpad, int vk_nav)
{
	bool down = runtime->is_key_down(vk_numpad) || runtime->is_key_down(vk_nav);
	down = down || ((GetAsyncKeyState(vk_numpad) & 0x8000) != 0) || ((GetAsyncKeyState(vk_nav) & 0x8000) != 0);
	return down;
}

static bool is_key_pressed_numpad_or_nav(reshade::api::effect_runtime* runtime, int vk_numpad, int vk_nav)
{
	return runtime->is_key_pressed(vk_numpad) || runtime->is_key_pressed(vk_nav);
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

static std::string buildIniSignature()
{
	std::string data;
	data += "Creator=";
	data += GT_CREATOR;
	data += ";AmountGroups=";
	data += std::to_string(g_toggleGroups.size());

	for (const auto& group : g_toggleGroups)
	{
		data += "|Name=" + group.getName();
		data += "|Key=" + std::to_string(group.getToggleKey().toInt());
		data += "|Startup=" + std::to_string(group.isActiveAtStartup() ? 1 : 0);

		for (auto v : group.getPixelShaderHashes())
			data += "|P=" + std::to_string(v);
		for (auto v : group.getVertexShaderHashes())
			data += "|V=" + std::to_string(v);
		for (auto v : group.getComputeShaderHashes())
			data += "|C=" + std::to_string(v);
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

	// Try custom format first
	int numberOfGroups = iniFile.GetInt("GTAmountGroups", "General");
	bool usingCustomFormat = true;

	// Fall back to regular ShaderToggler format
	if (numberOfGroups == INT_MIN)
	{
		numberOfGroups = iniFile.GetInt("AmountGroups", "General");
		usingCustomFormat = false;
	}

	// Legacy pre-group format fallback
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

	// Only enforce watermark/signature on custom files
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

	// Save only in custom format
	iniFile.SetInt("GTAmountGroups", static_cast<int>(g_toggleGroups.size()), "", "General");
	iniFile.SetValue("Creator", GT_CREATOR, "", "General");
	iniFile.SetValue(GT_CACHE_KEY, buildIniSignature(), "", "General");

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
	return blockDrawCallForCommandList(commandList);
}

static bool onDrawIndexed(command_list* commandList, uint32_t, uint32_t, uint32_t, int32_t, uint32_t)
{
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

	// Stable per-group toggle handling using rising edge + debounce
	for (auto& group : g_toggleGroups)
	{
		const bool isDownNow = group.getToggleKey().isKeyDown(runtime);
		bool& wasDownLastFrame = g_groupHotkeyWasDown[group.getId()];
		auto& lastToggleTime = g_groupHotkeyLastToggleTime[group.getId()];

		const auto nowTime = std::chrono::steady_clock::now();
		const auto msSinceLastToggle =
			std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastToggleTime).count();

		if (isDownNow && !wasDownLastFrame && msSinceLastToggle > g_groupHotkeyDebounceMs)
		{
			group.setActive(!group.isActive());
			lastToggleTime = nowTime;

			if (group.getId() == g_toggleGroupIdShaderEditing)
			{
				g_vertexShaderManager.toggleHideMarkedShaders();
				g_pixelShaderManager.toggleHideMarkedShaders();
				g_computeShaderManager.toggleHideMarkedShaders();
			}
		}

		wasDownLastFrame = isDownNow;
	}

	const bool ctrlDown = runtime->is_key_down(VK_CONTROL);
	auto now = std::chrono::steady_clock::now();

	const int NP1 = VK_NUMPAD1, NAV1 = VK_END;
	const int NP2 = VK_NUMPAD2, NAV2 = VK_DOWN;
	const int NP3 = VK_NUMPAD3, NAV3 = VK_NEXT;
	const int NP4 = VK_NUMPAD4, NAV4 = VK_LEFT;
	const int NP5 = VK_NUMPAD5, NAV5 = VK_CLEAR;
	const int NP6 = VK_NUMPAD6, NAV6 = VK_RIGHT;
	const int NP7 = VK_NUMPAD7, NAV7 = VK_HOME;
	const int NP8 = VK_NUMPAD8, NAV8 = VK_UP;
	const int NP9 = VK_NUMPAD9, NAV9 = VK_PRIOR;

	bool np1Down = is_key_down_numpad_or_nav(runtime, NP1, NAV1);
	bool np1Pressed = is_key_pressed_numpad_or_nav(runtime, NP1, NAV1);
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

	bool np2Down = is_key_down_numpad_or_nav(runtime, NP2, NAV2);
	bool np2Pressed = is_key_pressed_numpad_or_nav(runtime, NP2, NAV2);
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

	if (is_key_pressed_numpad_or_nav(runtime, NP3, NAV3))
	{
		g_pixelShaderManager.toggleMarkOnHuntedShader();
	}

	bool np4Down = is_key_down_numpad_or_nav(runtime, NP4, NAV4);
	bool np4Pressed = is_key_pressed_numpad_or_nav(runtime, NP4, NAV4);
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

	bool np5Down = is_key_down_numpad_or_nav(runtime, NP5, NAV5);
	bool np5Pressed = is_key_pressed_numpad_or_nav(runtime, NP5, NAV5);
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

	if (is_key_pressed_numpad_or_nav(runtime, NP6, NAV6))
	{
		g_vertexShaderManager.toggleMarkOnHuntedShader();
	}

	bool np7Down = is_key_down_numpad_or_nav(runtime, NP7, NAV7);
	bool np7Pressed = is_key_pressed_numpad_or_nav(runtime, NP7, NAV7);
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

	bool np8Down = is_key_down_numpad_or_nav(runtime, NP8, NAV8);
	bool np8Pressed = is_key_pressed_numpad_or_nav(runtime, NP8, NAV8);
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

	if (is_key_pressed_numpad_or_nav(runtime, NP9, NAV9))
	{
		g_computeShaderManager.toggleMarkOnHuntedShader();
	}
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
	g_toggleGroupIdKeyBindingEditing = groupEditing.getId();
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

	if (g_toggleGroupIdKeyBindingEditing >= 0)
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
				g_toggleGroups.push_back(group.makeDuplicate());
				saveShaderTogglerIniFile();
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

			if (group.isActive())
			{
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.90f, 0.45f, 1.0f));
				ImGui::Text("active");
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

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
				ImGui::Text(" ");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
				bool isDefaultActive = group.isActiveAtStartup();
				ImGui::Checkbox("Is active at startup", &isDefaultActive);
				group.setIsActiveAtStartup(isDefaultActive);
				ImGui::PopItemWidth();

				if (!isKeyEditing)
				{
					if (ImGui::Button("OK"))
					{
						group.setEditing(false);
						g_toggleGroupIdKeyBindingEditing = -1;
						g_keyCollector.clear();
						saveShaderTogglerIniFile();
					}
				}
				ImGui::Separator();
			}

			ImGui::PopID();
		}

		if (!idsToRemove.empty())
		{
			g_toggleGroupIdKeyBindingEditing = -1;
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