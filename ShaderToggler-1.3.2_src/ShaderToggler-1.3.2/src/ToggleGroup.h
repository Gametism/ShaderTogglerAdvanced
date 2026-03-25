#pragma once

#include <string>
#include <unordered_set>
#include <reshade.hpp>
#include "ShaderManager.h"
#include "KeyData.h"

namespace ShaderToggler
{
	class ToggleGroup
	{
	public:
		using GroupId = int;

		ToggleGroup(const std::string& name, GroupId id);
		ToggleGroup(const ToggleGroup& other) = default;

		static GroupId getNewGroupId();

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

		bool isHoldMode() const;
		void setHoldMode(bool holdMode);

		bool isTimedMode() const;
		void setTimedMode(bool timedMode);

		bool isHoldInverted() const;
		void setHoldInverted(bool holdInverted);

		int getAutoHideDelayMs() const;
		void setAutoHideDelayMs(int delayMs);

		void setToggleKey(uint8_t newKeyValue, bool shiftRequired = false, bool altRequired = false, bool ctrlRequired = false);
		void setToggleKey(const KeyData& key);
		const KeyData& getToggleKey() const;
		std::string getToggleKeyAsString() const;

		void clearHashes();
		void storeCollectedHashes(
			const std::unordered_set<uint32_t>& pixel,
			const std::unordered_set<uint32_t>& vertex,
			const std::unordered_set<uint32_t>& compute);

		const std::unordered_set<uint32_t>& getPixelShaderHashes() const;
		const std::unordered_set<uint32_t>& getVertexShaderHashes() const;
		const std::unordered_set<uint32_t>& getComputeShaderHashes() const;

		void loadState(class CDataFile& iniFile, int index, bool usingCustomFormat);
		void saveState(class CDataFile& iniFile, int index, bool usingCustomFormat) const;

		ToggleGroup makeDuplicate() const;

	private:
		GroupId m_id;
		std::string m_name;
		bool m_active;
		bool m_activeAtStartup;
		bool m_editing;
		bool m_holdMode;
		bool m_timedMode;
		bool m_holdInverted;
		int m_autoHideDelayMs;
		KeyData m_toggleKey;

		std::unordered_set<uint32_t> m_pixelShaderHashes;
		std::unordered_set<uint32_t> m_vertexShaderHashes;
		std::unordered_set<uint32_t> m_computeShaderHashes;
	};
}