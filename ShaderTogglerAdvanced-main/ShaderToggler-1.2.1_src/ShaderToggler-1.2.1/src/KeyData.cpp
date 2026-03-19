///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – A shader toggler add-on for ReShade 5+
// which allows you to define groups of shaders to toggle them on/off 
// with one key press.
//
/////////////////////////////////////////////////////////////////////////

#include "KeyData.h"
#include <Xinput.h>

#pragma comment(lib, "Xinput9_1_0.lib")

namespace ShaderToggler
{
	namespace
	{
		// Custom codes stored in the normal 8-bit key slot, chosen from a high unused range
		constexpr uint8_t GPAD_A          = 240;
		constexpr uint8_t GPAD_B          = 241;
		constexpr uint8_t GPAD_X          = 242;
		constexpr uint8_t GPAD_Y          = 243;
		constexpr uint8_t GPAD_LB         = 244;
		constexpr uint8_t GPAD_RB         = 245;
		constexpr uint8_t GPAD_BACK       = 246;
		constexpr uint8_t GPAD_START      = 247;
		constexpr uint8_t GPAD_LS         = 248;
		constexpr uint8_t GPAD_RS         = 249;
		constexpr uint8_t GPAD_DPAD_UP    = 250;
		constexpr uint8_t GPAD_DPAD_DOWN  = 251;
		constexpr uint8_t GPAD_DPAD_LEFT  = 252;
		constexpr uint8_t GPAD_DPAD_RIGHT = 253;

		static bool pollGamepadButtons(WORD& previousButtons, WORD& currentButtons)
		{
			static DWORD s_lastPollTick = 0;
			static WORD s_prevButtons = 0;
			static WORD s_currButtons = 0;

			const DWORD nowTick = GetTickCount();
			if (nowTick != s_lastPollTick)
			{
				s_prevButtons = s_currButtons;

				XINPUT_STATE state = {};
				if (XInputGetState(0, &state) == ERROR_SUCCESS)
				{
					s_currButtons = state.Gamepad.wButtons;
				}
				else
				{
					s_currButtons = 0;
				}

				s_lastPollTick = nowTick;
			}

			previousButtons = s_prevButtons;
			currentButtons = s_currButtons;
			return true;
		}

		static WORD gamepadCodeToButtonMask(uint8_t code)
		{
			switch (code)
			{
			case GPAD_A:          return XINPUT_GAMEPAD_A;
			case GPAD_B:          return XINPUT_GAMEPAD_B;
			case GPAD_X:          return XINPUT_GAMEPAD_X;
			case GPAD_Y:          return XINPUT_GAMEPAD_Y;
			case GPAD_LB:         return XINPUT_GAMEPAD_LEFT_SHOULDER;
			case GPAD_RB:         return XINPUT_GAMEPAD_RIGHT_SHOULDER;
			case GPAD_BACK:       return XINPUT_GAMEPAD_BACK;
			case GPAD_START:      return XINPUT_GAMEPAD_START;
			case GPAD_LS:         return XINPUT_GAMEPAD_LEFT_THUMB;
			case GPAD_RS:         return XINPUT_GAMEPAD_RIGHT_THUMB;
			case GPAD_DPAD_UP:    return XINPUT_GAMEPAD_DPAD_UP;
			case GPAD_DPAD_DOWN:  return XINPUT_GAMEPAD_DPAD_DOWN;
			case GPAD_DPAD_LEFT:  return XINPUT_GAMEPAD_DPAD_LEFT;
			case GPAD_DPAD_RIGHT: return XINPUT_GAMEPAD_DPAD_RIGHT;
			default:              return 0;
			}
		}
	}

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

	bool KeyData::isGamepadCode(uint8_t code)
	{
		return code >= GPAD_A && code <= GPAD_DPAD_RIGHT;
	}

	bool KeyData::isGamepadButtonDown(uint8_t code)
	{
		const WORD mask = gamepadCodeToButtonMask(code);
		if (mask == 0)
			return false;

		WORD prevButtons = 0;
		WORD currButtons = 0;
		pollGamepadButtons(prevButtons, currButtons);
		return (currButtons & mask) != 0;
	}

	bool KeyData::isGamepadButtonPressed(uint8_t code)
	{
		const WORD mask = gamepadCodeToButtonMask(code);
		if (mask == 0)
			return false;

		WORD prevButtons = 0;
		WORD currButtons = 0;
		pollGamepadButtons(prevButtons, currButtons);

		return ((currButtons & mask) != 0) && ((prevButtons & mask) == 0);
	}

	void KeyData::collectKeysPressed(const reshade::api::effect_runtime* runtime)
	{
		// Keyboard + mouse
		for (int i = 1; i < 256; i++)
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
					setKeyAsString();
					return;
				}
				break;
			}
		}

		// Gamepad (modifiers do not apply)
		const uint8_t gamepadCandidates[] = {
			GPAD_A, GPAD_B, GPAD_X, GPAD_Y,
			GPAD_LB, GPAD_RB,
			GPAD_BACK, GPAD_START,
			GPAD_LS, GPAD_RS,
			GPAD_DPAD_UP, GPAD_DPAD_DOWN, GPAD_DPAD_LEFT, GPAD_DPAD_RIGHT
		};

		for (uint8_t code : gamepadCandidates)
		{
			if (isGamepadButtonDown(code))
			{
				_keyCode = code;
				_altRequired = false;
				_ctrlRequired = false;
				_shiftRequired = false;
				setKeyAsString();
				return;
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

		if (isGamepadCode(_keyCode))
		{
			return isGamepadButtonPressed(_keyCode);
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
		switch (vkCode)
		{
		case GPAD_A:          return "Gamepad A";
		case GPAD_B:          return "Gamepad B";
		case GPAD_X:          return "Gamepad X";
		case GPAD_Y:          return "Gamepad Y";
		case GPAD_LB:         return "Gamepad LB";
		case GPAD_RB:         return "Gamepad RB";
		case GPAD_BACK:       return "Gamepad Back";
		case GPAD_START:      return "Gamepad Start";
		case GPAD_LS:         return "Gamepad LS";
		case GPAD_RS:         return "Gamepad RS";
		case GPAD_DPAD_UP:    return "Gamepad D-Pad Up";
		case GPAD_DPAD_DOWN:  return "Gamepad D-Pad Down";
		case GPAD_DPAD_LEFT:  return "Gamepad D-Pad Left";
		case GPAD_DPAD_RIGHT: return "Gamepad D-Pad Right";
		default:
			break;
		}

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

		// Do not prefix gamepad buttons with keyboard modifiers
		if (!isGamepadCode(_keyCode))
		{
			if (_altRequired)   _keyAsString.append("Alt + ");
			if (_ctrlRequired)  _keyAsString.append("Ctrl + ");
			if (_shiftRequired) _keyAsString.append("Shift + ");
		}

		if (_keyCode > 0)
			_keyAsString.append(vkCodeToString(_keyCode));
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