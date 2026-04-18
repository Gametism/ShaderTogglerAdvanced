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

#include "ShaderManager.h"

using namespace reshade::api;

namespace ShaderToggler
{
	ShaderManager::ShaderManager() : _activeHuntedShaderHash(0)
	{
	}

	void ShaderManager::addHashHandlePair(uint32_t shaderHash, uint64_t pipelineHandle)
	{
		if (pipelineHandle > 0 && shaderHash > 0)
		{
			std::unique_lock lock(_hashHandlesMutex);
			_handleToShaderHash[pipelineHandle] = shaderHash;
			_shaderHashes.emplace(shaderHash);
		}
	}

	void ShaderManager::removeHandle(uint64_t handle)
	{
		uint32_t shaderHash = 0;
		bool stillReferencedByAnotherHandle = false;

		{
			std::unique_lock lock(_hashHandlesMutex);

			const auto it = _handleToShaderHash.find(handle);
			if (it == _handleToShaderHash.end())
			{
				return;
			}

			shaderHash = it->second;
			_handleToShaderHash.erase(it);

			for (const auto& pair : _handleToShaderHash)
			{
				if (pair.second == shaderHash)
				{
					stillReferencedByAnotherHandle = true;
					break;
				}
			}

			if (!stillReferencedByAnotherHandle)
			{
				_shaderHashes.erase(shaderHash);
			}
		}

		if (!stillReferencedByAnotherHandle && shaderHash > 0)
		{
			std::unique_lock collectedLock(_collectedActiveHandlesMutex);

			_collectedActiveShaderHashes.erase(shaderHash);

			for (auto it = _collectedActiveShaderHashesOrdered.begin(); it != _collectedActiveShaderHashesOrdered.end();)
			{
				if (*it == shaderHash)
				{
					it = _collectedActiveShaderHashesOrdered.erase(it);
				}
				else
				{
					++it;
				}
			}

			if (_collectedActiveShaderHashesOrdered.empty())
			{
				_activeHuntedShaderIndex = -1;
				_activeHuntedShaderHash = 0;
			}
			else
			{
				if (_activeHuntedShaderIndex >= static_cast<int>(_collectedActiveShaderHashesOrdered.size()))
				{
					_activeHuntedShaderIndex = static_cast<int>(_collectedActiveShaderHashesOrdered.size()) - 1;
				}

				if (_activeHuntedShaderIndex < 0)
				{
					_activeHuntedShaderIndex = 0;
				}

				_activeHuntedShaderHash = _collectedActiveShaderHashesOrdered[static_cast<size_t>(_activeHuntedShaderIndex)];
			}
		}
	}

	void ShaderManager::startHuntingMode(const std::unordered_set<uint32_t> currentMarkedHashes)
	{
		{
			std::unique_lock lock(_markedShaderHashMutex);
			_markedShaderHashes.clear();
			for (const auto hash : currentMarkedHashes)
			{
				_markedShaderHashes.emplace(hash);
			}
		}

		_isInHuntingMode = true;
		_activeHuntedShaderIndex = -1;
		_activeHuntedShaderHash = 0;

		{
			std::unique_lock lock(_collectedActiveHandlesMutex);
			_collectedActiveShaderHashes.clear();
			_collectedActiveShaderHashesOrdered.clear();
		}
	}

	void ShaderManager::stopHuntingMode()
	{
		_isInHuntingMode = false;
		_activeHuntedShaderIndex = -1;
		_activeHuntedShaderHash = 0;

		{
			std::unique_lock lock(_markedShaderHashMutex);
			_markedShaderHashes.clear();
		}
	}

	void ShaderManager::setActiveHuntedShaderHandle()
	{
		std::shared_lock lock(_collectedActiveHandlesMutex);

		if (_activeHuntedShaderIndex < 0 ||
			_collectedActiveShaderHashesOrdered.empty() ||
			_activeHuntedShaderIndex >= static_cast<int>(_collectedActiveShaderHashesOrdered.size()))
		{
			_activeHuntedShaderHash = 0;
			return;
		}

		_activeHuntedShaderHash = _collectedActiveShaderHashesOrdered[static_cast<size_t>(_activeHuntedShaderIndex)];
	}

	void ShaderManager::huntNextShader(bool ctrlPressed)
	{
		if (!_isInHuntingMode)
		{
			return;
		}

		std::shared_lock collectedLock(_collectedActiveHandlesMutex);
		if (_collectedActiveShaderHashesOrdered.empty())
		{
			return;
		}

		const int collectedCount = static_cast<int>(_collectedActiveShaderHashesOrdered.size());

		if (ctrlPressed)
		{
			std::shared_lock markedLock(_markedShaderHashMutex);

			if (_markedShaderHashes.empty() ||
				(_markedShaderHashes.size() == 1 && _markedShaderHashes.count(_activeHuntedShaderHash) == 1))
			{
				return;
			}

			int startIndex = _activeHuntedShaderIndex;
			if (startIndex < 0 || startIndex >= collectedCount)
			{
				startIndex = -1;
			}

			int index = startIndex;
			for (int step = 0; step < collectedCount; ++step)
			{
				index = (index + 1) % collectedCount;
				const uint32_t hash = _collectedActiveShaderHashesOrdered[static_cast<size_t>(index)];
				if (_markedShaderHashes.count(hash) == 1)
				{
					_activeHuntedShaderIndex = index;
					_activeHuntedShaderHash = hash;
					return;
				}
			}

			return;
		}

		if (_activeHuntedShaderIndex < collectedCount - 1)
		{
			_activeHuntedShaderIndex++;
		}
		else
		{
			_activeHuntedShaderIndex = 0;
		}

		_activeHuntedShaderHash = _collectedActiveShaderHashesOrdered[static_cast<size_t>(_activeHuntedShaderIndex)];
	}

	void ShaderManager::huntPreviousShader(bool ctrlPressed)
	{
		if (!_isInHuntingMode)
		{
			return;
		}

		std::shared_lock collectedLock(_collectedActiveHandlesMutex);
		if (_collectedActiveShaderHashesOrdered.empty())
		{
			return;
		}

		const int collectedCount = static_cast<int>(_collectedActiveShaderHashesOrdered.size());

		if (ctrlPressed)
		{
			std::shared_lock markedLock(_markedShaderHashMutex);

			if (_markedShaderHashes.empty() ||
				(_markedShaderHashes.size() == 1 && _markedShaderHashes.count(_activeHuntedShaderHash) == 1))
			{
				return;
			}

			int startIndex = _activeHuntedShaderIndex;
			if (startIndex < 0 || startIndex >= collectedCount)
			{
				startIndex = 0;
			}

			int index = startIndex;
			for (int step = 0; step < collectedCount; ++step)
			{
				index = (index - 1 + collectedCount) % collectedCount;
				const uint32_t hash = _collectedActiveShaderHashesOrdered[static_cast<size_t>(index)];
				if (_markedShaderHashes.count(hash) == 1)
				{
					_activeHuntedShaderIndex = index;
					_activeHuntedShaderHash = hash;
					return;
				}
			}

			return;
		}

		if (_activeHuntedShaderIndex <= 0)
		{
			_activeHuntedShaderIndex = collectedCount - 1;
		}
		else
		{
			--_activeHuntedShaderIndex;
		}

		_activeHuntedShaderHash = _collectedActiveShaderHashesOrdered[static_cast<size_t>(_activeHuntedShaderIndex)];
	}

	bool ShaderManager::isBlockedShader(uint32_t shaderHash)
	{
		bool toReturn = false;

		if (_isInHuntingMode)
		{
			toReturn |= shaderHash <= 0 ? false : _activeHuntedShaderHash == shaderHash;
		}

		if (_hideMarkedShaders)
		{
			std::shared_lock lock(_markedShaderHashMutex);
			toReturn |= _markedShaderHashes.count(shaderHash) == 1;
		}

		return toReturn;
	}

	void ShaderManager::addActivePipelineHandle(uint64_t handle)
	{
		const auto shaderHash = getShaderHash(handle);
		if (shaderHash > 0)
		{
			std::unique_lock lock(_collectedActiveHandlesMutex);

			if (_collectedActiveShaderHashes.emplace(shaderHash).second)
			{
				_collectedActiveShaderHashesOrdered.push_back(shaderHash);

				if (_activeHuntedShaderIndex < 0)
				{
					_activeHuntedShaderIndex = 0;
					_activeHuntedShaderHash = _collectedActiveShaderHashesOrdered[0];
				}
			}
		}
	}

	void ShaderManager::toggleMarkOnHuntedShader()
	{
		if (_activeHuntedShaderHash <= 0)
		{
			return;
		}

		std::unique_lock lock(_markedShaderHashMutex);
		if (_markedShaderHashes.count(_activeHuntedShaderHash) == 1)
		{
			_markedShaderHashes.erase(_activeHuntedShaderHash);
		}
		else
		{
			_markedShaderHashes.emplace(_activeHuntedShaderHash);
		}
	}

	uint32_t ShaderManager::getShaderHash(uint64_t handle)
	{
		std::shared_lock lock(_hashHandlesMutex);

		const auto it = _handleToShaderHash.find(handle);
		if (it == _handleToShaderHash.end())
		{
			return 0;
		}

		return it->second;
	}
}