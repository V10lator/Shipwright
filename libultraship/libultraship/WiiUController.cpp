#include "WiiUController.h"
#include "GlobalCtx2.h"

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
