///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – A shader toggler add-on for ReShade 5+
// which allows you to define groups of shaders to toggle them on/off 
// with one key press.
//
// Based on the original ShaderToggler by Frans 'Otis_Inf' Bouma.
// (c) Frans 'Otis_Inf' Bouma. All rights reserved.
//
// https://github.com/FransBouma/ShaderToggler
//
// Modifications
// (c) 2026 Sven 'Gametism' Koenigsmann. All rights reserved.
// 
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notices,
//    this list of conditions, and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notices,
//    this list of conditions, and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <vector>
#include <reshade_api_device.hpp>
#include <reshade_api_pipeline.hpp>
#include <shared_mutex>
#include <unordered_set>

#include "CDataFile.h"
#include "ToggleGroup.h"

namespace ShaderToggler
{
	class ShaderManager
	{
	public:
		ShaderManager();

		void addHashHandlePair(uint32_t shaderHash, uint64_t pipelineHandle);
		void removeHandle(uint64_t handle);

		void startHuntingMode(const std::unordered_set<uint32_t> currentMarkedHashes);
		void stopHuntingMode();

		void huntNextShader(bool ctrlPressed);

		void huntPreviousShader(bool ctrlPressed);

		bool isBlockedShader(uint32_t shaderHash);

		uint32_t getShaderHash(uint64_t handle);

		void addActivePipelineHandle(uint64_t handle);
		void toggleMarkOnHuntedShader();

		uint32_t getPipelineCount()
		{
			std::shared_lock lock(_hashHandlesMutex);
			return static_cast<uint32_t>(_handleToShaderHash.size());
		}

		uint32_t getShaderCount()
		{
			std::shared_lock lock(_hashHandlesMutex);
			return static_cast<uint32_t>(_shaderHashes.size());
		}

		uint32_t getAmountShaderHashesCollected()
		{
			std::shared_lock lock(_collectedActiveHandlesMutex);
			return static_cast<uint32_t>(_collectedActiveShaderHashesOrdered.size());
		}

		bool isInHuntingMode() { return _isInHuntingMode; }
		uint32_t getActiveHuntedShaderHash() { return _activeHuntedShaderHash; }
		int getActiveHuntedShaderIndex() { return _activeHuntedShaderIndex; }
		void toggleHideMarkedShaders() { _hideMarkedShaders = !_hideMarkedShaders; }

		bool isHuntedShaderMarked()
		{
			std::shared_lock lock(_markedShaderHashMutex);
			return _markedShaderHashes.count(_activeHuntedShaderHash) == 1;
		}

		std::unordered_set<uint32_t> getMarkedShaderHashes()
		{
			std::shared_lock lock(_markedShaderHashMutex);
			return _markedShaderHashes;
		}

		uint32_t getMarkedShaderCount()
		{
			std::shared_lock lock(_markedShaderHashMutex);
			return static_cast<uint32_t>(_markedShaderHashes.size());
		}

		bool isKnownHandle(uint64_t pipelineHandle)
		{
			std::shared_lock lock(_hashHandlesMutex);
			return _handleToShaderHash.count(pipelineHandle) == 1;
		}
		
	private:
		void setActiveHuntedShaderHandle();
		void rebuildHuntSnapshotLocked();
		void syncActiveHuntedShaderToSnapshotLocked();

		std::unordered_set<uint32_t> _shaderHashes;					
		std::map<uint64_t, uint32_t> _handleToShaderHash;			

		std::unordered_set<uint32_t> _collectedActiveShaderHashes;	
		std::vector<uint32_t> _collectedActiveShaderHashesOrdered;	
		std::vector<uint32_t> _huntShaderHashesSnapshot;			

		std::unordered_set<uint32_t> _markedShaderHashes;			

		bool _isInHuntingMode = false;
		int _activeHuntedShaderIndex = -1;
		uint32_t _activeHuntedShaderHash;
		std::shared_mutex _collectedActiveHandlesMutex;
		std::shared_mutex _hashHandlesMutex;
		std::shared_mutex _markedShaderHashMutex;
		bool _hideMarkedShaders = false;
	};
}