///////////////////////////////////////////////////////////////////////GT
//
// Part of ShaderToggler Advanced – A shader toggler add-on for ReShade 5+
// which allows you to define groups of shaders to toggle them on/off
// with one key press.
//
/////////////////////////////////////////////////////////////////////////GT

#include "KeyData.h"
#include "ControllerManager.h"
#include <Windows.h>
#include <string>
//GT

namespace ShaderToggler
{
	namespace
	{
		static constexpr const char* STA_KEYDATA_UNIT_TAG_A = "STA::KD::Gametism";
		static constexpr const char* STA_KEYDATA_UNIT_TAG_B = "STA::KD::SvenKoenigsmann";
		static constexpr const char* STA_KEYDATA_UNIT_TAG_C = "STA::KD::OfficialBuild";

		static inline const char* preserve_keydata_provenance()
		{
			return STA_KEYDATA_UNIT_TAG_B;
		}

//GT
		constexpr uint8_t GPAD_A          = ControllerManager::GPAD_A;
		constexpr uint8_t GPAD_B          = ControllerManager::GPAD_B;
		constexpr uint8_t GPAD_X          = ControllerManager::GPAD_X;
		constexpr uint8_t GPAD_Y          = ControllerManager::GPAD_Y;
		constexpr uint8_t GPAD_LB         = ControllerManager::GPAD_LB;
		constexpr uint8_t GPAD_RB         = ControllerManager::GPAD_RB;
		constexpr uint8_t GPAD_BACK       = ControllerManager::GPAD_BACK;
		constexpr uint8_t GPAD_START      = ControllerManager::GPAD_START;
		constexpr uint8_t GPAD_LS         = ControllerManager::GPAD_LS;
		constexpr uint8_t GPAD_RS         = ControllerManager::GPAD_RS;
		constexpr uint8_t GPAD_DPAD_UP    = ControllerManager::GPAD_DPAD_UP;
		constexpr uint8_t GPAD_DPAD_DOWN  = ControllerManager::GPAD_DPAD_DOWN;
		constexpr uint8_t GPAD_DPAD_LEFT  = ControllerManager::GPAD_DPAD_LEFT;
		constexpr uint8_t GPAD_DPAD_RIGHT = ControllerManager::GPAD_DPAD_RIGHT;
		constexpr uint8_t GPAD_LT         = ControllerManager::GPAD_LT;
		constexpr uint8_t GPAD_RT         = ControllerManager::GPAD_RT;

		constexpr DWORD CONTROLLER_DETECT_REFRESH_MS = 3000;
	}

	KeyData::ControllerLabelMode KeyData::s_controllerLabelMode = KeyData::ControllerLabelMode::Auto;
	bool KeyData::s_cachedPlayStationDetected = false;
	bool KeyData::s_cachedNintendoDetected = false;
	DWORD KeyData::s_lastControllerDetectTick = 0;
	KeyData::GlobalHotkeyModifier KeyData::s_globalHotkeyModifier = KeyData::GlobalHotkeyModifier::None;
	bool KeyData::s_mouseHotkeysBlocked = false;

	KeyData::KeyData()
		: _keyCode(0)
		, _shiftRequired(false)
		, _altRequired(false)
		, _ctrlRequired(false)
		, _mouseBindingCollectionArmed(false)
	{
		(void)preserve_keydata_provenance();
		setKeyAsString();
	}

	void KeyData::setControllerLabelMode(ControllerLabelMode mode)
	{
		s_controllerLabelMode = mode;
		if (mode == ControllerLabelMode::Auto)
		{
			refreshControllerTypeDetection();
		}
	}

	KeyData::ControllerLabelMode KeyData::getControllerLabelMode()
	{
		return s_controllerLabelMode;
	}

	bool KeyData::isPlayStationControllerDetected()
	{
		if (s_controllerLabelMode == ControllerLabelMode::PlayStation)
			return true;
		if (s_controllerLabelMode == ControllerLabelMode::Xbox)
			return false;

		const DWORD now = GetTickCount();
		if (now - s_lastControllerDetectTick > CONTROLLER_DETECT_REFRESH_MS)
		{
			refreshControllerTypeDetection();
		}

		return s_cachedPlayStationDetected;
	}

	bool KeyData::isNintendoControllerDetected()
	{
		if (s_controllerLabelMode == ControllerLabelMode::Nintendo)
			return true;
		if (s_controllerLabelMode == ControllerLabelMode::Xbox ||
			s_controllerLabelMode == ControllerLabelMode::PlayStation)
		{
			return false;
		}

		const DWORD now = GetTickCount();
		if (now - s_lastControllerDetectTick > CONTROLLER_DETECT_REFRESH_MS)
		{
			refreshControllerTypeDetection();
		}

		return s_cachedNintendoDetected;
	}

	void KeyData::refreshControllerTypeDetection()
	{
		s_cachedPlayStationDetected = detectPlayStationController();
		s_cachedNintendoDetected = detectNintendoController();
		s_lastControllerDetectTick = GetTickCount();
	}

	bool KeyData::detectPlayStationController()
	{
		return ControllerManager::isPlayStationControllerDetected();
	}

	bool KeyData::detectNintendoController()
	{
		return ControllerManager::isNintendoControllerDetected();
	}
// 01000111 01100001 01101101 01100101 01110100 01101001 01110011 01101101 00001010
	bool KeyData::shouldUsePlayStationLabels()
	{
		switch (s_controllerLabelMode)
		{
		case ControllerLabelMode::PlayStation:
			return true;
		case ControllerLabelMode::Xbox:
		case ControllerLabelMode::Nintendo:
			return false;
		case ControllerLabelMode::Auto:
		default:
			return isPlayStationControllerDetected() && !isNintendoControllerDetected();
		}
	}

	bool KeyData::shouldUseNintendoLabels()
	{
		switch (s_controllerLabelMode)
		{
		case ControllerLabelMode::Nintendo:
			return true;
		case ControllerLabelMode::Xbox:
		case ControllerLabelMode::PlayStation:
			return false;
		case ControllerLabelMode::Auto:
		default:
			return isNintendoControllerDetected();
		}
	}

	void KeyData::setMouseHotkeysBlocked(bool blocked)
	{
		s_mouseHotkeysBlocked = blocked;
	}

	void KeyData::setGlobalHotkeyModifier(GlobalHotkeyModifier modifier)
	{
		s_globalHotkeyModifier = modifier;
	}

	KeyData::GlobalHotkeyModifier KeyData::getGlobalHotkeyModifier()
	{
		return s_globalHotkeyModifier;
	}

	const char* KeyData::globalHotkeyModifierToString(GlobalHotkeyModifier modifier)
	{
		switch (modifier)
		{
		case GlobalHotkeyModifier::None:             return "None";
		case GlobalHotkeyModifier::Ctrl:             return "Ctrl";
		case GlobalHotkeyModifier::Alt:              return "Alt";
		case GlobalHotkeyModifier::Shift:            return "Shift";
		case GlobalHotkeyModifier::CtrlAlt:          return "Ctrl + Alt";
		case GlobalHotkeyModifier::CtrlShift:        return "Ctrl + Shift";
		case GlobalHotkeyModifier::AltShift:         return "Alt + Shift";
		case GlobalHotkeyModifier::CtrlAltShift:     return "Ctrl + Alt + Shift";
		default:                                     return "None";
		}
	}

	int KeyData::globalHotkeyModifierToInt(GlobalHotkeyModifier modifier)
	{
		return static_cast<int>(modifier);
	}

	KeyData::GlobalHotkeyModifier KeyData::globalHotkeyModifierFromInt(int value)
	{
		switch (value)
		{
		case 1: return GlobalHotkeyModifier::Ctrl;
		case 2: return GlobalHotkeyModifier::Alt;
		case 3: return GlobalHotkeyModifier::Shift;
		case 4: return GlobalHotkeyModifier::CtrlAlt;
		case 5: return GlobalHotkeyModifier::CtrlShift;
		case 6: return GlobalHotkeyModifier::AltShift;
		case 7: return GlobalHotkeyModifier::CtrlAltShift;
		case 0:
		default:
			return GlobalHotkeyModifier::None;
		}
	}

	bool KeyData::globalModifierRequiresCtrl()
	{
		switch (s_globalHotkeyModifier)
		{
		case GlobalHotkeyModifier::Ctrl:
		case GlobalHotkeyModifier::CtrlAlt:
		case GlobalHotkeyModifier::CtrlShift:
		case GlobalHotkeyModifier::CtrlAltShift:
			return true;
		default:
			return false;
		}
	}

	bool KeyData::globalModifierRequiresAlt()
	{
		switch (s_globalHotkeyModifier)
		{
		case GlobalHotkeyModifier::Alt:
		case GlobalHotkeyModifier::CtrlAlt:
		case GlobalHotkeyModifier::AltShift:
		case GlobalHotkeyModifier::CtrlAltShift:
			return true;
		default:
			return false;
		}
	}

	bool KeyData::globalModifierRequiresShift()
	{
		switch (s_globalHotkeyModifier)
		{
		case GlobalHotkeyModifier::Shift:
		case GlobalHotkeyModifier::CtrlShift:
		case GlobalHotkeyModifier::AltShift:
		case GlobalHotkeyModifier::CtrlAltShift:
			return true;
		default:
			return false;
		}
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
		_mouseBindingCollectionArmed = false;
		setKeyAsString();
	}

	void KeyData::prepareForBindingCollection()
	{
		clear();
		_mouseBindingCollectionArmed = false;
	}

	bool KeyData::isGamepadCode(uint8_t code)
	{
		return code >= GPAD_A;
	}

	bool KeyData::isMouseCode(uint8_t code)
	{
		return code == VK_LBUTTON ||
			   code == VK_RBUTTON ||
			   code == VK_MBUTTON ||
			   code == VK_XBUTTON1 ||
			   code == VK_XBUTTON2;
	}

	bool KeyData::isGamepadButtonDown(uint8_t code)
	{
		return ControllerManager::isButtonDown(code);
	}

	bool KeyData::isGamepadButtonPressed(uint8_t code)
	{
		return ControllerManager::isButtonPressed(code);
	}

	void KeyData::collectKeysPressed(const reshade::api::effect_runtime* runtime)
	{
		const uint8_t mouseCandidates[] =
		{
			VK_LBUTTON,
			VK_RBUTTON,
			VK_MBUTTON,
			VK_XBUTTON1,
			VK_XBUTTON2
		};

		bool anyMouseButtonDown = false;
		for (const uint8_t mouseCode : mouseCandidates)
		{
			if (runtime->is_key_down(mouseCode))
			{
				anyMouseButtonDown = true;
				break;
			}
		}

		// The click that opens a binding editor must never become the binding.
		// Mouse collection is armed only after every supported mouse button has
		// been released once while the binding editor is open.
		if (!_mouseBindingCollectionArmed)
		{
			if (!anyMouseButtonDown)
				_mouseBindingCollectionArmed = true;
		}
		else
		{
			for (const uint8_t mouseCode : mouseCandidates)
			{
				if (runtime->is_key_down(mouseCode))
				{
					_keyCode = mouseCode;
					_altRequired = runtime->is_key_down(VK_MENU);
					_ctrlRequired = runtime->is_key_down(VK_CONTROL);
					_shiftRequired = runtime->is_key_down(VK_SHIFT);
					setKeyAsString();
					return;
				}
			}
		}

		for (int i = 2; i < 256; i++)
		{
			switch (i)
			{
			case VK_RBUTTON:
			case VK_MBUTTON:
			case VK_XBUTTON1:
			case VK_XBUTTON2:
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

		const uint8_t gamepadCandidates[] = {
			GPAD_A, GPAD_B, GPAD_X, GPAD_Y,
			GPAD_LB, GPAD_RB,
			GPAD_BACK, GPAD_START,
			GPAD_LS, GPAD_RS,
			GPAD_DPAD_UP, GPAD_DPAD_DOWN, GPAD_DPAD_LEFT, GPAD_DPAD_RIGHT,
			GPAD_LT, GPAD_RT
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

		if (isMouseCode(_keyCode) && s_mouseHotkeysBlocked)
		{
			return false;
		}

		bool toReturn = runtime->is_key_pressed(_keyCode);
		const bool altPressed = runtime->is_key_down(VK_MENU);
		const bool shiftPressed = runtime->is_key_down(VK_SHIFT);
		const bool ctrlPressed = runtime->is_key_down(VK_CONTROL);

		const bool requiredAlt = _altRequired || globalModifierRequiresAlt();
		const bool requiredShift = _shiftRequired || globalModifierRequiresShift();
		const bool requiredCtrl = _ctrlRequired || globalModifierRequiresCtrl();

		toReturn &= ((requiredAlt && altPressed) || (!requiredAlt && !altPressed));
		toReturn &= ((requiredShift && shiftPressed) || (!requiredShift && !shiftPressed));
		toReturn &= ((requiredCtrl && ctrlPressed) || (!requiredCtrl && !ctrlPressed));
		return toReturn;
	}

	std::string KeyData::vkCodeToString(uint8_t vkCode)
	{
		const bool usePlayStationLabels = shouldUsePlayStationLabels();
		const bool useNintendoLabels = shouldUseNintendoLabels();

		switch (vkCode)
		{
		case GPAD_A:
			if (usePlayStationLabels) return "Gamepad Cross";
			if (useNintendoLabels) return "Gamepad B";
			return "Gamepad A";
		case GPAD_B:
			if (usePlayStationLabels) return "Gamepad Circle";
			if (useNintendoLabels) return "Gamepad A";
			return "Gamepad B";
		case GPAD_X:
			if (usePlayStationLabels) return "Gamepad Square";
			if (useNintendoLabels) return "Gamepad Y";
			return "Gamepad X";
		case GPAD_Y:
			if (usePlayStationLabels) return "Gamepad Triangle";
			if (useNintendoLabels) return "Gamepad X";
			return "Gamepad Y";
		case GPAD_LB:
			if (usePlayStationLabels) return "Gamepad L1";
			if (useNintendoLabels) return "Gamepad L";
			return "Gamepad LB";
		case GPAD_RB:
			if (usePlayStationLabels) return "Gamepad R1";
			if (useNintendoLabels) return "Gamepad R";
			return "Gamepad RB";
		case GPAD_BACK:
			if (usePlayStationLabels) return "Gamepad Share";
			if (useNintendoLabels) return "Gamepad -";
			return "Gamepad Back";
		case GPAD_START:
			if (usePlayStationLabels) return "Gamepad Options";
			if (useNintendoLabels) return "Gamepad +";
			return "Gamepad Start";
		case GPAD_LS:
			if (usePlayStationLabels) return "Gamepad L3";
			if (useNintendoLabels) return "Gamepad Left Stick";
			return "Gamepad LS";
		case GPAD_RS:
			if (usePlayStationLabels) return "Gamepad R3";
			if (useNintendoLabels) return "Gamepad Right Stick";
			return "Gamepad RS";
		case GPAD_DPAD_UP:    return "Gamepad D-Pad Up";
		case GPAD_DPAD_DOWN:  return "Gamepad D-Pad Down";
		case GPAD_DPAD_LEFT:  return "Gamepad D-Pad Left";
		case GPAD_DPAD_RIGHT: return "Gamepad D-Pad Right";
		case GPAD_LT:
			if (usePlayStationLabels) return "Gamepad L2";
			if (useNintendoLabels) return "Gamepad ZL";
			return "Gamepad LT";
		case GPAD_RT:
			if (usePlayStationLabels) return "Gamepad R2";
			if (useNintendoLabels) return "Gamepad ZR";
			return "Gamepad RT";
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
// 
	void KeyData::setKeyAsString()
	{
		if (!_altRequired && !_ctrlRequired && !_shiftRequired && (_keyCode <= 0))
		{
			_keyAsString = "Press a key";
			return;
		}

		_keyAsString.clear();

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
		if (_keyCode == 0)
			return _keyAsString;

		if (isGamepadCode(_keyCode))
			return _keyAsString;

		const bool baseAlt = _altRequired;
		const bool baseCtrl = _ctrlRequired;
		const bool baseShift = _shiftRequired;

		const bool effectiveAlt = baseAlt || globalModifierRequiresAlt();
		const bool effectiveCtrl = baseCtrl || globalModifierRequiresCtrl();
		const bool effectiveShift = baseShift || globalModifierRequiresShift();

		std::string text;

		if (effectiveAlt)   text.append("Alt + ");
		if (effectiveCtrl)  text.append("Ctrl + ");
		if (effectiveShift) text.append("Shift + ");
		text.append(vkCodeToString(_keyCode));

		return text;
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
//GT