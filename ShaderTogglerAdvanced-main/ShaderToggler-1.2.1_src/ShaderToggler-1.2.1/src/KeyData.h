#pragma once

#include <reshade_api.hpp>
#include "stdafx.h"

namespace ShaderToggler
{
	class KeyData
	{
	public:
		KeyData();

		void setKeyFromIniFile(uint32_t newKeyValue);
		void setKey(uint8_t newKeyValue, bool shiftRequired = false, bool altRequired = false, bool ctrlRequired = false);
		uint32_t getKeyForIniFile() const;
		void clear();
		void collectKeysPressed(const reshade::api::effect_runtime* runtime);

		bool isKeyDown(const reshade::api::effect_runtime* runtime)
		{
			if (!_keyCode) return false;
			const bool altPressed = runtime->is_key_down(VK_MENU);
			const bool shiftPressed = runtime->is_key_down(VK_SHIFT);
			const bool ctrlPressed = runtime->is_key_down(VK_CONTROL);

			bool toReturn = runtime->is_key_down(_keyCode);
			toReturn &= ((_altRequired && altPressed) || (!_altRequired && !altPressed));
			toReturn &= ((_shiftRequired && shiftPressed) || (!_shiftRequired && !shiftPressed));
			toReturn &= ((_ctrlRequired && ctrlPressed) || (!_ctrlRequired && !ctrlPressed));
			return toReturn;
		}

		bool isKeyPressed(const reshade::api::effect_runtime* runtime) const;

		std::string getKeyAsString() { return _keyAsString; }
		uint8_t getKeyCode() { return _keyCode; }
		bool isValid() { return _keyCode > 0; }

		std::string toString() const;
		int toInt() const;
		static KeyData fromInt(uint32_t value);

	private:
		static std::string vkCodeToString(uint8_t vkCode);
		void setKeyAsString();

		uint8_t _keyCode;
		bool _shiftRequired;
		bool _altRequired;
		bool _ctrlRequired;
		std::string _keyAsString;
	};
}