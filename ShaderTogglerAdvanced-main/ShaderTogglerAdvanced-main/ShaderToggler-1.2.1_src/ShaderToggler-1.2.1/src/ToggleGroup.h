#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <reshade.hpp>
#include "ShaderManager.h"
#include "KeyData.h"

namespace ShaderToggler
{
	struct PassSignature
	{
		uint64_t pixelPipeline = 0;
		uint64_t renderTargetView = 0;
		bool hasViewport = false;
		float viewportX = 0.0f;
		float viewportY = 0.0f;
		float viewportWidth = 0.0f;
		float viewportHeight = 0.0f;
		uint32_t vertices = 0;
		uint32_t indices = 0;
		bool indexed = false;
	};

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

		void clearPassSignatures();
		void storeCollectedPassSignatures(const std::vector<PassSignature>& passes);
		const std::vector<PassSignature>& getPassSignatures() const;

		void loadState(class CDataFile& iniFile, int index, bool usingCustomFormat);
		void saveState(class CDataFile& iniFile, int index, bool usingCustomFormat) const;

		ToggleGroup makeDuplicate() const;

	private:
		GroupId m_id = 0;
		std::string m_name;
		bool m_active = false;
		bool m_activeAtStartup = false;
		bool m_editing = false;
		KeyData m_toggleKey;

		std::unordered_set<uint32_t> m_pixelShaderHashes;
		std::unordered_set<uint32_t> m_vertexShaderHashes;
		std::unordered_set<uint32_t> m_computeShaderHashes;

		std::vector<PassSignature> m_passSignatures;
	};
}