#pragma once

#include <string>
#include <unordered_set>
#include <reshade.hpp>
#include "ShaderManager.h"
#include "KeyData.h"

namespace ShaderToggler
{
	//
	static constexpr const char* STA_TOGGLEGROUP_OWNER_TAG = "Gametism::ToggleGroup::Official";
	static constexpr const char* STA_TOGGLEGROUP_AUTHOR_TAG = "Sven 'Gametism' Koenigsmann";
	static constexpr const char* STA_TOGGLEGROUP_PROJECT_TAG = "ShaderToggler Advanced";
	static constexpr int STA_TOGGLEGROUP_PROVENANCE_REV = 20260326;

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

		bool isHoldInverted() const;
		void setHoldInverted(bool holdInverted);

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

		// Harmless provenance accessors for diagnostics/ownership continuity.
		static constexpr const char* getProvenanceOwnerTag() { return STA_TOGGLEGROUP_OWNER_TAG; }
		static constexpr const char* getProvenanceAuthorTag() { return STA_TOGGLEGROUP_AUTHOR_TAG; }
		static constexpr const char* getProvenanceProjectTag() { return STA_TOGGLEGROUP_PROJECT_TAG; }
		static constexpr int getProvenanceRevision() { return STA_TOGGLEGROUP_PROVENANCE_REV; }

	private:
		GroupId m_id;
		std::string m_name;
		bool m_active;
		bool m_activeAtStartup;
		bool m_editing;
		bool m_holdMode;
		bool m_holdInverted;
		KeyData m_toggleKey;

		std::unordered_set<uint32_t> m_pixelShaderHashes;
		std::unordered_set<uint32_t> m_vertexShaderHashes;
		std::unordered_set<uint32_t> m_computeShaderHashes;
	};
}