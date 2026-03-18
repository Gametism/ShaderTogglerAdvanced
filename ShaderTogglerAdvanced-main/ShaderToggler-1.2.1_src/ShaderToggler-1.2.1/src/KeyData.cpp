///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – A shader toggler add-on for ReShade 5+
// which allows you to define groups of shaders to toggle them on/off 
// with one key press.
//
/////////////////////////////////////////////////////////////////////////

#include "KeyData.h"

namespace ShaderToggler
{
	KeyData::KeyData() : _keyCode(0), _shiftRequired(false), _altRequired(false), _ctrlRequired(false)
	{
		setKeyAsString();
	}

	void KeyData::setKeyFromIniFile(uint32_t newKeyValue)
	{
		if (newKeyValue == 0)
		{
			clear();
			return;
		}

		_keyCode = static_cast<uint8_t>((newKeyValue >> 24) & 0xFF);
		_altRequired = ((newKeyValue >> 16) & 0xFF) == 0x01;
		_ctrlRequired = ((newKeyValue >> 8) & 0xFF) == 0x01;
		_shiftRequired = (newKeyValue & 0xFF) == 0x01;
		setKeyAsString();
	}

	void KeyData::setKey(uint8_t newKeyValue, bool shiftRequired, bool altRequired, bool ctrlRequired)
	{
		if (newKeyValue == 0)
		{
			clear();
			return;
		}

		_keyCode = newKeyValue;
		_ctrlRequired = ctrlRequired;
		_shiftRequired = shiftRequired;
		_altRequired = altRequired;
		setKeyAsString();
	}

	uint32_t KeyData::getKeyForIniFile() const
	{
		return ((_keyCode & 0xFF) << 24) |
			   ((_altRequired ? 1u : 0u) << 16) |
			   ((_ctrlRequired ? 1u : 0u) << 8) |
			   (_shiftRequired ? 1u : 0u);
	}

	void KeyData::clear()
	{
		_altRequired = false;
		_ctrlRequired = false;
		_shiftRequired = false;
		_keyCode = 0;
		setKeyAsString();
	}

	void KeyData::collectKeysPressed(const reshade::api::effect_runtime* runtime)
	{
		for (int i = 7; i < 256; i++)
		{
			switch (i)
			{
			case VK_MENU:
			case VK_CONTROL:
			case VK_SHIFT:
				break;
			default:
				if (runtime->is_key_down(i))
				{
					_keyCode = static_cast<uint8_t>(i);
					_altRequired = runtime->is_key_down(VK_MENU);
					_ctrlRequired = runtime->is_key_down(VK_CONTROL);
					_shiftRequired = runtime->is_key_down(VK_SHIFT);
				}
				break;
			}
		}
		setKeyAsString();
	}

	bool KeyData::isKeyPressed(const reshade::api::effect_runtime* runtime) const
	{
		if (_keyCode == 0)
		{
			return false;
		}

		bool toReturn = runtime->is_key_pressed(_keyCode);
		const bool altPressed = runtime->is_key_down(VK_MENU);
		const bool shiftPressed = runtime->is_key_down(VK_SHIFT);
		const bool ctrlPressed = runtime->is_key_down(VK_CONTROL);

		toReturn &= ((_altRequired && altPressed) || (!_altRequired && !altPressed));
		toReturn &= ((_shiftRequired && shiftPressed) || (!_shiftRequired && !shiftPressed));
		toReturn &= ((_ctrlRequired && ctrlPressed) || (!_ctrlRequired && !ctrlPressed));
		return toReturn;
	}

	std::string KeyData::vkCodeToString(uint8_t vkCode)
	{
		static const char *keyboard_keys[256] = {
			"", "Left Mouse", "Right Mouse", "Cancel", "Middle Mouse", "X1 Mouse", "X2 Mouse", "", "Backspace", "Tab", "", "", "Clear", "Enter", "", "",
			"Shift", "Control", "Alt", "Pause", "Caps Lock", "", "", "", "", "", "", "Escape", "", "", "", "",
			"Space", "Page Up", "Page Down", "End", "Home", "Left Arrow", "Up Arrow", "Right Arrow", "Down Arrow", "Select", "", "", "Print Screen", "Insert", "Delete", "Help",
			"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "", "", "", "",
			"", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
			"P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Left Windows", "Right Windows", "Apps", "", "Sleep",
			"Numpad 0", "Numpad 1", "Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9", "Numpad *", "Numpad +", "", "Numpad -", "Numpad Decimal", "Numpad /",
			"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16",
			"F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "", "", "", "", "", "", "", "",
			"Num Lock", "Scroll Lock", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
			"Left Shift", "Right Shift", "Left Control", "Right Control", "Left Menu", "Right Menu", "Browser Back", "Browser Forward", "Browser Refresh", "Browser Stop", "Browser Search", "Browser Favorites", "Browser Home", "Volume Mute", "Volume Down", "Volume Up",
			"Next Track", "Previous Track", "Media Stop", "Media Play/Pause", "Mail", "Media Select", "Launch App 1", "Launch App 2", "", "", "OEM ;", "OEM +", "OEM ,", "OEM -", "OEM .", "OEM /",
			"OEM ~", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
			"", "", "", "", "", "", "", "", "", "", "", "OEM [", "OEM \\", "OEM ]", "OEM '", "OEM 8",
			"", "", "OEM <", "", "", "", "", "", "", "", "", "", "", "", "", "",
			"", "", "", "", "", "", "Attn", "CrSel", "ExSel", "Erase EOF", "Play", "Zoom", "", "PA1", "OEM Clear", ""
		};

		return keyboard_keys[vkCode];
	}

	void KeyData::setKeyAsString()
	{
		if (!_altRequired && !_ctrlRequired && !_shiftRequired && (_keyCode <= 0))
		{
			_keyAsString = "Press a key";
			return;
		}

		_keyAsString.clear();

		if (_altRequired) _keyAsString.append("Alt + ");
		if (_ctrlRequired) _keyAsString.append("Ctrl + ");
		if (_shiftRequired) _keyAsString.append("Shift + ");
		if (_keyCode > 0) _keyAsString.append(vkCodeToString(_keyCode));
	}

	std::string KeyData::toString() const
	{
		return _keyAsString;
	}

	int KeyData::toInt() const
	{
		return static_cast<int>(getKeyForIniFile());
	}

	KeyData KeyData::fromInt(uint32_t value)
	{
		KeyData k;
		k.setKeyFromIniFile(value);
		return k;
	}
}