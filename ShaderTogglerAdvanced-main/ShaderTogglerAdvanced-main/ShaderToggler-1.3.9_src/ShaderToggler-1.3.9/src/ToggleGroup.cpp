#include "ToggleGroup.h"
#include "CDataFile.h"
#include <sstream>
#include <vector>
//GT
namespace ShaderToggler
{
	namespace
	{
		//
		static constexpr const char* STA_TOGGLEGROUP_UNIT_TAG_A = "STA::TG::Gametism";
		static constexpr const char* STA_TOGGLEGROUP_UNIT_TAG_B = "STA::TG::SvenKoenigsmann";
		static constexpr const char* STA_TOGGLEGROUP_UNIT_TAG_C = "STA::TG::ShaderTogglerAdvanced";

		//
		static inline const char* preserve_togglegroup_provenance()
		{
			return STA_TOGGLEGROUP_UNIT_TAG_A;
		}
	}

	static ToggleGroup::GroupId s_nextGroupId = 1;

	ToggleGroup::ToggleGroup(const std::string& name, GroupId id)
		: m_id(id)
		, m_name(name)
		, m_active(false)
		, m_activeAtStartup(false)
		, m_editing(false)
		, m_holdMode(false)
		, m_holdInverted(false)
		, m_timedMode(false)
		, m_timedModeInverted(false)
		, m_timedModeDelayMs(1500)
		, m_timedModeMinVisibleMs(250)
		, m_timedModeFadeOutMs(150)
		, m_reactiveTriggerEnabled(false)
		, m_reactiveTriggerMode(ReactiveTriggerMode::Disabled)
	{
		(void)preserve_togglegroup_provenance();
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

		if (m_holdMode)
			m_timedMode = false;
	}

	bool ToggleGroup::isHoldInverted() const { return m_holdInverted; }
	void ToggleGroup::setHoldInverted(bool holdInverted)
	{
		m_holdInverted = holdInverted;
		if (m_holdInverted)
		{
			m_holdMode = true;
			m_timedMode = false;
		}
	}

	bool ToggleGroup::isTimedMode() const { return m_timedMode; }
	void ToggleGroup::setTimedMode(bool timedMode)
	{
		m_timedMode = timedMode;
		if (m_timedMode)
		{
			m_holdMode = false;
			m_holdInverted = false;
		}
	}

	bool ToggleGroup::isTimedModeInverted() const { return m_timedModeInverted; }
	void ToggleGroup::setTimedModeInverted(bool inverted)
	{
		m_timedModeInverted = inverted;
	}

	int ToggleGroup::getTimedModeDelayMs() const { return m_timedModeDelayMs; }
	void ToggleGroup::setTimedModeDelayMs(int delayMs)
	{
		if (delayMs < 100)
			delayMs = 100;

		m_timedModeDelayMs = delayMs;
	}

	int ToggleGroup::getTimedModeMinVisibleMs() const { return m_timedModeMinVisibleMs; }
	void ToggleGroup::setTimedModeMinVisibleMs(int visibleMs)
	{
		if (visibleMs < 0)
			visibleMs = 0;

		m_timedModeMinVisibleMs = visibleMs;
	}

	int ToggleGroup::getTimedModeFadeOutMs() const { return m_timedModeFadeOutMs; }
	void ToggleGroup::setTimedModeFadeOutMs(int fadeOutMs)
	{
		if (fadeOutMs < 0)
			fadeOutMs = 0;

		m_timedModeFadeOutMs = fadeOutMs;
	}
//GT
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

	void ToggleGroup::addTimedTriggerKey(const KeyData& key, TimedTriggerMode mode)
	{
		if (!key.isValid())
			return;

		TimedTriggerBinding binding;
		binding.key = key;
		binding.mode = mode;
		m_timedTriggerKeys.push_back(binding);
	}

	void ToggleGroup::setTimedTriggerKeyAt(size_t index, const KeyData& key)
	{
		if (!key.isValid())
			return;

		if (index < m_timedTriggerKeys.size())
		{
			m_timedTriggerKeys[index].key = key;
		}
		else if (index == m_timedTriggerKeys.size())
		{
			addTimedTriggerKey(key, TimedTriggerMode::OnPress);
		}
	}

	void ToggleGroup::setTimedTriggerModeAt(size_t index, TimedTriggerMode mode)
	{
		if (index < m_timedTriggerKeys.size())
			m_timedTriggerKeys[index].mode = mode;
	}

	void ToggleGroup::setTimedTriggerBindingAt(size_t index, const TimedTriggerBinding& binding)
	{
		if (!binding.key.isValid())
			return;

		if (index < m_timedTriggerKeys.size())
		{
			m_timedTriggerKeys[index] = binding;
		}
		else if (index == m_timedTriggerKeys.size())
		{
			m_timedTriggerKeys.push_back(binding);
		}
	}

	void ToggleGroup::removeTimedTriggerKeyAt(size_t index)
	{
		if (index >= m_timedTriggerKeys.size())
			return;

		m_timedTriggerKeys.erase(m_timedTriggerKeys.begin() + static_cast<std::ptrdiff_t>(index));
	}

	void ToggleGroup::clearTimedTriggerKeys()
	{
		m_timedTriggerKeys.clear();
		m_reactiveTriggerEnabled = false;
		m_reactiveTriggerMode = ReactiveTriggerMode::Disabled;
		clearReactiveWatcherHashes();
	}

	bool ToggleGroup::hasTimedTriggerKeys() const
	{
		return !m_timedTriggerKeys.empty();
	}

	size_t ToggleGroup::getTimedTriggerKeyCount() const
	{
		return m_timedTriggerKeys.size();
	}

	const KeyData& ToggleGroup::getTimedTriggerKeyAt(size_t index) const
	{
		return m_timedTriggerKeys.at(index).key;
	}

	std::string ToggleGroup::getTimedTriggerKeyAsString(size_t index) const
	{
		return m_timedTriggerKeys.at(index).key.toString();
	}

	ToggleGroup::TimedTriggerMode ToggleGroup::getTimedTriggerModeAt(size_t index) const
	{
		return m_timedTriggerKeys.at(index).mode;
	}

	const ToggleGroup::TimedTriggerBinding& ToggleGroup::getTimedTriggerBindingAt(size_t index) const
	{
		return m_timedTriggerKeys.at(index);
	}

	const std::vector<ToggleGroup::TimedTriggerBinding>& ToggleGroup::getTimedTriggerKeys() const
	{
		return m_timedTriggerKeys;
	}

	const char* ToggleGroup::timedTriggerModeToString(TimedTriggerMode mode)
	{
		switch (mode)
		{
		case TimedTriggerMode::OnPress:
			return "On press";
		case TimedTriggerMode::WhileHeld:
			return "While held";
		case TimedTriggerMode::PressAndHold:
			return "Press + hold";
		default:
			return "On press";
		}
	}

	int ToggleGroup::timedTriggerModeToInt(TimedTriggerMode mode)
	{
		return static_cast<int>(mode);
	}

	ToggleGroup::TimedTriggerMode ToggleGroup::timedTriggerModeFromInt(int value)
	{
		switch (value)
		{
		case 1:
			return TimedTriggerMode::WhileHeld;
		case 2:
			return TimedTriggerMode::PressAndHold;
		case 0:
		default:
			return TimedTriggerMode::OnPress;
		}
	}

//GT
	bool ToggleGroup::isReactiveTriggerEnabled() const
	{
		return m_reactiveTriggerEnabled;
	}

	void ToggleGroup::setReactiveTriggerEnabled(bool enabled)
	{
		m_reactiveTriggerEnabled = enabled;
		if (!enabled)
			m_reactiveTriggerMode = ReactiveTriggerMode::Disabled;
		else if (m_reactiveTriggerMode == ReactiveTriggerMode::Disabled)
			m_reactiveTriggerMode = ReactiveTriggerMode::ActivateWhilePresent;
	}

	ToggleGroup::ReactiveTriggerMode ToggleGroup::getReactiveTriggerMode() const
	{
		return m_reactiveTriggerMode;
	}

	void ToggleGroup::setReactiveTriggerMode(ReactiveTriggerMode mode)
	{
		m_reactiveTriggerMode = mode;
		m_reactiveTriggerEnabled = (mode != ReactiveTriggerMode::Disabled);
	}

	const char* ToggleGroup::reactiveTriggerModeToString(ReactiveTriggerMode mode)
	{
		switch (mode)
		{
		case ReactiveTriggerMode::ActivateWhilePresent:
			return "Activate while watcher is present";
		case ReactiveTriggerMode::DeactivateWhilePresent:
			return "Deactivate while watcher is present";
		case ReactiveTriggerMode::Disabled:
		default:
			return "Disabled";
		}
	}

	int ToggleGroup::reactiveTriggerModeToInt(ReactiveTriggerMode mode)
	{
		return static_cast<int>(mode);
	}

	ToggleGroup::ReactiveTriggerMode ToggleGroup::reactiveTriggerModeFromInt(int value)
	{
		switch (value)
		{
		case 1:
			return ReactiveTriggerMode::ActivateWhilePresent;
		case 2:
			return ReactiveTriggerMode::DeactivateWhilePresent;
		case 0:
		default:
			return ReactiveTriggerMode::Disabled;
		}
	}

	void ToggleGroup::clearReactiveWatcherHashes()
	{
		m_reactivePixelShaderHashes.clear();
		m_reactiveVertexShaderHashes.clear();
		m_reactiveComputeShaderHashes.clear();
	}

	void ToggleGroup::storeReactiveWatcherHashes(
		const std::unordered_set<uint32_t>& pixel,
		const std::unordered_set<uint32_t>& vertex,
		const std::unordered_set<uint32_t>& compute)
	{
		m_reactivePixelShaderHashes = pixel;
		m_reactiveVertexShaderHashes = vertex;
		m_reactiveComputeShaderHashes = compute;
	}

	const std::unordered_set<uint32_t>& ToggleGroup::getReactivePixelShaderHashes() const { return m_reactivePixelShaderHashes; }
	const std::unordered_set<uint32_t>& ToggleGroup::getReactiveVertexShaderHashes() const { return m_reactiveVertexShaderHashes; }
	const std::unordered_set<uint32_t>& ToggleGroup::getReactiveComputeShaderHashes() const { return m_reactiveComputeShaderHashes; }

	void ToggleGroup::clearHashes()
	{
		m_pixelShaderHashes.clear();
		m_vertexShaderHashes.clear();
		m_computeShaderHashes.clear();
	}
//GT
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

	ToggleGroup ToggleGroup::makeDuplicate() const
	{
		ToggleGroup copy(*this);
		copy.m_id = getNewGroupId();
		copy.m_name += " Copy";
		copy.m_active = false;
		copy.m_editing = false;
		return copy;
	}
//GT
	void ToggleGroup::loadState(CDataFile& iniFile, int index, bool usingCustomFormat)
	{
		clearHashes();
		m_holdMode = false;
		m_holdInverted = false;
		m_timedMode = false;
		m_timedModeInverted = false;
		m_timedModeDelayMs = 1500;
		m_timedModeMinVisibleMs = 250;
		m_timedModeFadeOutMs = 150;
		m_timedTriggerKeys.clear();

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
			m_timedMode = false;
			m_timedModeInverted = false;
			m_timedModeDelayMs = 1500;
			m_timedModeMinVisibleMs = 250;
			m_timedModeFadeOutMs = 150;
			m_toggleKey.setKey(VK_CAPITAL, false, false, false);
			m_timedTriggerKeys.clear();
			m_reactiveTriggerEnabled = false;
			m_reactiveTriggerMode = ReactiveTriggerMode::Disabled;
			clearReactiveWatcherHashes();
			return;
		}
//GT
		const std::string prefix = usingCustomFormat ? "GTGroup" : "Group";
		const std::string sectionRoot = prefix + std::to_string(index);
		const std::string vertexHashesCategory = sectionRoot + "_VertexShaders";
		const std::string pixelHashesCategory = sectionRoot + "_PixelShaders";
		const std::string computeHashesCategory = sectionRoot + "_ComputeShaders";
		const std::string reactivePixelHashesCategory = sectionRoot + "_ReactiveWatcherPixelShaders";
		const std::string reactiveVertexHashesCategory = sectionRoot + "_ReactiveWatcherVertexShaders";
		const std::string reactiveComputeHashesCategory = sectionRoot + "_ReactiveWatcherComputeShaders";

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
//GT
		amountShaders = iniFile.GetInt("AmountHashes", computeHashesCategory);
		for (int i = 0; i < amountShaders; i++)
		{
			uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), computeHashesCategory);
			if (hash != UINT_MAX)
				m_computeShaderHashes.insert(hash);
		}


		amountShaders = iniFile.GetInt("AmountHashes", reactiveVertexHashesCategory);
		if (amountShaders != INT_MIN)
		{
			for (int i = 0; i < amountShaders; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), reactiveVertexHashesCategory);
				if (hash != UINT_MAX)
					m_reactiveVertexShaderHashes.insert(hash);
			}
		}

		amountShaders = iniFile.GetInt("AmountHashes", reactivePixelHashesCategory);
		if (amountShaders != INT_MIN)
		{
			for (int i = 0; i < amountShaders; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), reactivePixelHashesCategory);
				if (hash != UINT_MAX)
					m_reactivePixelShaderHashes.insert(hash);
			}
		}

		amountShaders = iniFile.GetInt("AmountHashes", reactiveComputeHashesCategory);
		if (amountShaders != INT_MIN)
		{
			for (int i = 0; i < amountShaders; i++)
			{
				uint32_t hash = iniFile.GetUInt("ShaderHash" + std::to_string(i), reactiveComputeHashesCategory);
				if (hash != UINT_MAX)
					m_reactiveComputeShaderHashes.insert(hash);
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

		const std::vector<uint32_t> timedTriggerKeyValues = iniFile.GetArray("TimedTriggerKeys", sectionRoot);
		const std::vector<uint32_t> timedTriggerModeValues = iniFile.GetArray("TimedTriggerModes", sectionRoot);

		if (!timedTriggerKeyValues.empty())
		{
			for (size_t i = 0; i < timedTriggerKeyValues.size(); ++i)
			{
				KeyData key = KeyData::fromInt(timedTriggerKeyValues[i]);
				if (!key.isValid())
					continue;

				TimedTriggerMode mode = TimedTriggerMode::OnPress;
				if (i < timedTriggerModeValues.size())
					mode = timedTriggerModeFromInt(static_cast<int>(timedTriggerModeValues[i]));

				addTimedTriggerKey(key, mode);
			}
		}
		else
		{
			const uint32_t legacyTimedTriggerKeyValue = iniFile.GetUInt("TimedTriggerKey", sectionRoot);
			if (legacyTimedTriggerKeyValue != UINT_MAX)
			{
				KeyData key = KeyData::fromInt(legacyTimedTriggerKeyValue);
				if (key.isValid())
					addTimedTriggerKey(key, TimedTriggerMode::OnPress);
			}
		}

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

		const std::string timedModeValue = iniFile.GetValue("TimedMode", sectionRoot);
		if (!timedModeValue.empty())
			m_timedMode = iniFile.GetBool("TimedMode", sectionRoot);
		else
			m_timedMode = false;

		const std::string timedModeInvertedValue = iniFile.GetValue("TimedModeInverted", sectionRoot);
		if (!timedModeInvertedValue.empty())
			m_timedModeInverted = iniFile.GetBool("TimedModeInverted", sectionRoot);
		else
			m_timedModeInverted = false;


		const std::string reactiveEnabledValue = iniFile.GetValue("ReactiveTriggerEnabled", sectionRoot);
		if (!reactiveEnabledValue.empty())
			m_reactiveTriggerEnabled = iniFile.GetBool("ReactiveTriggerEnabled", sectionRoot);
		else
			m_reactiveTriggerEnabled = false;

		const int reactiveModeValue = iniFile.GetInt("ReactiveTriggerMode", sectionRoot);
		if (reactiveModeValue != INT_MIN)
			m_reactiveTriggerMode = reactiveTriggerModeFromInt(reactiveModeValue);
		else
			m_reactiveTriggerMode = ReactiveTriggerMode::Disabled;

		if (m_reactiveTriggerMode != ReactiveTriggerMode::Disabled)
			m_reactiveTriggerEnabled = true;
		if (!m_reactiveTriggerEnabled)
			m_reactiveTriggerMode = ReactiveTriggerMode::Disabled;

		const int timedDelayValue = iniFile.GetInt("TimedModeDelayMs", sectionRoot);
		if (timedDelayValue != INT_MIN)
			m_timedModeDelayMs = (timedDelayValue < 100) ? 100 : timedDelayValue;
		else
			m_timedModeDelayMs = 1500;

		const int minVisibleValue = iniFile.GetInt("TimedModeMinVisibleMs", sectionRoot);
		if (minVisibleValue != INT_MIN)
			m_timedModeMinVisibleMs = (minVisibleValue < 0) ? 0 : minVisibleValue;
		else
			m_timedModeMinVisibleMs = 250;

		const int fadeOutValue = iniFile.GetInt("TimedModeFadeOutMs", sectionRoot);
		if (fadeOutValue != INT_MIN)
			m_timedModeFadeOutMs = (fadeOutValue < 0) ? 0 : fadeOutValue;
		else
			m_timedModeFadeOutMs = 150;

		if (m_holdInverted)
			m_holdMode = true;

		if (m_timedMode)
		{
			m_holdMode = false;
			m_holdInverted = false;
		}
	}
//GT 
	void ToggleGroup::saveState(CDataFile& iniFile, int index, bool usingCustomFormat) const
	{
		const std::string prefix = usingCustomFormat ? "GTGroup" : "Group";
		const std::string sectionRoot = prefix + std::to_string(index);
		const std::string vertexHashesCategory = sectionRoot + "_VertexShaders";
		const std::string pixelHashesCategory = sectionRoot + "_PixelShaders";
		const std::string computeHashesCategory = sectionRoot + "_ComputeShaders";
		const std::string reactivePixelHashesCategory = sectionRoot + "_ReactiveWatcherPixelShaders";
		const std::string reactiveVertexHashesCategory = sectionRoot + "_ReactiveWatcherVertexShaders";
		const std::string reactiveComputeHashesCategory = sectionRoot + "_ReactiveWatcherComputeShaders";

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
		for (const auto hash : m_reactiveVertexShaderHashes)
		{
			iniFile.SetUInt("ShaderHash" + std::to_string(counter), hash, "", reactiveVertexHashesCategory);
			counter++;
		}
		iniFile.SetUInt("AmountHashes", counter, "", reactiveVertexHashesCategory);

		counter = 0;
		for (const auto hash : m_reactivePixelShaderHashes)
		{
			iniFile.SetUInt("ShaderHash" + std::to_string(counter), hash, "", reactivePixelHashesCategory);
			counter++;
		}
		iniFile.SetUInt("AmountHashes", counter, "", reactivePixelHashesCategory);

		counter = 0;
		for (const auto hash : m_reactiveComputeShaderHashes)
		{
			iniFile.SetUInt("ShaderHash" + std::to_string(counter), hash, "", reactiveComputeHashesCategory);
			counter++;
		}
		iniFile.SetUInt("AmountHashes", counter, "", reactiveComputeHashesCategory);

		iniFile.SetValue("Name", m_name, "", sectionRoot);
		iniFile.SetUInt("ToggleKey", static_cast<uint32_t>(m_toggleKey.toInt()), "", sectionRoot);

		if (!m_timedTriggerKeys.empty())
		{
			std::vector<uint32_t> timedTriggerKeyValues;
			std::vector<uint32_t> timedTriggerModeValues;

			timedTriggerKeyValues.reserve(m_timedTriggerKeys.size());
			timedTriggerModeValues.reserve(m_timedTriggerKeys.size());

			for (const TimedTriggerBinding& binding : m_timedTriggerKeys)
			{
				if (!binding.key.isValid())
					continue;

				timedTriggerKeyValues.push_back(static_cast<uint32_t>(binding.key.toInt()));
				timedTriggerModeValues.push_back(static_cast<uint32_t>(timedTriggerModeToInt(binding.mode)));
			}

			iniFile.SetArray("TimedTriggerKeys", timedTriggerKeyValues, "", sectionRoot);
			iniFile.SetArray("TimedTriggerModes", timedTriggerModeValues, "", sectionRoot);
		}

		iniFile.SetBool("IsActiveAtStartup", m_activeAtStartup, "", sectionRoot);
		iniFile.SetBool("HoldMode", m_holdMode, "", sectionRoot);
		iniFile.SetBool("HoldInverted", m_holdInverted, "", sectionRoot);
		iniFile.SetBool("TimedMode", m_timedMode, "", sectionRoot);
		iniFile.SetBool("TimedModeInverted", m_timedModeInverted, "", sectionRoot);
		iniFile.SetInt("TimedModeDelayMs", m_timedModeDelayMs, "", sectionRoot);
		iniFile.SetInt("TimedModeMinVisibleMs", m_timedModeMinVisibleMs, "", sectionRoot);
		iniFile.SetInt("TimedModeFadeOutMs", m_timedModeFadeOutMs, "", sectionRoot);
		iniFile.SetBool("ReactiveTriggerEnabled", m_reactiveTriggerEnabled, "", sectionRoot);
		iniFile.SetInt("ReactiveTriggerMode", reactiveTriggerModeToInt(m_reactiveTriggerMode), "", sectionRoot);
	}
}
//GT