#pragma once

#include <Windows.h>
#include <Xinput.h>
#include <cfgmgr32.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#pragma comment(lib, "Cfgmgr32.lib")
#pragma comment(lib, "Hid.lib")
#pragma comment(lib, "Setupapi.lib")

namespace ShaderToggler
{
	class ControllerManager
	{
	public:
		enum class ControllerType
		{
			None = 0,
			Xbox = 1,
			PlayStation = 2,
			Nintendo = 3
		};

		// Existing values are intentionally preserved for full INI compatibility.
		static constexpr uint8_t GPAD_A          = 240;
		static constexpr uint8_t GPAD_B          = 241;
		static constexpr uint8_t GPAD_X          = 242;
		static constexpr uint8_t GPAD_Y          = 243;
		static constexpr uint8_t GPAD_LB         = 244;
		static constexpr uint8_t GPAD_RB         = 245;
		static constexpr uint8_t GPAD_BACK       = 246;
		static constexpr uint8_t GPAD_START      = 247;
		static constexpr uint8_t GPAD_LS         = 248;
		static constexpr uint8_t GPAD_RS         = 249;
		static constexpr uint8_t GPAD_DPAD_UP    = 250;
		static constexpr uint8_t GPAD_DPAD_DOWN  = 251;
		static constexpr uint8_t GPAD_DPAD_LEFT  = 252;
		static constexpr uint8_t GPAD_DPAD_RIGHT = 253;
		static constexpr uint8_t GPAD_LT         = 254;
		static constexpr uint8_t GPAD_RT         = 255;

		static bool isButtonDown(uint8_t code)
		{
			ControllerState previousState = {};
			ControllerState currentState = {};
			pollState(previousState, currentState);
			return isCodeDown(code, currentState);
		}

		static bool isButtonPressed(uint8_t code)
		{
			ControllerState previousState = {};
			ControllerState currentState = {};
			pollState(previousState, currentState);
			return isCodeDown(code, currentState) && !isCodeDown(code, previousState);
		}

		static ControllerType getControllerType()
		{
			ControllerState previousState = {};
			ControllerState currentState = {};
			pollState(previousState, currentState);
			return currentState.type;
		}

		static bool isPlayStationControllerDetected()
		{
			refreshDualSenseDevice(false);
			if (s_dualSenseHandle != INVALID_HANDLE_VALUE)
				return true;

			// Preserve detection of PlayStation controllers translated through
			// Steam Input, DS4Windows or another XInput wrapper.
			XINPUT_STATE state = {};
			if (!tryGetXInputState(0, state))
				return false;

			ULONG charCount = 0;
			if (CM_Get_Device_ID_List_SizeW(&charCount, nullptr, CM_GETIDLIST_FILTER_PRESENT) != CR_SUCCESS || charCount == 0)
				return false;

			std::vector<wchar_t> deviceList(static_cast<size_t>(charCount) + 2, L'\0');
			if (CM_Get_Device_ID_ListW(nullptr, deviceList.data(), static_cast<ULONG>(deviceList.size()), CM_GETIDLIST_FILTER_PRESENT) != CR_SUCCESS)
				return false;

			for (const wchar_t* current = deviceList.data(); *current != L'\0'; current += std::wcslen(current) + 1)
			{
				const std::string deviceId = wideToUtf8(current);
				if (looksLikePlayStationDeviceString(deviceId))
					return true;
			}

			return false;
		}


		static bool isNintendoControllerDetected()
		{
			refreshNintendoDevice(false);
			if (s_nintendoHandle != INVALID_HANDLE_VALUE)
				return true;

			ULONG charCount = 0;
			if (CM_Get_Device_ID_List_SizeW(&charCount, nullptr, CM_GETIDLIST_FILTER_PRESENT) != CR_SUCCESS || charCount == 0)
				return false;

			std::vector<wchar_t> deviceList(static_cast<size_t>(charCount) + 2, L'\0');
			if (CM_Get_Device_ID_ListW(nullptr, deviceList.data(), static_cast<ULONG>(deviceList.size()), CM_GETIDLIST_FILTER_PRESENT) != CR_SUCCESS)
				return false;

			for (const wchar_t* current = deviceList.data(); *current != L'\0'; current += std::wcslen(current) + 1)
			{
				const std::string deviceId = wideToUtf8(current);
				if (looksLikeNintendoDeviceString(deviceId))
					return true;
			}

			return false;
		}

	private:
		static constexpr BYTE GPAD_TRIGGER_THRESHOLD = 30;
		static constexpr USHORT SONY_VENDOR_ID = 0x054C;
		static constexpr USHORT DUALSENSE_PRODUCT_ID = 0x0CE6;
		static constexpr USHORT DUALSENSE_EDGE_PRODUCT_ID = 0x0DF2;
		static constexpr DWORD DUALSENSE_RESCAN_INTERVAL_MS = 2000;
		static constexpr size_t DUALSENSE_REPORT_BUFFER_SIZE = 128;

		static constexpr USHORT NINTENDO_VENDOR_ID = 0x057E;
		static constexpr USHORT SWITCH_PRO_PRODUCT_ID = 0x2009;
		static constexpr USHORT JOYCON_LEFT_PRODUCT_ID = 0x2006;
		static constexpr USHORT JOYCON_RIGHT_PRODUCT_ID = 0x2007;
		static constexpr USHORT JOYCON_GRIP_PRODUCT_ID = 0x200E;
		static constexpr DWORD NINTENDO_RESCAN_INTERVAL_MS = 2000;
		static constexpr size_t NINTENDO_REPORT_BUFFER_SIZE = 64;

		enum ButtonBits : uint32_t
		{
			BUTTON_A          = 1u << 0,
			BUTTON_B          = 1u << 1,
			BUTTON_X          = 1u << 2,
			BUTTON_Y          = 1u << 3,
			BUTTON_LB         = 1u << 4,
			BUTTON_RB         = 1u << 5,
			BUTTON_BACK       = 1u << 6,
			BUTTON_START      = 1u << 7,
			BUTTON_LS         = 1u << 8,
			BUTTON_RS         = 1u << 9,
			BUTTON_DPAD_UP    = 1u << 10,
			BUTTON_DPAD_DOWN  = 1u << 11,
			BUTTON_DPAD_LEFT  = 1u << 12,
			BUTTON_DPAD_RIGHT = 1u << 13
		};

		struct ControllerState
		{
			uint32_t buttons = 0;
			uint8_t leftTrigger = 0;
			uint8_t rightTrigger = 0;
			bool connected = false;
			ControllerType type = ControllerType::None;
		};

		using XInputGetStateFunc = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);

		static inline HMODULE s_xinputModule = nullptr;
		static inline XInputGetStateFunc s_xinputGetState = nullptr;
		static inline bool s_xinputInitialized = false;

		static inline HANDLE s_dualSenseHandle = INVALID_HANDLE_VALUE;
		static inline HANDLE s_dualSenseReadEvent = nullptr;
		static inline OVERLAPPED s_dualSenseOverlapped = {};
		static inline bool s_dualSenseReadPending = false;
		static inline BYTE s_dualSenseReport[DUALSENSE_REPORT_BUFFER_SIZE] = {};
		static inline DWORD s_lastDualSenseScanTick = 0;
		static inline ControllerState s_dualSenseState = {};

		static inline HANDLE s_nintendoHandle = INVALID_HANDLE_VALUE;
		static inline HANDLE s_nintendoReadEvent = nullptr;
		static inline OVERLAPPED s_nintendoOverlapped = {};
		static inline bool s_nintendoReadPending = false;
		static inline BYTE s_nintendoReport[NINTENDO_REPORT_BUFFER_SIZE] = {};
		static inline DWORD s_lastNintendoScanTick = 0;
		static inline ControllerState s_nintendoState = {};
		static inline USHORT s_nintendoProductId = 0;
		static inline uint8_t s_nintendoPacketCounter = 0;

		static inline DWORD s_lastPollTick = 0;
		static inline ControllerState s_previousState = {};
		static inline ControllerState s_currentState = {};

		static void initializeXInput()
		{
			if (s_xinputInitialized)
				return;

			s_xinputInitialized = true;

			static const wchar_t* dllNames[] =
			{
				L"xinput1_4.dll",
				L"xinput1_3.dll",
				L"xinput9_1_0.dll"
			};

			for (const wchar_t* dllName : dllNames)
			{
				HMODULE module = LoadLibraryW(dllName);
				if (module == nullptr)
					continue;

				auto proc = reinterpret_cast<XInputGetStateFunc>(GetProcAddress(module, "XInputGetState"));
				if (proc != nullptr)
				{
					s_xinputModule = module;
					s_xinputGetState = proc;
					return;
				}

				FreeLibrary(module);
			}
		}

		static bool tryGetXInputState(DWORD userIndex, XINPUT_STATE& state)
		{
			initializeXInput();

			if (s_xinputGetState == nullptr)
				return false;

			ZeroMemory(&state, sizeof(state));
			return s_xinputGetState(userIndex, &state) == ERROR_SUCCESS;
		}

		static ControllerState getXInputControllerState()
		{
			ControllerState result = {};
			XINPUT_STATE state = {};
			if (!tryGetXInputState(0, state))
				return result;

			result.connected = true;
			result.type = ControllerType::Xbox;
			result.leftTrigger = state.Gamepad.bLeftTrigger;
			result.rightTrigger = state.Gamepad.bRightTrigger;

			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0) result.buttons |= BUTTON_A;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0) result.buttons |= BUTTON_B;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0) result.buttons |= BUTTON_X;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0) result.buttons |= BUTTON_Y;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0) result.buttons |= BUTTON_LB;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) result.buttons |= BUTTON_RB;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0) result.buttons |= BUTTON_BACK;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0) result.buttons |= BUTTON_START;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0) result.buttons |= BUTTON_LS;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0) result.buttons |= BUTTON_RS;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0) result.buttons |= BUTTON_DPAD_UP;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0) result.buttons |= BUTTON_DPAD_DOWN;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0) result.buttons |= BUTTON_DPAD_LEFT;
			if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0) result.buttons |= BUTTON_DPAD_RIGHT;

			return result;
		}

		static void pollState(ControllerState& previousState, ControllerState& currentState)
		{
			const DWORD nowTick = GetTickCount();
			if (nowTick != s_lastPollTick)
			{
				s_previousState = s_currentState;

				const ControllerState dualSenseState = getDualSenseControllerState();
				if (dualSenseState.connected)
				{
					s_currentState = dualSenseState;
				}
				else
				{
					const ControllerState nintendoState = getNintendoControllerState();
					if (nintendoState.connected)
						s_currentState = nintendoState;
					else
						s_currentState = getXInputControllerState();
				}

				s_lastPollTick = nowTick;
			}

			previousState = s_previousState;
			currentState = s_currentState;
		}

		static bool isCodeDown(uint8_t code, const ControllerState& state)
		{
			if (!state.connected)
				return false;

			if (code == GPAD_LT)
				return state.leftTrigger > GPAD_TRIGGER_THRESHOLD;
			if (code == GPAD_RT)
				return state.rightTrigger > GPAD_TRIGGER_THRESHOLD;

			const uint32_t mask = codeToButtonBit(code);
			return mask != 0 && (state.buttons & mask) != 0;
		}

		static uint32_t codeToButtonBit(uint8_t code)
		{
			switch (code)
			{
			case GPAD_A:          return BUTTON_A;
			case GPAD_B:          return BUTTON_B;
			case GPAD_X:          return BUTTON_X;
			case GPAD_Y:          return BUTTON_Y;
			case GPAD_LB:         return BUTTON_LB;
			case GPAD_RB:         return BUTTON_RB;
			case GPAD_BACK:       return BUTTON_BACK;
			case GPAD_START:      return BUTTON_START;
			case GPAD_LS:         return BUTTON_LS;
			case GPAD_RS:         return BUTTON_RS;
			case GPAD_DPAD_UP:    return BUTTON_DPAD_UP;
			case GPAD_DPAD_DOWN:  return BUTTON_DPAD_DOWN;
			case GPAD_DPAD_LEFT:  return BUTTON_DPAD_LEFT;
			case GPAD_DPAD_RIGHT: return BUTTON_DPAD_RIGHT;
			default:              return 0;
			}
		}

		static bool isDualSenseProduct(USHORT productId)
		{
			return productId == DUALSENSE_PRODUCT_ID || productId == DUALSENSE_EDGE_PRODUCT_ID;
		}

		static void refreshDualSenseDevice(bool forceRescan)
		{
			if (s_dualSenseHandle != INVALID_HANDLE_VALUE)
				return;

			const DWORD nowTick = GetTickCount();
			if (!forceRescan && nowTick - s_lastDualSenseScanTick < DUALSENSE_RESCAN_INTERVAL_MS)
				return;
			s_lastDualSenseScanTick = nowTick;

			GUID hidGuid = {};
			HidD_GetHidGuid(&hidGuid);

			HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
				&hidGuid,
				nullptr,
				nullptr,
				DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

			if (deviceInfoSet == INVALID_HANDLE_VALUE)
				return;

			for (DWORD index = 0;; ++index)
			{
				SP_DEVICE_INTERFACE_DATA interfaceData = {};
				interfaceData.cbSize = sizeof(interfaceData);
				if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &hidGuid, index, &interfaceData))
					break;

				DWORD requiredSize = 0;
				SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, nullptr, 0, &requiredSize, nullptr);
				if (requiredSize == 0)
					continue;

				std::vector<BYTE> detailBuffer(requiredSize, 0);
				auto* detailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detailBuffer.data());
				detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
				if (!SetupDiGetDeviceInterfaceDetailW(
					deviceInfoSet,
					&interfaceData,
					detailData,
					requiredSize,
					nullptr,
					nullptr))
				{
					continue;
				}

				HANDLE deviceHandle = CreateFileW(
					detailData->DevicePath,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					nullptr,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
					nullptr);

				if (deviceHandle == INVALID_HANDLE_VALUE)
				{
					deviceHandle = CreateFileW(
						detailData->DevicePath,
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						nullptr,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
						nullptr);
				}

				if (deviceHandle == INVALID_HANDLE_VALUE)
					continue;

				HIDD_ATTRIBUTES attributes = {};
				attributes.Size = sizeof(attributes);
				if (!HidD_GetAttributes(deviceHandle, &attributes) ||
					attributes.VendorID != SONY_VENDOR_ID ||
					!isDualSenseProduct(attributes.ProductID))
				{
					CloseHandle(deviceHandle);
					continue;
				}

				s_dualSenseHandle = deviceHandle;
				s_dualSenseReadEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
				if (s_dualSenseReadEvent == nullptr)
				{
					closeDualSenseDevice();
					break;
				}

				ZeroMemory(&s_dualSenseOverlapped, sizeof(s_dualSenseOverlapped));
				s_dualSenseOverlapped.hEvent = s_dualSenseReadEvent;
				ZeroMemory(s_dualSenseReport, sizeof(s_dualSenseReport));
				s_dualSenseReadPending = false;
				s_dualSenseState = {};
				startDualSenseRead();
				break;
			}

			SetupDiDestroyDeviceInfoList(deviceInfoSet);
		}

		static void closeDualSenseDevice()
		{
			if (s_dualSenseHandle != INVALID_HANDLE_VALUE)
			{
				CancelIoEx(s_dualSenseHandle, &s_dualSenseOverlapped);
				CloseHandle(s_dualSenseHandle);
				s_dualSenseHandle = INVALID_HANDLE_VALUE;
			}

			if (s_dualSenseReadEvent != nullptr)
			{
				CloseHandle(s_dualSenseReadEvent);
				s_dualSenseReadEvent = nullptr;
			}

			s_dualSenseReadPending = false;
			s_dualSenseState = {};
			ZeroMemory(&s_dualSenseOverlapped, sizeof(s_dualSenseOverlapped));
		}

		static bool startDualSenseRead()
		{
			if (s_dualSenseHandle == INVALID_HANDLE_VALUE || s_dualSenseReadEvent == nullptr)
				return false;

			ResetEvent(s_dualSenseReadEvent);
			ZeroMemory(s_dualSenseReport, sizeof(s_dualSenseReport));

			DWORD bytesRead = 0;
			const BOOL readResult = ReadFile(
				s_dualSenseHandle,
				s_dualSenseReport,
				static_cast<DWORD>(sizeof(s_dualSenseReport)),
				&bytesRead,
				&s_dualSenseOverlapped);

			if (readResult)
			{
				s_dualSenseReadPending = false;
				if (bytesRead > 0)
					parseDualSenseReport(s_dualSenseReport, bytesRead);
				return true;
			}

			const DWORD error = GetLastError();
			if (error == ERROR_IO_PENDING)
			{
				s_dualSenseReadPending = true;
				return true;
			}

			closeDualSenseDevice();
			return false;
		}

		static void updateDualSenseState()
		{
			refreshDualSenseDevice(false);
			if (s_dualSenseHandle == INVALID_HANDLE_VALUE)
				return;

			if (!s_dualSenseReadPending)
			{
				startDualSenseRead();
				return;
			}

			DWORD bytesRead = 0;
			if (!GetOverlappedResult(s_dualSenseHandle, &s_dualSenseOverlapped, &bytesRead, FALSE))
			{
				const DWORD error = GetLastError();
				if (error == ERROR_IO_INCOMPLETE)
					return;

				closeDualSenseDevice();
				return;
			}

			s_dualSenseReadPending = false;
			if (bytesRead > 0)
				parseDualSenseReport(s_dualSenseReport, bytesRead);

			startDualSenseRead();
		}

		static ControllerState getDualSenseControllerState()
		{
			updateDualSenseState();
			if (s_dualSenseHandle == INVALID_HANDLE_VALUE)
				return ControllerState{};

			ControllerState result = s_dualSenseState;
			result.connected = true;
			result.type = ControllerType::PlayStation;
			return result;
		}

		static void parseDualSenseReport(const BYTE* report, DWORD reportLength)
		{
			if (report == nullptr || reportLength == 0)
				return;

			// USB reports use ID 0x01 and start the common input state at byte 1.
			// Bluetooth reports use ID 0x31 and start it at byte 2.
			size_t stateOffset = 0;
			if (report[0] == 0x01)
				stateOffset = 1;
			else if (report[0] == 0x31)
				stateOffset = 2;
			else
				return;

			const size_t requiredLength = stateOffset + 10;
			if (reportLength < requiredLength)
				return;

			ControllerState state = {};
			state.connected = true;
			state.type = ControllerType::PlayStation;
			state.leftTrigger = report[stateOffset + 4];
			state.rightTrigger = report[stateOffset + 5];

			const BYTE buttons0 = report[stateOffset + 7];
			const BYTE buttons1 = report[stateOffset + 8];
			const BYTE dpad = buttons0 & 0x0F;

			// Face-button positions are mapped to the existing Xbox-style stored
			// codes so all existing INI files and ToggleGroup behavior remain valid.
			if ((buttons0 & 0x20) != 0) state.buttons |= BUTTON_A; // Cross
			if ((buttons0 & 0x40) != 0) state.buttons |= BUTTON_B; // Circle
			if ((buttons0 & 0x10) != 0) state.buttons |= BUTTON_X; // Square
			if ((buttons0 & 0x80) != 0) state.buttons |= BUTTON_Y; // Triangle

			if ((buttons1 & 0x01) != 0) state.buttons |= BUTTON_LB;
			if ((buttons1 & 0x02) != 0) state.buttons |= BUTTON_RB;
			if ((buttons1 & 0x10) != 0) state.buttons |= BUTTON_BACK;  // Create
			if ((buttons1 & 0x20) != 0) state.buttons |= BUTTON_START; // Options
			if ((buttons1 & 0x40) != 0) state.buttons |= BUTTON_LS;
			if ((buttons1 & 0x80) != 0) state.buttons |= BUTTON_RS;

			switch (dpad)
			{
			case 0: state.buttons |= BUTTON_DPAD_UP; break;
			case 1: state.buttons |= BUTTON_DPAD_UP | BUTTON_DPAD_RIGHT; break;
			case 2: state.buttons |= BUTTON_DPAD_RIGHT; break;
			case 3: state.buttons |= BUTTON_DPAD_RIGHT | BUTTON_DPAD_DOWN; break;
			case 4: state.buttons |= BUTTON_DPAD_DOWN; break;
			case 5: state.buttons |= BUTTON_DPAD_DOWN | BUTTON_DPAD_LEFT; break;
			case 6: state.buttons |= BUTTON_DPAD_LEFT; break;
			case 7: state.buttons |= BUTTON_DPAD_LEFT | BUTTON_DPAD_UP; break;
			default: break;
			}

			s_dualSenseState = state;
		}


		static bool isNintendoProduct(USHORT productId)
		{
			return productId == SWITCH_PRO_PRODUCT_ID ||
				   productId == JOYCON_LEFT_PRODUCT_ID ||
				   productId == JOYCON_RIGHT_PRODUCT_ID ||
				   productId == JOYCON_GRIP_PRODUCT_ID;
		}

		static void closeNintendoDevice()
		{
			if (s_nintendoHandle != INVALID_HANDLE_VALUE)
			{
				CancelIoEx(s_nintendoHandle, &s_nintendoOverlapped);
				CloseHandle(s_nintendoHandle);
				s_nintendoHandle = INVALID_HANDLE_VALUE;
			}

			if (s_nintendoReadEvent != nullptr)
			{
				CloseHandle(s_nintendoReadEvent);
				s_nintendoReadEvent = nullptr;
			}

			s_nintendoReadPending = false;
			s_nintendoState = {};
			s_nintendoProductId = 0;
			ZeroMemory(&s_nintendoOverlapped, sizeof(s_nintendoOverlapped));
		}

		static bool writeNintendoOutputReport(const BYTE* report, DWORD reportLength)
		{
			if (s_nintendoHandle == INVALID_HANDLE_VALUE || report == nullptr || reportLength == 0)
				return false;

			DWORD bytesWritten = 0;
			return WriteFile(s_nintendoHandle, report, reportLength, &bytesWritten, nullptr) != FALSE;
		}

		static void sendNintendoUsbCommand(BYTE command)
		{
			BYTE report[64] = {};
			report[0] = 0x80;
			report[1] = command;
			writeNintendoOutputReport(report, static_cast<DWORD>(sizeof(report)));
		}

		static void sendNintendoSubcommand(BYTE subcommand, BYTE argument)
		{
			BYTE report[64] = {};
			report[0] = 0x01;
			report[1] = static_cast<BYTE>(s_nintendoPacketCounter++ & 0x0F);

			// Neutral rumble data required by Nintendo's output-report format.
			report[2] = 0x00;
			report[3] = 0x01;
			report[4] = 0x40;
			report[5] = 0x40;
			report[6] = 0x00;
			report[7] = 0x01;
			report[8] = 0x40;
			report[9] = 0x40;

			report[10] = subcommand;
			report[11] = argument;
			writeNintendoOutputReport(report, static_cast<DWORD>(sizeof(report)));
		}

		static void initializeNintendoInputMode()
		{
			if (s_nintendoHandle == INVALID_HANDLE_VALUE)
				return;

			// These USB commands are ignored on Bluetooth, but are required by
			// some wired Switch Pro controllers before subcommands are accepted.
			sendNintendoUsbCommand(0x02);
			sendNintendoUsbCommand(0x04);

			// Request the standard full input report (0x30).
			sendNintendoSubcommand(0x03, 0x30);
		}

		static void refreshNintendoDevice(bool forceRescan)
		{
			if (s_nintendoHandle != INVALID_HANDLE_VALUE)
				return;

			const DWORD nowTick = GetTickCount();
			if (!forceRescan && nowTick - s_lastNintendoScanTick < NINTENDO_RESCAN_INTERVAL_MS)
				return;
			s_lastNintendoScanTick = nowTick;

			GUID hidGuid = {};
			HidD_GetHidGuid(&hidGuid);

			HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
				&hidGuid,
				nullptr,
				nullptr,
				DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

			if (deviceInfoSet == INVALID_HANDLE_VALUE)
				return;

			for (DWORD index = 0;; ++index)
			{
				SP_DEVICE_INTERFACE_DATA interfaceData = {};
				interfaceData.cbSize = sizeof(interfaceData);
				if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &hidGuid, index, &interfaceData))
					break;

				DWORD requiredSize = 0;
				SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, nullptr, 0, &requiredSize, nullptr);
				if (requiredSize == 0)
					continue;

				std::vector<BYTE> detailBuffer(requiredSize, 0);
				auto* detailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detailBuffer.data());
				detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
				if (!SetupDiGetDeviceInterfaceDetailW(
					deviceInfoSet,
					&interfaceData,
					detailData,
					requiredSize,
					nullptr,
					nullptr))
				{
					continue;
				}

				HANDLE deviceHandle = CreateFileW(
					detailData->DevicePath,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					nullptr,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
					nullptr);

				if (deviceHandle == INVALID_HANDLE_VALUE)
				{
					deviceHandle = CreateFileW(
						detailData->DevicePath,
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						nullptr,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
						nullptr);
				}

				if (deviceHandle == INVALID_HANDLE_VALUE)
					continue;

				HIDD_ATTRIBUTES attributes = {};
				attributes.Size = sizeof(attributes);
				if (!HidD_GetAttributes(deviceHandle, &attributes) ||
					attributes.VendorID != NINTENDO_VENDOR_ID ||
					!isNintendoProduct(attributes.ProductID))
				{
					CloseHandle(deviceHandle);
					continue;
				}

				s_nintendoHandle = deviceHandle;
				s_nintendoProductId = attributes.ProductID;
				s_nintendoReadEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
				if (s_nintendoReadEvent == nullptr)
				{
					closeNintendoDevice();
					break;
				}

				ZeroMemory(&s_nintendoOverlapped, sizeof(s_nintendoOverlapped));
				s_nintendoOverlapped.hEvent = s_nintendoReadEvent;
				ZeroMemory(s_nintendoReport, sizeof(s_nintendoReport));
				s_nintendoReadPending = false;
				s_nintendoState = {};
				initializeNintendoInputMode();
				startNintendoRead();
				break;
			}

			SetupDiDestroyDeviceInfoList(deviceInfoSet);
		}

		static bool startNintendoRead()
		{
			if (s_nintendoHandle == INVALID_HANDLE_VALUE || s_nintendoReadEvent == nullptr)
				return false;

			ResetEvent(s_nintendoReadEvent);
			ZeroMemory(s_nintendoReport, sizeof(s_nintendoReport));

			DWORD bytesRead = 0;
			const BOOL readResult = ReadFile(
				s_nintendoHandle,
				s_nintendoReport,
				static_cast<DWORD>(sizeof(s_nintendoReport)),
				&bytesRead,
				&s_nintendoOverlapped);

			if (readResult)
			{
				s_nintendoReadPending = false;
				if (bytesRead > 0)
					parseNintendoReport(s_nintendoReport, bytesRead);
				return true;
			}

			const DWORD error = GetLastError();
			if (error == ERROR_IO_PENDING)
			{
				s_nintendoReadPending = true;
				return true;
			}

			closeNintendoDevice();
			return false;
		}

		static void updateNintendoState()
		{
			refreshNintendoDevice(false);
			if (s_nintendoHandle == INVALID_HANDLE_VALUE)
				return;

			if (!s_nintendoReadPending)
			{
				startNintendoRead();
				return;
			}

			DWORD bytesRead = 0;
			if (!GetOverlappedResult(s_nintendoHandle, &s_nintendoOverlapped, &bytesRead, FALSE))
			{
				const DWORD error = GetLastError();
				if (error == ERROR_IO_INCOMPLETE)
					return;

				closeNintendoDevice();
				return;
			}

			s_nintendoReadPending = false;
			if (bytesRead > 0)
				parseNintendoReport(s_nintendoReport, bytesRead);

			startNintendoRead();
		}

		static ControllerState getNintendoControllerState()
		{
			updateNintendoState();
			if (s_nintendoHandle == INVALID_HANDLE_VALUE)
				return ControllerState{};

			ControllerState result = s_nintendoState;
			result.connected = true;
			result.type = ControllerType::Nintendo;
			return result;
		}

		static void parseNintendoReport(const BYTE* report, DWORD reportLength)
		{
			if (report == nullptr || reportLength < 6)
				return;

			const BYTE reportId = report[0];
			if (reportId != 0x30 && reportId != 0x21)
				return;

			// Both 0x30 input reports and 0x21 subcommand replies expose the
			// three common button bytes at offsets 3, 4 and 5.
			const BYTE rightButtons = report[3];
			const BYTE sharedButtons = report[4];
			const BYTE leftButtons = report[5];

			ControllerState state = {};
			state.connected = true;
			state.type = ControllerType::Nintendo;

			// Map physical positions to the existing logical/Xbox-style codes:
			// south B -> A, east A -> B, west Y -> X, north X -> Y.
			if ((rightButtons & 0x04) != 0) state.buttons |= BUTTON_A; // B
			if ((rightButtons & 0x08) != 0) state.buttons |= BUTTON_B; // A
			if ((rightButtons & 0x01) != 0) state.buttons |= BUTTON_X; // Y
			if ((rightButtons & 0x02) != 0) state.buttons |= BUTTON_Y; // X

			if ((leftButtons & 0x40) != 0) state.buttons |= BUTTON_LB; // L
			if ((rightButtons & 0x40) != 0) state.buttons |= BUTTON_RB; // R
			if ((sharedButtons & 0x01) != 0) state.buttons |= BUTTON_BACK; // -
			if ((sharedButtons & 0x02) != 0) state.buttons |= BUTTON_START; // +
			if ((sharedButtons & 0x08) != 0) state.buttons |= BUTTON_LS;
			if ((sharedButtons & 0x04) != 0) state.buttons |= BUTTON_RS;

			if ((leftButtons & 0x02) != 0) state.buttons |= BUTTON_DPAD_UP;
			if ((leftButtons & 0x01) != 0) state.buttons |= BUTTON_DPAD_DOWN;
			if ((leftButtons & 0x08) != 0) state.buttons |= BUTTON_DPAD_LEFT;
			if ((leftButtons & 0x04) != 0) state.buttons |= BUTTON_DPAD_RIGHT;

			// Nintendo ZL/ZR are digital; expose them through the existing
			// trigger codes as fully pressed values.
			state.leftTrigger = (leftButtons & 0x80) != 0 ? 255 : 0;
			state.rightTrigger = (rightButtons & 0x80) != 0 ? 255 : 0;

			s_nintendoState = state;
		}

		static std::string toLowerAscii(const std::string& input)
		{
			std::string output = input;
			for (char& character : output)
			{
				if (character >= 'A' && character <= 'Z')
					character = static_cast<char>(character - 'A' + 'a');
			}
			return output;
		}

		static bool containsInsensitive(const std::string& haystack, const char* needle)
		{
			const std::string lowerHaystack = toLowerAscii(haystack);
			const std::string lowerNeedle = toLowerAscii(std::string(needle));
			return lowerHaystack.find(lowerNeedle) != std::string::npos;
		}

		static std::string wideToUtf8(const wchar_t* text)
		{
			if (text == nullptr || *text == L'\0')
				return std::string();

			const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
			if (required <= 1)
				return std::string();

			std::vector<char> buffer(static_cast<size_t>(required), '\0');
			WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer.data(), required, nullptr, nullptr);
			return std::string(buffer.data());
		}

		static bool looksLikePlayStationDeviceString(const std::string& deviceText)
		{
			return
				containsInsensitive(deviceText, "sony") ||
				containsInsensitive(deviceText, "playstation") ||
				containsInsensitive(deviceText, "dualshock") ||
				containsInsensitive(deviceText, "dualsense") ||
				containsInsensitive(deviceText, "wireless controller") ||
				containsInsensitive(deviceText, "vid_054c");
		}

		static bool looksLikeNintendoDeviceString(const std::string& deviceText)
		{
			return
				containsInsensitive(deviceText, "nintendo") ||
				containsInsensitive(deviceText, "switch pro") ||
				containsInsensitive(deviceText, "joy-con") ||
				containsInsensitive(deviceText, "vid_057e");
		}
	};
}
