#include "WiiUController.h"
#include "GlobalCtx2.h"

#include <padscore/kpad.h>
#include <padscore/wpad.h>
#include <vpad/input.h>

namespace Ship {
	WiiUController::WiiUController(int32_t dwControllerNumber) : Controller(dwControllerNumber) {
		LoadBinding();
	}

	WiiUController::~WiiUController() {
		
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
			if (status.hold & VPAD_BUTTON_ZR)
				dwPressedButtons |= BTN_Z;
			if (status.hold & VPAD_BUTTON_R)
				dwPressedButtons |= BTN_R;
			if (status.hold & VPAD_BUTTON_L)
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

			if (status.rightStick.x > 0.2f)
				dwPressedButtons |= BTN_CRIGHT;
			if (status.rightStick.x < -0.2f)
				dwPressedButtons |= BTN_CLEFT;
			if (status.rightStick.y > 0.2f)
				dwPressedButtons |= BTN_CUP;
			if (status.rightStick.y < -0.2f)
				dwPressedButtons |= BTN_CDOWN;

			wStickX = status.leftStick.x * 84;
			wStickY = status.leftStick.y * 84;

			// TODO gyro
		}

		KPADStatus kStatus;
		int32_t kProbe;
		uint32_t kType;

		for (int i = 0; i < 4; i++) {
			kProbe = WPADProbe(i, &kType);

			if (kProbe)
				continue;

			KPADRead(i, &kStatus, 1);

			switch (kType) {
				case WPAD_EXT_PRO_CONTROLLER:
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_A)
						dwPressedButtons |= BTN_A;
					if (kStatus.pro.hold & WPAD_PRO_BUTTON_B)
						dwPressedButtons |= BTN_B;
					if (kStatus.pro.hold & WPAD_PRO_TRIGGER_ZR)
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

					if (kStatus.pro.rightStick.x > 0.2f)
						dwPressedButtons |= BTN_CRIGHT;
					if (kStatus.pro.rightStick.x < -0.2f)
						dwPressedButtons |= BTN_CLEFT;
					if (kStatus.pro.rightStick.y > 0.2f)
						dwPressedButtons |= BTN_CUP;
					if (kStatus.pro.rightStick.y < -0.2f)
						dwPressedButtons |= BTN_CDOWN;

					wStickX = kStatus.pro.leftStick.x * 84;
					wStickY = kStatus.pro.leftStick.y * 84;
					break;
				case WPAD_EXT_CLASSIC:
				case WPAD_EXT_MPLUS_CLASSIC:
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_A)
						dwPressedButtons |= BTN_A;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_B)
						dwPressedButtons |= BTN_B;
					if (kStatus.classic.hold & WPAD_CLASSIC_BUTTON_ZR)
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
					break;
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
		// TODO rumble
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
