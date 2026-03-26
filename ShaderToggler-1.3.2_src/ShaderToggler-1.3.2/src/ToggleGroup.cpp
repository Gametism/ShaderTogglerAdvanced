#include "ToggleGroup.h"
#include "CDataFile.h"
#include <sstream>
#include <vector>

namespace ShaderToggler
{
	static ToggleGroup::GroupId s_nextGroupId = 1;

	ToggleGroup::ToggleGroup(const std::string& name, GroupId id)
		: m_id(id), m_name(name), m_active(false), m_activeAtStartup(false), m_editing(false), m_holdMode(false), m_holdInverted(false)
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

	bool ToggleGroup::isHoldMode() const { return m_holdMode; }
	void ToggleGroup::setHoldMode(bool holdMode)
	{
		m_holdMode = holdMode;
		if (!m_holdMode)
			m_holdInverted = false;
	}

	bool ToggleGroup::isHoldInverted() const { return m_holdInverted; }
	void ToggleGroup::setHoldInverted(bool holdInverted)
	{
		m_holdInverted = holdInverted;
		if (m_holdInverted)
			m_holdMode = true;
	}

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

	void ToggleGroup::clearTextureHashes()
	{
		m_textureHashes.clear();
	}

	void ToggleGroup::storeCollectedTextureHashes(const std::unordered_set<uint64_t>& textures)
	{
		m_textureHashes = textures;
	}

	const std::unordered_set<uint64_t>& ToggleGroup::getTextureHashes() const
	{
		return m_textureHashes;
	}

	ToggleGroup ToggleGroup::makeDuplicate() const
	{
		ToggleGroup copy(*this);
		copy.m_id = getNewGroupId();
		copy.m_name += " Copy";
		copy.m_active = false;
		copy.m_editing = false;
		return copy;
	}

	void ToggleGroup::loadState(CDataFile& iniFile, int index, bool usingCustomFormat)
	{
		clearHashes();
		clearTextureHashes();
		m_holdMode = false;
		m_holdInverted = false;

		if (index < 0)
		{
			int amount = iniFile.GetInt("AmountHashes", "PixelShaders");
			for (int i = 0; i < amount; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), "PixelShaders");
				if (hash != UINT_MAX)
					m_pixelShaderHashes.insert(hash);
			}

			amount = iniFile.GetInt("AmountHashes", "VertexShaders");
			for (int i = 0; i < amount; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), "VertexShaders");
				if (hash != UINT_MAX)
					m_vertexShaderHashes.insert(hash);
			}

			amount = iniFile.GetInt("AmountHashes", "ComputeShaders");
			for (int i = 0; i < amount; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), "ComputeShaders");
				if (hash != UINT_MAX)
					m_computeShaderHashes.insert(hash);
			}

			m_name = "Default";
			m_activeAtStartup = false;
			m_holdMode = false;
			m_holdInverted = false;
			m_toggleKey.setKey(VK_CAPITAL, false, false, false);
			return;
		}

		const std::string prefix = usingCustomFormat ? "GTGroup" : "Group";
		const std::string sectionRoot = prefix + std::to_string(index);
		const std::string vertexHashesCategory = sectionRoot + "_VertexShaders";
		const std::string pixelHashesCategory = sectionRoot + "_PixelShaders";
		const std::string computeHashesCategory = sectionRoot + "_ComputeShaders";
		const std::string textureHashesCategory = sectionRoot + "_Textures";

		int amountShaders = iniFile.GetInt("AmountHashes", vertexHashesCategory);
		for (int i = 0; i < amountShaders; i++)
		{
			uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), vertexHashesCategory);
			if (hash != UINT_MAX)
				m_vertexShaderHashes.insert(hash);
		}

		amountShaders = iniFile.GetInt("AmountHashes", pixelHashesCategory);
		for (int i = 0; i < amountShaders; i++)
		{
			uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), pixelHashesCategory);
			if (hash != UINT_MAX)
				m_pixelShaderHashes.insert(hash);
		}

		amountShaders = iniFile.GetInt("AmountHashes", computeHashesCategory);
		for (int i = 0; i < amountShaders; i++)
		{
			uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), computeHashesCategory);
			if (hash != UINT_MAX)
				m_computeShaderHashes.insert(hash);
		}

		int amountTextures = iniFile.GetInt("AmountHashes", textureHashesCategory);
		for (int i = 0; i < amountTextures; i++)
		{
			const std::string keyBase = "TextureHash" + std::to_string(i);
			uint32_t lo = iniFile.GetUInt(keyBase + "_Lo", textureHashesCategory);
			uint32_t hi = iniFile.GetUInt(keyBase + "_Hi", textureHashesCategory);
			if (lo != UINT_MAX && hi != UINT_MAX)
			{
				uint64_t hash = (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
				m_textureHashes.insert(hash);
			}
		}

		m_name = iniFile.GetValue("Name", sectionRoot);
		if (m_name.empty())
			m_name = "Default";

		const uint32_t toggleKeyValue = iniFile.GetUInt("ToggleKey", sectionRoot);
		if (toggleKeyValue == UINT_MAX)
			m_toggleKey.setKey(VK_CAPITAL, false, false, false);
		else
			m_toggleKey = KeyData::fromInt(toggleKeyValue);

		m_activeAtStartup = iniFile.GetBool("IsActiveAtStartup", sectionRoot);
		m_active = m_activeAtStartup;

		const std::string holdModeValue = iniFile.GetValue("HoldMode", sectionRoot);
		if (!holdModeValue.empty())
			m_holdMode = iniFile.GetBool("HoldMode", sectionRoot);
		else
			m_holdMode = false;

		const std::string holdInvertedValue = iniFile.GetValue("HoldInverted", sectionRoot);
		if (!holdInvertedValue.empty())
			m_holdInverted = iniFile.GetBool("HoldInverted", sectionRoot);
		else
			m_holdInverted = false;

		if (m_holdInverted)
			m_holdMode = true;
	}

	void ToggleGroup::saveState(CDataFile& iniFile, int index, bool usingCustomFormat) const
	{
		const std::string prefix = usingCustomFormat ? "GTGroup" : "Group";
		const std::string sectionRoot = prefix + std::to_string(index);
		const std::string vertexHashesCategory = sectionRoot + "_VertexShaders";
		const std::string pixelHashesCategory = sectionRoot + "_PixelShaders";
		const std::string computeHashesCategory = sectionRoot + "_ComputeShaders";
		const std::string textureHashesCategory = sectionRoot + "_Textures";

		int counter = 0;
		for (const auto hash : m_vertexShaderHashes)
		{
			iniFile.SetUInt("ShaderHash" + std::to_string(counter), hash, "", vertexHashesCategory);
			counter++;
		}
		iniFile.SetUInt("AmountHashes", counter, "", vertexHashesCategory);

		counter = 0;
		for (const auto hash : m_pixelShaderHashes)
		{
			iniFile.SetUInt("ShaderHash" + std::to_string(counter), hash, "", pixelHashesCategory);
			counter++;
		}
		iniFile.SetUInt("AmountHashes", counter, "", pixelHashesCategory);

		counter = 0;
		for (const auto hash : m_computeShaderHashes)
		{
			iniFile.SetUInt("ShaderHash" + std::to_string(counter), hash, "", computeHashesCategory);
			counter++;
		}
		iniFile.SetUInt("AmountHashes", counter, "", computeHashesCategory);

		counter = 0;
		for (const auto hash : m_textureHashes)
		{
			const std::string keyBase = "TextureHash" + std::to_string(counter);
			iniFile.SetUInt(keyBase + "_Lo", static_cast<uint32_t>(hash & 0xFFFFFFFFull), "", textureHashesCategory);
			iniFile.SetUInt(keyBase + "_Hi", static_cast<uint32_t>((hash >> 32) & 0xFFFFFFFFull), "", textureHashesCategory);
			counter++;
		}
		iniFile.SetUInt("AmountHashes", counter, "", textureHashesCategory);

		iniFile.SetValue("Name", m_name, "", sectionRoot);
		iniFile.SetUInt("ToggleKey", static_cast<uint32_t>(m_toggleKey.toInt()), "", sectionRoot);
		iniFile.SetBool("IsActiveAtStartup", m_activeAtStartup, "", sectionRoot);
		iniFile.SetBool("HoldMode", m_holdMode, "", sectionRoot);
		iniFile.SetBool("HoldInverted", m_holdInverted, "", sectionRoot);
	}
}