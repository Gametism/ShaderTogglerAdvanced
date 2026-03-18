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
// Modifications (including active-at-startup - x86, group reordering, and UI changes)
// (c) 2025 Sven 'Gametism' Königsmann. All rights reserved.
// 
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notices,
//    this list of conditions, and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notices,
//    this list of conditions, and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////

#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#define ImTextureID unsigned long long

#include <imgui.h>
#include <reshade.hpp>
#include "crc32_hash.hpp"
#include "ShaderManager.h"
#include "CDataFile.h"
#include "ToggleGroup.h"
#include <vector>
#include <filesystem>
#include <Windows.h>
#include <chrono>

void saveShaderTogglerIniFile();

using namespace reshade::api;
using namespace ShaderToggler;

extern "C" __declspec(dllexport) const char *NAME = "Shader Toggler";
extern "C" __declspec(dllexport) const char *DESCRIPTION = "Add-on which allows you to define groups of game shaders to toggle on/off with one key press.";

struct __declspec(uuid("038B03AA-4C75-443B-A695-752D80797037")) CommandListDataContainer {
    uint64_t activePixelShaderPipeline;
    uint64_t activeVertexShaderPipeline;
    uint64_t activeComputeShaderPipeline;
};

#define FRAMECOUNT_COLLECTION_PHASE_DEFAULT 250
#define HASH_FILE_NAME "ShaderToggler.ini"

static ShaderToggler::ShaderManager g_pixelShaderManager;
static ShaderToggler::ShaderManager g_vertexShaderManager;
static ShaderToggler::ShaderManager g_computeShaderManager;
static KeyData g_keyCollector;
static atomic_uint32_t g_activeCollectorFrameCounter = 0;
static std::vector<ToggleGroup> g_toggleGroups;
static atomic_int g_toggleGroupIdKeyBindingEditing = -1;
static atomic_int g_toggleGroupIdShaderEditing = -1;
static float g_overlayOpacity = 1.0f;
static int g_startValueFramecountCollectionPhase = FRAMECOUNT_COLLECTION_PHASE_DEFAULT;
static std::string g_iniFileName = "";

// --- Hold-to-cycle state + helpers ---
static std::chrono::steady_clock::time_point s_lastNP1, s_lastNP2, s_lastNP4, s_lastNP5, s_lastNP7, s_lastNP8;
static bool s_np1Held = false, s_np2Held = false, s_np4Held = false, s_np5Held = false, s_np7Held = false, s_np8Held = false;
static int s_holdRepeatMs = 200;
static bool s_holdDebug = false;

// Returns if either the numpad key OR its navigation-key equivalent is DOWN
static bool is_key_down_numpad_or_nav(reshade::api::effect_runtime* runtime, int vk_numpad, int vk_nav)
{
	bool down = runtime->is_key_down(vk_numpad) || runtime->is_key_down(vk_nav);
	down = down || ((GetAsyncKeyState(vk_numpad) & 0x8000) != 0) || ((GetAsyncKeyState(vk_nav) & 0x8000) != 0);
	return down;
}

// Returns if either the numpad key OR its navigation-key equivalent was PRESSED
static bool is_key_pressed_numpad_or_nav(reshade::api::effect_runtime* runtime, int vk_numpad, int vk_nav)
{
	return runtime->is_key_pressed(vk_numpad) || runtime->is_key_pressed(vk_nav);
}

// Calculates a crc32 hash from shader bytecode
static uint32_t calculateShaderHash(void* shaderData)
{
	if(nullptr==shaderData) return 0;
	const auto shaderDesc = *static_cast<shader_desc *>(shaderData);
	return compute_crc32(static_cast<const uint8_t *>(shaderDesc.code), shaderDesc.code_size);
}

// Adds a default group with VK_CAPITAL as toggle key
void addDefaultGroup()
{
	ToggleGroup toAdd("Default", ToggleGroup::getNewGroupId());
	toAdd.setToggleKey(VK_CAPITAL, false, false, false);
	g_toggleGroups.push_back(toAdd);
}

// Load shader toggler ini file
void loadShaderTogglerIniFile()
{
	CDataFile iniFile;
	if(!iniFile.Load(g_iniFileName)) return;

	int groupCounter = 0;
	const int numberOfGroups = iniFile.GetInt("AmountGroups", "General");
	if(numberOfGroups==INT_MIN)
	{
		addDefaultGroup();
		groupCounter=-1;
	}
	else
	{
		for(int i=0;i<numberOfGroups;i++)
			g_toggleGroups.push_back(ToggleGroup("", ToggleGroup::getNewGroupId()));
	}

	for(auto& group: g_toggleGroups)
	{
		group.loadState(iniFile, groupCounter);
		groupCounter++;
	}
}

// Save shader toggler ini file
void saveShaderTogglerIniFile()
{
	CDataFile iniFile;
	iniFile.SetInt("AmountGroups", g_toggleGroups.size(), "", "General");

	int groupCounter = 0;
	for(const auto& group: g_toggleGroups)
	{
		group.saveState(iniFile, groupCounter);
		groupCounter++;
	}

	iniFile.SetFileName(g_iniFileName);
	iniFile.Save();
}

// Forward declaration of UI helper functions
static void displayShaderManagerStats(ShaderManager&, const char*);
static void displayShaderManagerInfo(ShaderManager&, const char*);

// --- Start & End Shader/Key editing ---
void endKeyBindingEditing(bool acceptCollectedBinding, ToggleGroup& groupEditing)
{
	if (acceptCollectedBinding && g_toggleGroupIdKeyBindingEditing == groupEditing.getId() && g_keyCollector.isValid())
		groupEditing.setToggleKey(g_keyCollector);

	g_toggleGroupIdKeyBindingEditing = -1;
	g_keyCollector.clear();
}

void startKeyBindingEditing(ToggleGroup& groupEditing)
{
	if (g_toggleGroupIdKeyBindingEditing == groupEditing.getId()) return;
	if (g_toggleGroupIdKeyBindingEditing >= 0) endKeyBindingEditing(false, groupEditing);
	g_toggleGroupIdKeyBindingEditing = groupEditing.getId();
}

void endShaderEditing(bool acceptCollectedShaderHashes, ToggleGroup& groupEditing)
{
	if(acceptCollectedShaderHashes && g_toggleGroupIdShaderEditing == groupEditing.getId())
	{
		groupEditing.storeCollectedHashes(
			g_pixelShaderManager.getMarkedShaderHashes(),
			g_vertexShaderManager.getMarkedShaderHashes(),
			g_computeShaderManager.getMarkedShaderHashes()
		);
		g_pixelShaderManager.stopHuntingMode();
		g_vertexShaderManager.stopHuntingMode();
		g_computeShaderManager.stopHuntingMode();
	}
	g_toggleGroupIdShaderEditing = -1;
}

void startShaderEditing(ToggleGroup& groupEditing)
{
	if(g_toggleGroupIdShaderEditing == groupEditing.getId()) return;
	if(g_toggleGroupIdShaderEditing >= 0) endShaderEditing(false, groupEditing);

	g_toggleGroupIdShaderEditing = groupEditing.getId();
	g_activeCollectorFrameCounter = g_startValueFramecountCollectionPhase;

	g_pixelShaderManager.startHuntingMode(groupEditing.getPixelShaderHashes());
	g_vertexShaderManager.startHuntingMode(groupEditing.getVertexShaderHashes());
	g_computeShaderManager.startHuntingMode(groupEditing.getComputeShaderHashes());

	groupEditing.clearHashes();
}

// Helper for tooltip
static void showHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(450.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// ==========================
// Main overlay UI
// ==========================
static void displaySettings(reshade::api::effect_runtime* runtime)
{
	if(g_toggleGroupIdKeyBindingEditing >= 0)
		g_keyCollector.collectKeysPressed(runtime);

	if(ImGui::CollapsingHeader("General info and help"))
	{
		ImGui::PushTextWrapPos();
		ImGui::TextUnformatted("The Shader Toggler allows you to create one or more groups with shaders to toggle on/off. You can assign a keyboard shortcut (including using keys like Shift, Alt and Control) to each group, including a handy name. Each group can have one or more vertex or pixel shaders assigned to it. When you press the assigned keyboard shortcut, any draw calls using these shaders will be disabled, effectively hiding the elements in the 3D scene.");
		ImGui::PopTextWrapPos();
	}

	// Shader collection parameters
	if(ImGui::CollapsingHeader("Shader selection parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
		ImGui::SliderFloat("Overlay opacity", &g_overlayOpacity, 0.01f, 1.0f);
		ImGui::SliderInt("# of frames to collect", &g_startValueFramecountCollectionPhase, 10, 1000);
		showHelpMarker("Number of frames to collect active shaders. Only shaders active in these frames can be marked.");
		ImGui::PopItemWidth();
	}

	// List of toggle groups
	if(ImGui::CollapsingHeader("List of Toggle Groups", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if(ImGui::Button(" New ")) addDefaultGroup();
		ImGui::Separator();

		std::vector<ToggleGroup> toRemove;
		for(auto& group : g_toggleGroups)
		{
			ImGui::PushID(group.getId());
			ImGui::Text("%d: %s (%s%s)", group.getId(), group.getName().c_str(), group.getToggleKeyAsString().c_str(), group.isActive() ? ", Active" : "");
			ImGui::SameLine();
			if(ImGui::Button("Edit")) group.setEditing(true);
			ImGui::SameLine();
			if(ImGui::Button("X")) toRemove.push_back(group);
			ImGui::PopID();
		}

		for(const auto& group : toRemove) std::erase(g_toggleGroups, group);

		if(!g_toggleGroups.empty() && ImGui::Button("Save all Toggle Groups")) saveShaderTogglerIniFile();
	}

	// ==========================
	// Drag & Drop / Duplicate UI
	// ==========================
	if(ImGui::CollapsingHeader("Group Order", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextUnformatted("Drag, drop, or duplicate toggle groups");
		ImGui::Separator();

		float rowHeight = ImGui::GetTextLineHeightWithSpacing() + 4.0f;
		float winHeight = std::min(rowHeight * g_toggleGroups.size() + 20.0f, 800.0f); // max height
		ImGui::BeginChild("##grouporder_inline", ImVec2(0, winHeight), true);

		for(int i = 0; i < (int)g_toggleGroups.size(); ++i)
		{
			ImGui::PushID(i);
			const std::string &name = g_toggleGroups[i].getName();
			ImGui::Selectable(name.c_str(), false);

			// Drag & drop
			if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::SetDragDropPayload("ST_GROUP_INDEX", &i, sizeof(int));
				ImGui::Text("Move: %s", name.c_str());
				ImGui::EndDragDropSource();
			}
			if(ImGui::BeginDragDropTarget())
			{
				if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ST_GROUP_INDEX"))
				{
					int src = *(const int*)payload->Data;
					if(src != i)
					{
						auto tmp = g_toggleGroups[src];
						g_toggleGroups.erase(g_toggleGroups.begin() + src);
						if(src < i) i--;
						g_toggleGroups.insert(g_toggleGroups.begin() + i, std::move(tmp));
						saveShaderTogglerIniFile();
					}
				}
				ImGui::EndDragDropTarget();
			}

			// Duplicate button
			ImGui::SameLine();
			if(ImGui::SmallButton("Duplicate"))
			{
				ToggleGroup copy = g_toggleGroups[i];
				copy.setGroupId(ToggleGroup::getNewGroupId());
				copy.setName(copy.getName() + " (Copy)");
				g_toggleGroups.insert(g_toggleGroups.begin() + i + 1, copy);
				saveShaderTogglerIniFile();
			}

			ImGui::PopID();
		}

		ImGui::EndChild();
	}
}
