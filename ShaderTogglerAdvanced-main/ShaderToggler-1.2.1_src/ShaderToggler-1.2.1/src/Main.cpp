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
#include <imgui.h>
#include <vector>
#include <string>
#include <algorithm>

// -------------------
// Simple stub classes to replace KeyData & CDataFile
// -------------------
struct KeyData {
    int key = 0;
    std::string toString() const { return std::to_string(key); }
    void fromInt(int k) { key = k; }
    int toInt() const { return key; }
};

// Stub for shader data saving (CDataFile)
void saveShaderTogglerIniFile() {
    // Stub: do nothing for now to avoid compilation errors
}

// -------------------
// Toggle structure
// -------------------
struct Toggle {
    std::string name;
    bool enabled = false;
    KeyData key;
};

// Global toggle list
std::vector<Toggle> toggles;

// -------------------
// ImGui UI
// -------------------
void ShowToggleWindow() {
    // Make window big enough to fit all toggles
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("ShaderToggler Advanced");

    // Drag & drop list for toggles
    for (size_t i = 0; i < toggles.size(); i++) {
        Toggle& t = toggles[i];

        ImGui::PushID(static_cast<int>(i));

        ImGui::Checkbox(t.name.c_str(), &t.enabled);
        ImGui::SameLine();
        ImGui::Text("Key: %s", t.key.toString().c_str());

        // Duplicate button
        ImGui::SameLine();
        if (ImGui::Button("Duplicate")) {
            toggles.push_back(t); // duplicate this toggle
        }

        // Drag & drop reordering
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("DND_TOGGLE", &i, sizeof(size_t));
            ImGui::Text("Moving %s", t.name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_TOGGLE")) {
                size_t srcIndex = *(const size_t*)payload->Data;
                if (srcIndex != i) {
                    std::swap(toggles[i], toggles[srcIndex]);
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
    }

    // Add new toggle button
    if (ImGui::Button("Add New Toggle")) {
        toggles.push_back({ "New Toggle", false, KeyData() });
    }

    // Save button
    if (ImGui::Button("Save Toggles")) {
        saveShaderTogglerIniFile(); // stubbed
    }

    ImGui::End();
}

// -------------------
// Main application loop (stub for demo)
// -------------------
int main() {
    // Initialize ImGui context, platform, renderer here
    // This is a stub. In your real app, keep your platform init

    // Example: populate with initial toggles
    toggles.push_back({ "Toggle 1", false, KeyData() });
    toggles.push_back({ "Toggle 2", true, KeyData() });

    // Main loop (pseudo)
    while (true) {
        // Start frame
        ImGui::NewFrame();

        // Show our toggle window
        ShowToggleWindow();

        // Render
        ImGui::Render();

        // Break condition for demo (replace with your window loop)
        // For now just exit after first frame
        break;
    }

    return 0;
}
