#include "TextureManager.h"

namespace ShaderToggler
{
	TextureManager::TextureManager()
	{
	}

	void TextureManager::startHuntingMode(const std::unordered_set<uint64_t> currentMarkedHashes)
	{
		{
			std::unique_lock lock(_markedTextureHashMutex);
			_markedTextureHashes.clear();
			for (const auto hash : currentMarkedHashes)
			{
				_markedTextureHashes.emplace(hash);
			}
		}

		_isInHuntingMode = true;
		_activeHuntedTextureIndex = -1;
		_activeHuntedTextureHash = 0;
		_hideMarkedTextures = false;

		{
			std::unique_lock lock(_collectedActiveTextureHashesMutex);
			_collectedActiveTextureHashes.clear();
		}
	}

	void TextureManager::stopHuntingMode()
	{
		_isInHuntingMode = false;
		_activeHuntedTextureIndex = -1;
		_activeHuntedTextureHash = 0;
		_hideMarkedTextures = false;

		{
			std::unique_lock lock(_markedTextureHashMutex);
			_markedTextureHashes.clear();
		}
	}

	void TextureManager::setActiveHuntedTextureHash()
	{
		if (_activeHuntedTextureIndex < 0 || _collectedActiveTextureHashes.empty() || _activeHuntedTextureIndex >= static_cast<int>(_collectedActiveTextureHashes.size()))
		{
			_activeHuntedTextureHash = 0;
			return;
		}

		auto it = _collectedActiveTextureHashes.begin();
		std::advance(it, _activeHuntedTextureIndex);
		_activeHuntedTextureHash = *it;
	}

	void TextureManager::huntNextTexture(bool ctrlPressed)
	{
		if (!_isInHuntingMode || _collectedActiveTextureHashes.empty())
		{
			return;
		}

		if (ctrlPressed)
		{
			if (_markedTextureHashes.empty() || (_markedTextureHashes.size() == 1 && _markedTextureHashes.count(_activeHuntedTextureHash) == 1))
			{
				return;
			}

			auto it = _collectedActiveTextureHashes.begin();
			int index = _activeHuntedTextureIndex + 1;
			if (index >= static_cast<int>(_collectedActiveTextureHashes.size()))
				index = 0;

			std::advance(it, index);
			bool foundHash = false;
			uint64_t hash = 0;

			while (index != _activeHuntedTextureIndex)
			{
				if (it == _collectedActiveTextureHashes.end())
				{
					it = _collectedActiveTextureHashes.begin();
					index = 0;
				}

				hash = *it;
				if (_markedTextureHashes.count(hash) == 1)
				{
					foundHash = true;
					break;
				}

				++it;
				index++;
				if (index >= static_cast<int>(_collectedActiveTextureHashes.size()))
					index = 0;
			}

			if (foundHash)
			{
				_activeHuntedTextureIndex = index;
				_activeHuntedTextureHash = hash;
			}
			return;
		}

		if (_activeHuntedTextureIndex < static_cast<int>(_collectedActiveTextureHashes.size()) - 1)
		{
			_activeHuntedTextureIndex++;
		}
		else
		{
			_activeHuntedTextureIndex = 0;
		}
		setActiveHuntedTextureHash();
	}

	void TextureManager::huntPreviousTexture(bool ctrlPressed)
	{
		if (!_isInHuntingMode || _collectedActiveTextureHashes.empty())
		{
			return;
		}

		if (ctrlPressed)
		{
			if (_markedTextureHashes.empty() || (_markedTextureHashes.size() == 1 && _markedTextureHashes.count(_activeHuntedTextureHash) == 1))
			{
				return;
			}

			int index = _activeHuntedTextureIndex - 1;
			if (index < 0)
				index = static_cast<int>(_collectedActiveTextureHashes.size()) - 1;

			auto it = _collectedActiveTextureHashes.begin();
			std::advance(it, index);

			bool foundHash = false;
			uint64_t hash = 0;

			while (index != _activeHuntedTextureIndex)
			{
				hash = *it;
				if (_markedTextureHashes.count(hash) == 1)
				{
					foundHash = true;
					break;
				}

				if (it == _collectedActiveTextureHashes.begin())
				{
					it = _collectedActiveTextureHashes.end();
				}
				--it;

				index--;
				if (index < 0)
					index = static_cast<int>(_collectedActiveTextureHashes.size()) - 1;
			}

			if (foundHash)
			{
				_activeHuntedTextureIndex = index;
				_activeHuntedTextureHash = hash;
			}
			return;
		}

		if (_activeHuntedTextureIndex <= 0)
		{
			_activeHuntedTextureIndex = static_cast<int>(_collectedActiveTextureHashes.size()) - 1;
		}
		else
		{
			--_activeHuntedTextureIndex;
		}
		setActiveHuntedTextureHash();
	}

	bool TextureManager::isBlockedTexture(uint64_t textureHash)
	{
		bool toReturn = false;
		if (_isInHuntingMode)
		{
			toReturn |= textureHash > 0 && _activeHuntedTextureHash == textureHash;
		}
		if (_hideMarkedTextures)
		{
			toReturn |= _markedTextureHashes.count(textureHash) == 1;
		}
		return toReturn;
	}

	void TextureManager::addActiveTextureHash(uint64_t textureHash)
	{
		if (textureHash == 0)
		{
			return;
		}

		std::unique_lock lock(_collectedActiveTextureHashesMutex);
		_collectedActiveTextureHashes.emplace(textureHash);
		_textureHashes.emplace(textureHash);
	}

	void TextureManager::toggleMarkOnHuntedTexture()
	{
		if (_activeHuntedTextureHash == 0)
		{
			return;
		}

		std::unique_lock lock(_markedTextureHashMutex);
		if (_markedTextureHashes.count(_activeHuntedTextureHash) == 1)
		{
			_markedTextureHashes.erase(_activeHuntedTextureHash);
		}
		else
		{
			_markedTextureHashes.emplace(_activeHuntedTextureHash);
		}
	}
}