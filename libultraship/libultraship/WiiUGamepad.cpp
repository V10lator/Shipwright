#ifdef __WIIU__
#include "WiiUGamepad.h"
#include "GlobalCtx2.h"

#include <vpad/input.h>

namespace Ship {
	WiiUGamepad::WiiUGamepad(int32_t dwControllerNumber) : Controller(dwControllerNumber) {
		memset(rumblePattern, 0xFF, sizeof(rumblePattern));

		auto config = GlobalCtx2::GetInstance()->GetConfig();
		if (!config->has(GetBindingConfSection())) {
			CreateDefaultBinding();
		}

		LoadBinding();
	}

	WiiUGamepad::~WiiUGamepad() {
	}

	void WiiUGamepad::ReadFromSource() {
		dwPressedButtons = 0;
		wStickX = 0;
		wStickY = 0;

		VPADStatus vStatus;
		VPADReadError vError;
		VPADRead(VPAD_CHAN_0, &vStatus, 1, &vError);

		if (vError != VPAD_READ_SUCCESS) {
			if (vError == VPAD_READ_INVALID_CONTROLLER) {
				connected = false;
			}
			return;
		} else {
			connected = true;
		}

		for (uint32_t i = VPAD_BUTTON_SYNC; i <= VPAD_STICK_L_EMULATION_LEFT; i <<= 1) {
			if (ButtonMapping.contains(i)) {
				// check if the stick is mapped to an analog stick
				if ((ButtonMapping[i] == BTN_STICKRIGHT || ButtonMapping[i] == BTN_STICKLEFT) && (i >= VPAD_STICK_R_EMULATION_DOWN)) {
					float axis = i >= VPAD_STICK_L_EMULATION_DOWN ? vStatus.leftStick.x : vStatus.rightStick.x;
					wStickX = axis * 84;
				} else if ((ButtonMapping[i] == BTN_STICKDOWN || ButtonMapping[i] == BTN_STICKUP) && (i >= VPAD_STICK_R_EMULATION_DOWN)) {
					float axis = i >= VPAD_STICK_L_EMULATION_DOWN ? vStatus.leftStick.y : vStatus.rightStick.y;
					wStickY = axis * 84;
				} else {
					if (vStatus.hold & i) {
						dwPressedButtons |= ButtonMapping[i];
					}
				}
			}
		}

		// TODO gyro
	}

	void WiiUGamepad::WriteToSource(ControllerCallback* controller) {
		VPADControlMotor(VPAD_CHAN_0, rumblePattern, controller->rumble ? 120 : 0);
	}

	void WiiUGamepad::CreateDefaultBinding() {
        std::string ConfSection = GetBindingConfSection();
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();

        Conf[ConfSection][STR(BTN_CRIGHT)] = std::to_string(VPAD_STICK_R_EMULATION_RIGHT);
        Conf[ConfSection][STR(BTN_CLEFT)] = std::to_string(VPAD_STICK_R_EMULATION_LEFT);
        Conf[ConfSection][STR(BTN_CDOWN)] = std::to_string(VPAD_STICK_R_EMULATION_DOWN);
        Conf[ConfSection][STR(BTN_CUP)] = std::to_string(VPAD_STICK_R_EMULATION_UP);
        Conf[ConfSection][STR(BTN_CRIGHT_2)] = std::to_string(VPAD_BUTTON_X);
        Conf[ConfSection][STR(BTN_CLEFT_2)] = std::to_string(VPAD_BUTTON_Y);
        Conf[ConfSection][STR(BTN_CDOWN_2)] = std::to_string(VPAD_BUTTON_R);
        Conf[ConfSection][STR(BTN_CUP_2)] = std::to_string(VPAD_BUTTON_L);
        Conf[ConfSection][STR(BTN_R)] = std::to_string(VPAD_BUTTON_ZR);
        Conf[ConfSection][STR(BTN_L)] = std::to_string(VPAD_BUTTON_MINUS);
        Conf[ConfSection][STR(BTN_DRIGHT)] = std::to_string(VPAD_BUTTON_RIGHT);
        Conf[ConfSection][STR(BTN_DLEFT)] = std::to_string(VPAD_BUTTON_LEFT);
        Conf[ConfSection][STR(BTN_DDOWN)] = std::to_string(VPAD_BUTTON_DOWN);
        Conf[ConfSection][STR(BTN_DUP)] = std::to_string(VPAD_BUTTON_UP);
        Conf[ConfSection][STR(BTN_START)] = std::to_string(VPAD_BUTTON_PLUS);
        Conf[ConfSection][STR(BTN_Z)] = std::to_string(VPAD_BUTTON_ZL);
        Conf[ConfSection][STR(BTN_B)] = std::to_string(VPAD_BUTTON_B);
        Conf[ConfSection][STR(BTN_A)] = std::to_string(VPAD_BUTTON_A);
        Conf[ConfSection][STR(BTN_STICKRIGHT)] = std::to_string(VPAD_STICK_L_EMULATION_RIGHT);
        Conf[ConfSection][STR(BTN_STICKLEFT)] = std::to_string(VPAD_STICK_L_EMULATION_LEFT);
        Conf[ConfSection][STR(BTN_STICKDOWN)] = std::to_string(VPAD_STICK_L_EMULATION_DOWN);
        Conf[ConfSection][STR(BTN_STICKUP)] = std::to_string(VPAD_STICK_L_EMULATION_UP);

        Conf.Save();
	}

	std::string WiiUGamepad::GetControllerType() {
		return "WIIU";
	}

	std::string WiiUGamepad::GetConfSection() {
		return GetControllerType() + " GAMEPAD " + std::to_string(GetControllerNumber() + 1);
	}

	std::string WiiUGamepad::GetBindingConfSection() {
		return GetControllerType() + " GAMEPAD BINDING " + std::to_string(GetControllerNumber() + 1);
	}
}
#endif
