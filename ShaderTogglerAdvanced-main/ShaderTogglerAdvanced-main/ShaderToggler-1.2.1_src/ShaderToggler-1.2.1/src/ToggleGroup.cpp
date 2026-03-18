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
#include "ToggleGroup.h"
#include "CDataFile.h"
#include <sstream>

namespace ShaderToggler {

static ToggleGroup::GroupId s_nextGroupId = 1;

ToggleGroup::ToggleGroup(const std::string& name, GroupId id)
    : m_id(id), m_name(name), m_active(false), m_activeAtStartup(false), m_editing(false)
{
}

ToggleGroup::ToggleGroup(const ToggleGroup& other)
    : m_id(getNewGroupId()), // new unique ID for duplicate
      m_name(other.m_name + " Copy"),
      m_active(other.m_active),
      m_activeAtStartup(other.m_activeAtStartup),
      m_editing(false),
      m_toggleKey(other.m_toggleKey),
      m_pixelShaderHashes(other.m_pixelShaderHashes),
      m_vertexShaderHashes(other.m_vertexShaderHashes),
      m_computeShaderHashes(other.m_computeShaderHashes)
{
}

ToggleGroup::GroupId ToggleGroup::getNewGroupId() {
    return s_nextGroupId++;
}

ToggleGroup::GroupId ToggleGroup::getId() const { return m_id; }
void ToggleGroup::setId(GroupId id) { m_id = id; }

const std::string& ToggleGroup::getName() const { return m_name; }
void ToggleGroup::setName(const std::string& name) { m_name = name; }

bool ToggleGroup::isActive() const { return m_active; }
void ToggleGroup::setActive(bool active) { m_active = active; }

bool ToggleGroup::isActiveAtStartup() const { return m_activeAtStartup; }
void ToggleGroup::setIsActiveAtStartup(bool startup) { m_activeAtStartup = startup; }

bool ToggleGroup::isEditing() const { return m_editing; }
void ToggleGroup::setEditing(bool editing) { m_editing = editing; }

void ToggleGroup::setToggleKey(const KeyData& key) { m_toggleKey = key; }
const KeyData& ToggleGroup::getToggleKey() const { return m_toggleKey; }

std::string ToggleGroup::getToggleKeyAsString() const { return m_toggleKey.toString(); }

void ToggleGroup::clearHashes() {
    m_pixelShaderHashes.clear();
    m_vertexShaderHashes.clear();
    m_computeShaderHashes.clear();
}

void ToggleGroup::storeCollectedHashes(const std::vector<uint32_t>& pixel, const std::vector<uint32_t>& vertex, const std::vector<uint32_t>& compute) {
    m_pixelShaderHashes = pixel;
    m_vertexShaderHashes = vertex;
    m_computeShaderHashes = compute;
}

const std::vector<uint32_t>& ToggleGroup::getPixelShaderHashes() const { return m_pixelShaderHashes; }
const std::vector<uint32_t>& ToggleGroup::getVertexShaderHashes() const { return m_vertexShaderHashes; }
const std::vector<uint32_t>& ToggleGroup::getComputeShaderHashes() const { return m_computeShaderHashes; }

void ToggleGroup::loadState(CDataFile& iniFile, int index)
{
    std::stringstream ss;
    ss << "Group" << index;

    m_name = iniFile.GetString("Name", ss.str());
    m_activeAtStartup = iniFile.GetBool("ActiveAtStartup", ss.str());
    uint32_t key = iniFile.GetInt("ToggleKey", ss.str());
    m_toggleKey = KeyData::fromInt(key);

    // Load shader hashes
    m_pixelShaderHashes = iniFile.GetArray("PixelShaderHashes", ss.str());
    m_vertexShaderHashes = iniFile.GetArray("VertexShaderHashes", ss.str());
    m_computeShaderHashes = iniFile.GetArray("ComputeShaderHashes", ss.str());
}

void ToggleGroup::saveState(CDataFile& iniFile, int index) const
{
    std::stringstream ss;
    ss << "Group" << index;

    iniFile.SetString("Name", m_name, ss.str());
    iniFile.SetBool("ActiveAtStartup", m_activeAtStartup, ss.str());
    iniFile.SetInt("ToggleKey", m_toggleKey.toInt(), ss.str());

    iniFile.SetArray("PixelShaderHashes", m_pixelShaderHashes, ss.str());
    iniFile.SetArray("VertexShaderHashes", m_vertexShaderHashes, ss.str());
    iniFile.SetArray("ComputeShaderHashes", m_computeShaderHashes, ss.str());
}

} // namespace ShaderToggler