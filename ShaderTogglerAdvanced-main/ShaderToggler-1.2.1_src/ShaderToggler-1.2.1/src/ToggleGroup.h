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