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
#pragma once
#include <string>
#include <vector>
#include <reshade.hpp>
#include "ShaderManager.h"
#include "KeyData.h"

namespace ShaderToggler {

class ToggleGroup {
public:
    using GroupId = int;

    ToggleGroup(const std::string& name, GroupId id);

    // Duplication-safe copy constructor
    ToggleGroup(const ToggleGroup& other);

    GroupId getId() const;
    void setId(GroupId id);

    const std::string& getName() const;
    void setName(const std::string& name);

    bool isActive() const;
    void setActive(bool active);

    bool isActiveAtStartup() const;
    void setIsActiveAtStartup(bool startup);

    bool isEditing() const;
    void setEditing(bool editing);

    void setToggleKey(const KeyData& key);
    const KeyData& getToggleKey() const;
    std::string getToggleKeyAsString() const;

    void clearHashes();
    void storeCollectedHashes(const std::vector<uint32_t>& pixel, const std::vector<uint32_t>& vertex, const std::vector<uint32_t>& compute);
    const std::vector<uint32_t>& getPixelShaderHashes() const;
    const std::vector<uint32_t>& getVertexShaderHashes() const;
    const std::vector<uint32_t>& getComputeShaderHashes() const;

    void loadState(class CDataFile& iniFile, int index);
    void saveState(class CDataFile& iniFile, int index) const;

    static GroupId getNewGroupId();

private:
    GroupId m_id;
    std::string m_name;
    bool m_active;
    bool m_activeAtStartup;
    bool m_editing;
    KeyData m_toggleKey;

    std::vector<uint32_t> m_pixelShaderHashes;
    std::vector<uint32_t> m_vertexShaderHashes;
    std::vector<uint32_t> m_computeShaderHashes;
};

} // namespace ShaderToggler
