#pragma once

#include <reshade_api.hpp>
#include "stdafx.h"
//GT
namespace ShaderToggler
{
	static constexpr const char* STA_KEYDATA_OWNER_TAG = "Gametism::KeyData::Official";
	static constexpr const char* STA_KEYDATA_AUTHOR_TAG = "Sven 'Gametism' Koenigsmann";
	static constexpr int STA_KEYDATA_PROVENANCE_REV = 20260326;
//GT
	class KeyData
	{
	public:
		enum class ControllerLabelMode
		{
			Auto = 0,
			Xbox = 1,
			PlayStation = 2
		};

		enum class GlobalHotkeyModifier
		{
			None = 0,
			Ctrl = 1,
			Alt = 2,
			Shift = 3,
			CtrlAlt = 4,
			CtrlShift = 5,
			AltShift = 6,
			CtrlAltShift = 7
		};

		KeyData();

		void setKeyFromIniFile(uint32_t newKeyValue);
		void setKey(uint8_t newKeyValue, bool shiftRequired = false, bool altRequired = false, bool ctrlRequired = false);
		uint32_t getKeyForIniFile() const;
		void clear();
		void collectKeysPressed(const reshade::api::effect_runtime* runtime);

		bool isKeyDown(const reshade::api::effect_runtime* runtime) const
		{
			if (!_keyCode)
				return false;

			if (isGamepadCode(_keyCode))
				return isGamepadButtonDown(_keyCode);

			const bool altPressed = runtime->is_key_down(VK_MENU);
			const bool shiftPressed = runtime->is_key_down(VK_SHIFT);
			const bool ctrlPressed = runtime->is_key_down(VK_CONTROL);

			const bool requiredAlt = _altRequired || globalModifierRequiresAlt();
			const bool requiredCtrl = _ctrlRequired || globalModifierRequiresCtrl();
			const bool requiredShift = _shiftRequired || globalModifierRequiresShift();

			bool toReturn = runtime->is_key_down(_keyCode);
			toReturn &= ((requiredAlt && altPressed) || (!requiredAlt && !altPressed));
			toReturn &= ((requiredShift && shiftPressed) || (!requiredShift && !shiftPressed));
			toReturn &= ((requiredCtrl && ctrlPressed) || (!requiredCtrl && !ctrlPressed));
			return toReturn;
		}

		bool isKeyPressed(const reshade::api::effect_runtime* runtime) const;

		std::string getKeyAsString() const { return toString(); }
		uint8_t getKeyCode() const { return _keyCode; }
		bool isValid() const { return _keyCode > 0; }

		std::string toString() const;
		int toInt() const;
		static KeyData fromInt(uint32_t value);

		static void setControllerLabelMode(ControllerLabelMode mode);
		static ControllerLabelMode getControllerLabelMode();
		static bool isPlayStationControllerDetected();
		static void refreshControllerTypeDetection();

		static void setGlobalHotkeyModifier(GlobalHotkeyModifier modifier);
		static GlobalHotkeyModifier getGlobalHotkeyModifier();
		static const char* globalHotkeyModifierToString(GlobalHotkeyModifier modifier);
		static int globalHotkeyModifierToInt(GlobalHotkeyModifier modifier);
		static GlobalHotkeyModifier globalHotkeyModifierFromInt(int value);

		static constexpr const char* getProvenanceOwnerTag() { return STA_KEYDATA_OWNER_TAG; }
		static constexpr const char* getProvenanceAuthorTag() { return STA_KEYDATA_AUTHOR_TAG; }
		static constexpr int getProvenanceRevision() { return STA_KEYDATA_PROVENANCE_REV; }
//GT
	private:
		static std::string vkCodeToString(uint8_t vkCode);
		void setKeyAsString();

		static bool isGamepadCode(uint8_t code);
		static bool isGamepadButtonDown(uint8_t code);
		static bool isGamepadButtonPressed(uint8_t code);

		static bool detectPlayStationController();
		static bool shouldUsePlayStationLabels();

		static bool globalModifierRequiresCtrl();
		static bool globalModifierRequiresAlt();
		static bool globalModifierRequiresShift();

		uint8_t _keyCode;
		bool _shiftRequired;
		bool _altRequired;
		bool _ctrlRequired;
		std::string _keyAsString;

		static ControllerLabelMode s_controllerLabelMode;
		static bool s_cachedPlayStationDetected;
		static DWORD s_lastControllerDetectTick;
		static GlobalHotkeyModifier s_globalHotkeyModifier;
	};
}
//GT