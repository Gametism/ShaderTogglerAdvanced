#include "ToggleGroup.h"
#include "CDataFile.h"
#include <sstream>
#include <vector>
#include <string>
#include <climits>
#include <cfloat>
#include <Windows.h>

namespace ShaderToggler
{
	static ToggleGroup::GroupId s_nextGroupId = 1;

	ToggleGroup::ToggleGroup(const std::string& name, GroupId id)
		: m_id(id), m_name(name), m_active(false), m_activeAtStartup(false), m_editing(false), m_passMatchMode(PassMatchMode::Balanced)
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

	PassMatchMode ToggleGroup::getPassMatchMode() const
	{
		return m_passMatchMode;
	}

	void ToggleGroup::setPassMatchMode(PassMatchMode mode)
	{
		m_passMatchMode = mode;
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

	void ToggleGroup::clearPassSignatures()
	{
		m_passSignatures.clear();
	}

	void ToggleGroup::storeCollectedPassSignatures(const std::vector<PassSignature>& passes)
	{
		m_passSignatures = passes;
	}

	const std::vector<PassSignature>& ToggleGroup::getPassSignatures() const
	{
		return m_passSignatures;
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
		clearPassSignatures();
		m_passMatchMode = PassMatchMode::Balanced;

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
			m_toggleKey.setKey(VK_CAPITAL, false, false, false);
			m_active = m_activeAtStartup;
			return;
		}

		const std::string prefix = usingCustomFormat ? "GTGroup" : "Group";
		const std::string sectionRoot = prefix + std::to_string(index);
		const std::string vertexHashesCategory = sectionRoot + "_VertexShaders";
		const std::string pixelHashesCategory = sectionRoot + "_PixelShaders";
		const std::string computeHashesCategory = sectionRoot + "_ComputeShaders";
		const std::string passesCategory = sectionRoot + "_Passes";

		int amountShaders = iniFile.GetInt("AmountHashes", vertexHashesCategory);
		if (amountShaders != INT_MIN)
		{
			for (int i = 0; i < amountShaders; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), vertexHashesCategory);
				if (hash != UINT_MAX)
					m_vertexShaderHashes.insert(hash);
			}
		}

		amountShaders = iniFile.GetInt("AmountHashes", pixelHashesCategory);
		if (amountShaders != INT_MIN)
		{
			for (int i = 0; i < amountShaders; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), pixelHashesCategory);
				if (hash != UINT_MAX)
					m_pixelShaderHashes.insert(hash);
			}
		}

		amountShaders = iniFile.GetInt("AmountHashes", computeHashesCategory);
		if (amountShaders != INT_MIN)
		{
			for (int i = 0; i < amountShaders; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), computeHashesCategory);
				if (hash != UINT_MAX)
					m_computeShaderHashes.insert(hash);
			}
		}

		const int amountPasses = iniFile.GetInt("AmountPasses", passesCategory);
		if (amountPasses != INT_MIN)
		{
			for (int i = 0; i < amountPasses; ++i)
			{
				PassSignature sig{};

				const std::string keyPrefix = "Pass" + std::to_string(i);

				uint32_t pipelineLo = iniFile.GetUInt(keyPrefix + "_PipelineLo", passesCategory);
				uint32_t pipelineHi = iniFile.GetUInt(keyPrefix + "_PipelineHi", passesCategory);
				if (pipelineLo != UINT_MAX && pipelineHi != UINT_MAX)
				{
					sig.pixelPipeline =
						(static_cast<uint64_t>(pipelineHi) << 32) |
						static_cast<uint64_t>(pipelineLo);
				}

				uint32_t computeLo = iniFile.GetUInt(keyPrefix + "_ComputeLo", passesCategory);
				uint32_t computeHi = iniFile.GetUInt(keyPrefix + "_ComputeHi", passesCategory);
				if (computeLo != UINT_MAX && computeHi != UINT_MAX)
				{
					sig.computePipeline =
						(static_cast<uint64_t>(computeHi) << 32) |
						static_cast<uint64_t>(computeLo);
				}

				uint32_t rtvLo = iniFile.GetUInt(keyPrefix + "_RTVLo", passesCategory);
				uint32_t rtvHi = iniFile.GetUInt(keyPrefix + "_RTVHi", passesCategory);
				if (rtvLo != UINT_MAX && rtvHi != UINT_MAX)
				{
					sig.renderTargetView =
						(static_cast<uint64_t>(rtvHi) << 32) |
						static_cast<uint64_t>(rtvLo);
				}

				const std::string hasViewportValue = iniFile.GetValue(keyPrefix + "_HasViewport", passesCategory);
				if (!hasViewportValue.empty())
					sig.hasViewport = iniFile.GetBool(keyPrefix + "_HasViewport", passesCategory);

				float value = iniFile.GetFloat(keyPrefix + "_ViewportX", passesCategory);
				if (value != FLT_MIN) sig.viewportX = value;

				value = iniFile.GetFloat(keyPrefix + "_ViewportY", passesCategory);
				if (value != FLT_MIN) sig.viewportY = value;

				value = iniFile.GetFloat(keyPrefix + "_ViewportW", passesCategory);
				if (value != FLT_MIN) sig.viewportWidth = value;

				value = iniFile.GetFloat(keyPrefix + "_ViewportH", passesCategory);
				if (value != FLT_MIN) sig.viewportHeight = value;

				uint32_t uintValue = iniFile.GetUInt(keyPrefix + "_Vertices", passesCategory);
				if (uintValue != UINT_MAX) sig.vertices = uintValue;

				uintValue = iniFile.GetUInt(keyPrefix + "_Indices", passesCategory);
				if (uintValue != UINT_MAX) sig.indices = uintValue;

				uintValue = iniFile.GetUInt(keyPrefix + "_DispatchX", passesCategory);
				if (uintValue != UINT_MAX) sig.dispatchX = uintValue;

				uintValue = iniFile.GetUInt(keyPrefix + "_DispatchY", passesCategory);
				if (uintValue != UINT_MAX) sig.dispatchY = uintValue;

				uintValue = iniFile.GetUInt(keyPrefix + "_DispatchZ", passesCategory);
				if (uintValue != UINT_MAX) sig.dispatchZ = uintValue;

				const std::string indexedValue = iniFile.GetValue(keyPrefix + "_Indexed", passesCategory);
				if (!indexedValue.empty())
					sig.indexed = iniFile.GetBool(keyPrefix + "_Indexed", passesCategory);

				const std::string computeValue = iniFile.GetValue(keyPrefix + "_IsCompute", passesCategory);
				if (!computeValue.empty())
					sig.isCompute = iniFile.GetBool(keyPrefix + "_IsCompute", passesCategory);

				if (sig.pixelPipeline != 0 || sig.computePipeline != 0)
					m_passSignatures.push_back(sig);
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

		const int passMatchModeValue = iniFile.GetInt("PassMatchMode", sectionRoot);
		if (passMatchModeValue >= static_cast<int>(PassMatchMode::Exact) &&
			passMatchModeValue <= static_cast<int>(PassMatchMode::Loose))
		{
			m_passMatchMode = static_cast<PassMatchMode>(passMatchModeValue);
		}
		else
		{
			m_passMatchMode = PassMatchMode::Balanced;
		}

		m_activeAtStartup = iniFile.GetBool("IsActiveAtStartup", sectionRoot);
		m_active = m_activeAtStartup;
	}

	void ToggleGroup::saveState(CDataFile& iniFile, int index, bool usingCustomFormat) const
	{
		const std::string prefix = usingCustomFormat ? "GTGroup" : "Group";
		const std::string sectionRoot = prefix + std::to_string(index);
		const std::string vertexHashesCategory = sectionRoot + "_VertexShaders";
		const std::string pixelHashesCategory = sectionRoot + "_PixelShaders";
		const std::string computeHashesCategory = sectionRoot + "_ComputeShaders";
		const std::string passesCategory = sectionRoot + "_Passes";

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
		for (const auto& pass : m_passSignatures)
		{
			const std::string keyPrefix = "Pass" + std::to_string(counter);

			iniFile.SetUInt(keyPrefix + "_PipelineLo", static_cast<uint32_t>(pass.pixelPipeline & 0xFFFFFFFFull), "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_PipelineHi", static_cast<uint32_t>((pass.pixelPipeline >> 32) & 0xFFFFFFFFull), "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_ComputeLo", static_cast<uint32_t>(pass.computePipeline & 0xFFFFFFFFull), "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_ComputeHi", static_cast<uint32_t>((pass.computePipeline >> 32) & 0xFFFFFFFFull), "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_RTVLo", static_cast<uint32_t>(pass.renderTargetView & 0xFFFFFFFFull), "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_RTVHi", static_cast<uint32_t>((pass.renderTargetView >> 32) & 0xFFFFFFFFull), "", passesCategory);

			iniFile.SetBool(keyPrefix + "_HasViewport", pass.hasViewport, "", passesCategory);
			iniFile.SetFloat(keyPrefix + "_ViewportX", pass.viewportX, "", passesCategory);
			iniFile.SetFloat(keyPrefix + "_ViewportY", pass.viewportY, "", passesCategory);
			iniFile.SetFloat(keyPrefix + "_ViewportW", pass.viewportWidth, "", passesCategory);
			iniFile.SetFloat(keyPrefix + "_ViewportH", pass.viewportHeight, "", passesCategory);

			iniFile.SetUInt(keyPrefix + "_Vertices", pass.vertices, "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_Indices", pass.indices, "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_DispatchX", pass.dispatchX, "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_DispatchY", pass.dispatchY, "", passesCategory);
			iniFile.SetUInt(keyPrefix + "_DispatchZ", pass.dispatchZ, "", passesCategory);

			iniFile.SetBool(keyPrefix + "_Indexed", pass.indexed, "", passesCategory);
			iniFile.SetBool(keyPrefix + "_IsCompute", pass.isCompute, "", passesCategory);

			counter++;
		}
		iniFile.SetUInt("AmountPasses", counter, "", passesCategory);

		iniFile.SetValue("Name", m_name, "", sectionRoot);
		iniFile.SetUInt("ToggleKey", static_cast<uint32_t>(m_toggleKey.toInt()), "", sectionRoot);
		iniFile.SetBool("IsActiveAtStartup", m_activeAtStartup, "", sectionRoot);
		iniFile.SetInt("PassMatchMode", static_cast<int>(m_passMatchMode), "", sectionRoot);
	}
}