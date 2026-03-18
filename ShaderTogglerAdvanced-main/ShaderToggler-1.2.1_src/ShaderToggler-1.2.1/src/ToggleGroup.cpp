///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – A shader toggler add-on for ReShade 5+
// which allows you to define groups of shaders to toggle them on/off
// with one key press.
//
/////////////////////////////////////////////////////////////////////////

#include "ToggleGroup.h"
#include "CDataFile.h"
#include <sstream>
#include <vector>

namespace ShaderToggler
{
	static ToggleGroup::GroupId s_nextGroupId = 1;

	ToggleGroup::ToggleGroup(const std::string& name, GroupId id)
		: m_id(id), m_name(name), m_active(false), m_activeAtStartup(false), m_editing(false)
	{
	}

	ToggleGroup::ToggleGroup(const ToggleGroup& other)
		: m_id(getNewGroupId()),
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

	ToggleGroup::GroupId ToggleGroup::getNewGroupId()
	{
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

	void ToggleGroup::setToggleKey(uint8_t newKeyValue, bool shiftRequired, bool altRequired, bool ctrlRequired)
	{
		m_toggleKey.setKey(newKeyValue, shiftRequired, altRequired, ctrlRequired);
	}

	void ToggleGroup::setToggleKey(const KeyData& key)
	{
		m_toggleKey = key;
	}

	const KeyData& ToggleGroup::getToggleKey() const
	{
		return m_toggleKey;
	}

	std::string ToggleGroup::getToggleKeyAsString() const
	{
		return m_toggleKey.toString();
	}

	void ToggleGroup::clearHashes()
	{
		m_pixelShaderHashes.clear();
		m_vertexShaderHashes.clear();
		m_computeShaderHashes.clear();
	}

	void ToggleGroup::storeCollectedHashes(
		const std::unordered_set<uint32_t>& pixel,
		const std::unordered_set<uint32_t>& vertex,
		const std::unordered_set<uint32_t>& compute)
	{
		m_pixelShaderHashes = pixel;
		m_vertexShaderHashes = vertex;
		m_computeShaderHashes = compute;
	}

	const std::unordered_set<uint32_t>& ToggleGroup::getPixelShaderHashes() const { return m_pixelShaderHashes; }
	const std::unordered_set<uint32_t>& ToggleGroup::getVertexShaderHashes() const { return m_vertexShaderHashes; }
	const std::unordered_set<uint32_t>& ToggleGroup::getComputeShaderHashes() const { return m_computeShaderHashes; }

	void ToggleGroup::loadState(CDataFile& iniFile, int index)
	{
		std::stringstream ss;
		ss << "Group" << index;

		m_name = iniFile.GetString("Name", ss.str());
		m_activeAtStartup = iniFile.GetBool("ActiveAtStartup", ss.str());

		uint32_t key = iniFile.GetUInt("ToggleKey", ss.str());
		m_toggleKey = KeyData::fromInt(key);

		m_pixelShaderHashes.clear();
		m_vertexShaderHashes.clear();
		m_computeShaderHashes.clear();

		for (auto v : iniFile.GetArray("PixelShaderHashes", ss.str()))
			m_pixelShaderHashes.insert(v);
		for (auto v : iniFile.GetArray("VertexShaderHashes", ss.str()))
			m_vertexShaderHashes.insert(v);
		for (auto v : iniFile.GetArray("ComputeShaderHashes", ss.str()))
			m_computeShaderHashes.insert(v);
	}

	void ToggleGroup::saveState(CDataFile& iniFile, int index) const
	{
		std::stringstream ss;
		ss << "Group" << index;

		iniFile.SetString("Name", m_name, "", ss.str());
		iniFile.SetBool("ActiveAtStartup", m_activeAtStartup, "", ss.str());
		iniFile.SetUInt("ToggleKey", static_cast<uint32_t>(m_toggleKey.toInt()), "", ss.str());

		std::vector<uint32_t> pixel(m_pixelShaderHashes.begin(), m_pixelShaderHashes.end());
		std::vector<uint32_t> vertex(m_vertexShaderHashes.begin(), m_vertexShaderHashes.end());
		std::vector<uint32_t> compute(m_computeShaderHashes.begin(), m_computeShaderHashes.end());

		iniFile.SetArray("PixelShaderHashes", pixel, "", ss.str());
		iniFile.SetArray("VertexShaderHashes", vertex, "", ss.str());
		iniFile.SetArray("ComputeShaderHashes", compute, "", ss.str());
	}
}