#include "WiiUController.h"
#include "GlobalCtx2.h"

#include <padscore/kpad.h>
#include <padscore/wpad.h>
#include <vpad/input.h>

extern "C" {
	extern void WPADControlMotor(int controller, int state);
}

namespace Ship {
	static uint8_t rumblePattern[120];

	WiiUController::WiiUController(int32_t dwControllerNumber) : Controller(dwControllerNumber) {
		KPADInit();
		WPADEnableURCC(true);
		memset(rumblePattern, 0xFF, 120);
		LoadBinding();
	}

	WiiUController::~WiiUController() {
		KPADShutdown();
	}

	void WiiUController::ReadFromSource() {
		dwPressedButtons = 0;
		wStickX = 0;
		wStickY = 0;

		VPADStatus status;
		VPADReadError vError;
		VPADRead(VPAD_CHAN_0, &status, 1, &vError);

		// TODO button mappings
		if (vError == VPAD_READ_SUCCESS) {
			if (status.hold & VPAD_BUTTON_A)
				dwPressedButtons |= BTN_A;
			if (status.hold & VPAD_BUTTON_B)
				dwPressedButtons |= BTN_B;
			if (status.hold & VPAD_BUTTON_ZL)
				dwPressedButtons |= BTN_Z;
			if (status.hold & VPAD_BUTTON_ZR)
				dwPressedButtons |= BTN_R;
			if (status.hold & VPAD_BUTTON_MINUS)
				dwPressedButtons |= BTN_L;
			if (status.hold & VPAD_BUTTON_PLUS)
				dwPressedButtons |= BTN_START;
			if (status.hold & VPAD_BUTTON_UP)
				dwPressedButtons |= BTN_DUP;
			if (status.hold & VPAD_BUTTON_DOWN)
				dwPressedButtons |= BTN_DDOWN;
			if (status.hold & VPAD_BUTTON_RIGHT)
				dwPressedButtons |= BTN_DRIGHT;
			if (status.hold & VPAD_BUTTON_LEFT)
				dwPressedButtons |= BTN_DLEFT;

			if (status.hold & VPAD_STICK_R_EMULATION_RIGHT || status.hold & VPAD_BUTTON_X)
				dwPressedButtons |= BTN_CRIGHT;
			else if (status.hold & VPAD_STICK_R_EMULATION_LEFT || status.hold & VPAD_BUTTON_Y)
				dwPressedButtons |= BTN_CLEFT;
			if (status.hold & VPAD_STICK_R_EMULATION_UP || status.hold & VPAD_BUTTON_L)
				dwPressedButtons |= BTN_CUP;
			else if (status.hold & VPAD_STICK_R_EMULATION_DOWN || status.hold & VPAD_BUTTON_R)
				dwPressedButtons |= BTN_CDOWN;

			wStickX = status.leftStick.x * 84;
			wStickY = status.leftStick.y * 84;

			// TODO gyro
		}

		KPADStatus kStatus;
		int32_t kProbe;
		WPADExtensionType kType;

		for (int i = 0; i < 4; i++) {
			kProbe = WPADProbe((WPADChan) i, &kType);

			if (kProbe)
				continue;

			KPADRead((WPADChan) i, &kStatus, 1);

			switch (kType) {
				case WPAD_EXT_PRO_CONTROLLER:
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_A)
						dwPressedButtons |= BTN_A;
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_B)
						dwPressedButtons |= BTN_B;
					if (kStatus.pro.hold & WPAD_PRO_TRIGGER_ZL)
						dwPressedButtons |= BTN_Z;
					if (kStatus.pro.hold & WPAD_PRO_TRIGGER_R)
						dwPressedButtons |= BTN_R;
					if (kStatus.pro.hold & WPAD_PRO_TRIGGER_L)
						dwPressedButtons |= BTN_L;
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_PLUS)
						dwPressedButtons |= BTN_START;
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_UP)
						dwPressedButtons |= BTN_DUP;
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_DOWN)
						dwPressedButtons |= BTN_DDOWN;
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_RIGHT)
						dwPressedButtons |= BTN_DRIGHT;
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_LEFT)
						dwPressedButtons |= BTN_DLEFT;

					if (kStatus.pro.hold & WPAD_PRO_STICK_R_EMULATION_RIGHT)
						dwPressedButtons |= BTN_CRIGHT;
					else if (kStatus.pro.hold & WPAD_PRO_STICK_R_EMULATION_LEFT)
						dwPressedButtons |= BTN_CLEFT;
					if (kStatus.pro.hold & WPAD_PRO_STICK_R_EMULATION_UP)
						dwPressedButtons |= BTN_CUP;
					else if (kStatus.pro.hold & WPAD_PRO_STICK_R_EMULATION_DOWN)
						dwPressedButtons |= BTN_CDOWN;

					wStickX += kStatus.pro.leftStick.x * 84;
					wStickY += kStatus.pro.leftStick.y * 84;
					break;
				case WPAD_EXT_CLASSIC:
				case WPAD_EXT_MPLUS_CLASSIC:
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_A)
						dwPressedButtons |= BTN_A;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_B)
						dwPressedButtons |= BTN_B;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_ZL)
						dwPressedButtons |= BTN_Z;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_R)
						dwPressedButtons |= BTN_R;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_L)
						dwPressedButtons |= BTN_L;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_PLUS)
						dwPressedButtons |= BTN_START;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_UP)
						dwPressedButtons |= BTN_DUP;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_DOWN)
						dwPressedButtons |= BTN_DDOWN;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_RIGHT)
						dwPressedButtons |= BTN_DRIGHT;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_LEFT)
						dwPressedButtons |= BTN_DLEFT;

					if (kStatus.classic.hold & WPAD_CLASSIC_STICK_R_EMULATION_RIGHT)
						dwPressedButtons |= BTN_CRIGHT;
					else if (kStatus.classic.hold & WPAD_CLASSIC_STICK_R_EMULATION_LEFT)
						dwPressedButtons |= BTN_CLEFT;
					if (kStatus.classic.hold & WPAD_CLASSIC_STICK_R_EMULATION_UP)
						dwPressedButtons |= BTN_CUP;
					else if (kStatus.classic.hold & WPAD_CLASSIC_STICK_R_EMULATION_DOWN)
						dwPressedButtons |= BTN_CDOWN;

					wStickX += kStatus.classic.leftStick.x * 84;
					wStickY += kStatus.classic.leftStick.y * 84;
					break;
				case WPAD_EXT_NUNCHUK:
				case WPAD_EXT_MPLUS_NUNCHUK:
					wStickX += kStatus.nunchuck.stick.x * 84;
					wStickY += kStatus.nunchuck.stick.y * 84;
			}

			if (kStatus.hold & WPAD_BUTTON_A)
				dwPressedButtons |= BTN_A;
			if (kStatus.hold & WPAD_BUTTON_B)
				dwPressedButtons |= BTN_B;
			if (kStatus.hold & WPAD_BUTTON_MINUS)
				dwPressedButtons |= BTN_Z;
			if (kStatus.hold & WPAD_BUTTON_1)
				dwPressedButtons |= BTN_R;
			if (kStatus.hold & WPAD_BUTTON_2)
				dwPressedButtons |= BTN_L;
			if (kStatus.hold & WPAD_BUTTON_PLUS)
				dwPressedButtons |= BTN_START;
			if (kStatus.hold & WPAD_BUTTON_UP)
				dwPressedButtons |= BTN_DUP;
			if (kStatus.hold & WPAD_BUTTON_DOWN)
				dwPressedButtons |= BTN_DDOWN;
			if (kStatus.hold & WPAD_BUTTON_RIGHT)
				dwPressedButtons |= BTN_DRIGHT;
			if (kStatus.hold & WPAD_BUTTON_LEFT)
				dwPressedButtons |= BTN_DLEFT;
		}
	}

	void WiiUController::WriteToSource(ControllerCallback* controller) {
		if (controller->rumble > 0) {
			VPADControlMotor(VPAD_CHAN_0, rumblePattern, 120);
			for (int i = 0; i < 4; i++)
					WPADControlMotor(i, 1);
		} else {
			VPADStopMotor(VPAD_CHAN_0);
			for (int i = 0; i < 4; i++)
					WPADControlMotor(i, 0);
		}
	}

	std::string WiiUController::GetControllerType() {
		return "WIIU GAMEPAD";
	}

	std::string WiiUController::GetConfSection() {
		return GetControllerType() + " CONTROLLER " + std::to_string(GetControllerNumber() + 1);
	}

	std::string WiiUController::GetBindingConfSection() {
		return GetControllerType() + " CONTROLLER BINDING " + std::to_string(GetControllerNumber() + 1);
	}
}
