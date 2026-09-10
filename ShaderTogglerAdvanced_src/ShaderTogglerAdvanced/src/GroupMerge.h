// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#pragma once
#include "ToggleGroup.h"
#include <optional>
#include <set>
namespace ShaderToggler
{
    enum class GroupMergeError { None, NoSources, MissingGroup, InvalidFilter, TooManyFilters };
    struct GroupMergeResult
    {
        std::optional<ToggleGroup> group;
        GroupMergeError error=GroupMergeError::None;
        size_t filterCount=0;
    };
    inline GroupMergeResult prepareGroupMerge(const std::vector<ToggleGroup>& groups,
        int destination,const std::set<int>& sources)
    {
        GroupMergeResult result;
        const auto target=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return g.getId()==destination;});
        if(target==groups.end()) { result.error=GroupMergeError::MissingGroup;return result; }
        if(sources.empty() || sources.count(destination)) { result.error=GroupMergeError::NoSources;return result; }
        for(int id:sources)
            if(std::none_of(groups.begin(),groups.end(),[&](const auto& g){return g.getId()==id;}))
            { result.error=GroupMergeError::MissingGroup;return result; }
        auto pixels=target->getPixelShaderHashes();
        auto vertices=target->getVertexShaderHashes();
        auto computes=target->getComputeShaderHashes();
        std::set<finder::Signature> unique;
        std::vector<finder::Signature> additions;
        for(const auto& rule:target->getEffectFilters())
        {
            if(!finder::valid(rule)) { result.error=GroupMergeError::InvalidFilter;return result; }
            unique.insert(rule);
        }
        for(const auto& source:groups)
        {
            if(!sources.count(source.getId())) continue;
            pixels.insert(source.getPixelShaderHashes().begin(),source.getPixelShaderHashes().end());
            vertices.insert(source.getVertexShaderHashes().begin(),source.getVertexShaderHashes().end());
            computes.insert(source.getComputeShaderHashes().begin(),source.getComputeShaderHashes().end());
            for(const auto& rule:source.getEffectFilters())
            {
                if(!finder::valid(rule)) { result.error=GroupMergeError::InvalidFilter;return result; }
                if(unique.insert(rule).second) additions.push_back(rule);
            }
        }
        result.filterCount=unique.size();
        if(result.filterCount>finder::maxRules) { result.error=GroupMergeError::TooManyFilters;return result; }
        result.group=*target;
        result.group->storeCollectedHashes(pixels,vertices,computes);
        for(const auto& rule:additions)
            if(!result.group->addEffectFilter(rule))
            { result.group.reset();result.error=GroupMergeError::InvalidFilter;return result; }
        return result;
    }
}
