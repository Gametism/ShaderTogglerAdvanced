#pragma once

#include <shared_mutex>
#include <unordered_set>
#include <cstdint>

namespace ShaderToggler
{
	class TextureManager
	{
	public:
		TextureManager();

		void startHuntingMode(const std::unordered_set<uint64_t> currentMarkedHashes);
		void stopHuntingMode();

		void huntNextTexture(bool ctrlPressed);
		void huntPreviousTexture(bool ctrlPressed);

		bool isBlockedTexture(uint64_t textureHash);

		void addActiveTextureHash(uint64_t textureHash);
		void toggleMarkOnHuntedTexture();

		uint32_t getTextureCount() const { return static_cast<uint32_t>(_textureHashes.size()); }
		uint32_t getAmountTextureHashesCollected() const { return static_cast<uint32_t>(_collectedActiveTextureHashes.size()); }
		bool isInHuntingMode() const { return _isInHuntingMode; }
		uint64_t getActiveHuntedTextureHash() const { return _activeHuntedTextureHash; }
		int getActiveHuntedTextureIndex() const { return _activeHuntedTextureIndex; }
		void toggleHideMarkedTextures() { _hideMarkedTextures = !_hideMarkedTextures; }

		bool isHuntedTextureMarked()
		{
			std::shared_lock lock(_markedTextureHashMutex);
			return _markedTextureHashes.count(_activeHuntedTextureHash) == 1;
		}

		std::unordered_set<uint64_t> getMarkedTextureHashes()
		{
			std::shared_lock lock(_markedTextureHashMutex);
			return _markedTextureHashes;
		}

		uint32_t getMarkedTextureCount()
		{
			std::shared_lock lock(_markedTextureHashMutex);
			return static_cast<uint32_t>(_markedTextureHashes.size());
		}

	private:
		void setActiveHuntedTextureHash();

		std::unordered_set<uint64_t> _textureHashes;
		std::unordered_set<uint64_t> _collectedActiveTextureHashes;
		std::unordered_set<uint64_t> _markedTextureHashes;

		bool _isInHuntingMode = false;
		int _activeHuntedTextureIndex = -1;
		uint64_t _activeHuntedTextureHash = 0;
		std::shared_mutex _collectedActiveTextureHashesMutex;
		std::shared_mutex _markedTextureHashMutex;
		bool _hideMarkedTextures = false;
	};
}