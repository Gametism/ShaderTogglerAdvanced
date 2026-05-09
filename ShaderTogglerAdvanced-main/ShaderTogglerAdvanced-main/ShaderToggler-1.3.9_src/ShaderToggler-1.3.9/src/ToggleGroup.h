#pragma once

#include <string>
#include <vector>
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

		enum class ReactiveTriggerMode
		{
			Disabled = 0,
			ActivateWhilePresent = 1,
			DeactivateWhilePresent = 2
		};


	public:
		using GroupId = int;

		enum class TimedTriggerMode
		{
			OnPress = 0,
			WhileHeld = 1,
			PressAndHold = 2
		};

		struct TimedTriggerBinding
		{
			KeyData key;
			TimedTriggerMode mode = TimedTriggerMode::OnPress;
		};

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

		bool isTimedMode() const;
		void setTimedMode(bool timedMode);

		bool isTimedModeInverted() const;
		void setTimedModeInverted(bool inverted);

		int getTimedModeDelayMs() const;
		void setTimedModeDelayMs(int delayMs);

		int getTimedModeMinVisibleMs() const;
		void setTimedModeMinVisibleMs(int visibleMs);

		int getTimedModeFadeOutMs() const;
		void setTimedModeFadeOutMs(int fadeOutMs);

		void setToggleKey(uint8_t newKeyValue, bool shiftRequired = false, bool altRequired = false, bool ctrlRequired = false);
		void setToggleKey(const KeyData& key);
		const KeyData& getToggleKey() const;
		std::string getToggleKeyAsString() const;

		void addTimedTriggerKey(const KeyData& key, TimedTriggerMode mode = TimedTriggerMode::OnPress);
		void setTimedTriggerKeyAt(size_t index, const KeyData& key);
		void setTimedTriggerModeAt(size_t index, TimedTriggerMode mode);
		void setTimedTriggerBindingAt(size_t index, const TimedTriggerBinding& binding);
		void removeTimedTriggerKeyAt(size_t index);
		void clearTimedTriggerKeys();
		bool hasTimedTriggerKeys() const;
		size_t getTimedTriggerKeyCount() const;
		const KeyData& getTimedTriggerKeyAt(size_t index) const;
		std::string getTimedTriggerKeyAsString(size_t index) const;
		TimedTriggerMode getTimedTriggerModeAt(size_t index) const;
		const TimedTriggerBinding& getTimedTriggerBindingAt(size_t index) const;
		const std::vector<TimedTriggerBinding>& getTimedTriggerKeys() const;

		static const char* timedTriggerModeToString(TimedTriggerMode mode);
		static int timedTriggerModeToInt(TimedTriggerMode mode);
		static TimedTriggerMode timedTriggerModeFromInt(int value);

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

	
		bool isReactiveTriggerEnabled() const;
		void setReactiveTriggerEnabled(bool enabled);

		ReactiveTriggerMode getReactiveTriggerMode() const;
		void setReactiveTriggerMode(ReactiveTriggerMode mode);

		const std::unordered_set<uint32_t>& getReactivePixelShaderHashes() const;
		const std::unordered_set<uint32_t>& getReactiveVertexShaderHashes() const;
		const std::unordered_set<uint32_t>& getReactiveComputeShaderHashes() const;

		void storeReactiveWatcherHashes(
			const std::unordered_set<uint32_t>& pixelHashes,
			const std::unordered_set<uint32_t>& vertexHashes,
			const std::unordered_set<uint32_t>& computeHashes);


private:
		GroupId m_id;
		std::string m_name;
		bool m_active;
		bool m_activeAtStartup;
		bool m_editing;
		bool m_holdMode;
		bool m_holdInverted;
		bool m_timedMode;
		bool m_timedModeInverted;
		int m_timedModeDelayMs;
		int m_timedModeMinVisibleMs;
		int m_timedModeFadeOutMs;
		KeyData m_toggleKey;
		std::vector<TimedTriggerBinding> m_timedTriggerKeys;

		std::unordered_set<uint32_t> m_pixelShaderHashes;
		std::unordered_set<uint32_t> m_vertexShaderHashes;
		std::unordered_set<uint32_t> m_computeShaderHashes;
	
		bool m_reactiveTriggerEnabled = false;
		ReactiveTriggerMode m_reactiveTriggerMode = ReactiveTriggerMode::Disabled;

		std::unordered_set<uint32_t> m_reactivePixelShaderHashes;
		std::unordered_set<uint32_t> m_reactiveVertexShaderHashes;
		std::unordered_set<uint32_t> m_reactiveComputeShaderHashes;


};
}