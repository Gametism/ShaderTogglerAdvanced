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
#include "reshade.hpp"
#include <imgui.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

// ------------------ Globals ------------------
struct ToggleGroup
{
private:
    int id;
    std::string name;
    bool activeAtStartup = false;
    bool editing = false;
    int toggleKey = 0; // Example: ImGuiKey
    // TODO: include shaders list here

public:
    ToggleGroup() : id(nextId()), name("New Group") {}
    ToggleGroup(const ToggleGroup& other) // copy constructor for Duplicate
    {
        id = nextId(); // assign new unique ID
        name = other.name + " Copy";
        activeAtStartup = other.activeAtStartup;
        editing = other.editing;
        toggleKey = other.toggleKey;
        // TODO: copy shader lists
    }

    int getId() const { return id; }
    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }
    bool isActiveAtStartup() const { return activeAtStartup; }
    void setIsActiveAtStartup(bool v) { activeAtStartup = v; }
    bool isEditing() const { return editing; }
    void setEditing(bool e) { editing = e; }

    std::string getToggleKeyAsString() const { return "Key"; /* TODO: convert key */ }

private:
    static int nextId() { static int s_id = 0; return ++s_id; }
};

// Global storage
std::vector<ToggleGroup> g_toggleGroups;
int g_toggleGroupIdKeyBindingEditing = -1;
int g_toggleGroupIdShaderEditing = -1;
float g_overlayOpacity = 0.5f;
int g_startValueFramecountCollectionPhase = 100;

// Dummy functions
void addDefaultGroup() { g_toggleGroups.push_back(ToggleGroup()); }
void saveShaderTogglerIniFile() {}
void startKeyBindingEditing(ToggleGroup& group) { g_toggleGroupIdKeyBindingEditing = group.getId(); }
void endKeyBindingEditing(bool save, ToggleGroup& group) { g_toggleGroupIdKeyBindingEditing = -1; }
void startShaderEditing(ToggleGroup& group) { g_toggleGroupIdShaderEditing = group.getId(); }
void endShaderEditing(bool save, ToggleGroup& group) { g_toggleGroupIdShaderEditing = -1; }

// ------------------ UI ------------------
static void displaySettings(reshade::api::effect_runtime* runtime)
{
    // General info
    if (ImGui::CollapsingHeader("General info and help"))
    {
        ImGui::PushTextWrapPos();
        ImGui::TextUnformatted("The Shader Toggler allows you to create one or more groups with shaders to toggle on/off...");
        ImGui::PopTextWrapPos();
    }

    // Shader selection parameters
    if (ImGui::CollapsingHeader("Shader selection parameters", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
        ImGui::SliderFloat("Overlay opacity", &g_overlayOpacity, 0.01f, 1.0f);
        ImGui::SliderInt("# of frames to collect", &g_startValueFramecountCollectionPhase, 10, 1000);
        ImGui::PopItemWidth();
        ImGui::Separator();
    }

    // List of Toggle Groups
    if (ImGui::CollapsingHeader("List of Toggle Groups", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button(" New ")) addDefaultGroup();
        ImGui::Separator();

        std::vector<ToggleGroup> toRemove;
        for (auto& group : g_toggleGroups)
        {
            ImGui::PushID(group.getId());
            ImGui::AlignTextToFramePadding();

            // Delete
            if (ImGui::Button("X")) toRemove.push_back(group);

            ImGui::SameLine();
            ImGui::Text(" %d ", group.getId());

            // Edit
            ImGui::SameLine();
            if (ImGui::Button("Edit")) group.setEditing(true);

            // Duplicate
            ImGui::SameLine();
            if (ImGui::Button("Duplicate")) g_toggleGroups.push_back(ToggleGroup(group));

            // Change shaders / Done
            ImGui::SameLine();
            if (g_toggleGroupIdShaderEditing >= 0)
            {
                if (g_toggleGroupIdShaderEditing == group.getId())
                {
                    if (ImGui::Button(" Done ")) endShaderEditing(true, group);
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
                if (ImGui::Button("Change shaders")) startShaderEditing(group);
            }

            ImGui::SameLine();
            ImGui::Text(" %s (%s)", group.getName().c_str(), group.getToggleKeyAsString().c_str());

            // Editing UI
            if (group.isEditing())
            {
                ImGui::Separator();
                ImGui::Text("Edit group %d", group.getId());

                // Name
                char tmpBuffer[150];
                strncpy_s(tmpBuffer, 150, group.getName().c_str(), group.getName().size());
                ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
                ImGui::Text("Name");
                ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
                ImGui::InputText("##Name", tmpBuffer, 149);
                group.setName(tmpBuffer);
                ImGui::PopItemWidth();

                // Active at startup
                ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
                bool isDefaultActive = group.isActiveAtStartup();
                ImGui::Checkbox("Is active at startup", &isDefaultActive);
                group.setIsActiveAtStartup(isDefaultActive);
                ImGui::PopItemWidth();

                if (ImGui::Button("OK"))
                {
                    group.setEditing(false);
                    g_toggleGroupIdKeyBindingEditing = -1;
                }
                ImGui::Separator();
            }

            ImGui::PopID();
        }

        // Remove deleted
        for (auto& group : toRemove)
        {
            g_toggleGroups.erase(std::remove(g_toggleGroups.begin(), g_toggleGroups.end(), group), g_toggleGroups.end());
        }

        if (!g_toggleGroups.empty())
        {
            if (ImGui::Button("Save all Toggle Groups")) saveShaderTogglerIniFile();
        }
    }

    // Drag & drop window for group order
    if (ImGui::CollapsingHeader("Group Order", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextUnformatted("Drag and drop to reorder toggle groups");
        ImGui::Separator();
        float childHeight = std::min(600.0f, 50.0f * g_toggleGroups.size());
        ImGui::BeginChild("##grouporder_inline", ImVec2(0, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (int i = 0; i < (int)g_toggleGroups.size(); ++i)
        {
            ImGui::PushID(i);
            const std::string& name = g_toggleGroups[i].getName();
            ImGui::Selectable(name.c_str(), false);

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("ST_GROUP_INDEX", &i, sizeof(int));
                ImGui::Text("Move: %s", name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ST_GROUP_INDEX"))
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

// ------------------ Main entry ------------------
void drawUI(reshade::api::effect_runtime* runtime)
{
    ImGui::Begin("ShaderToggler", nullptr, ImGuiWindowFlags_MenuBar);
    displaySettings(runtime);
    ImGui::End();
}
