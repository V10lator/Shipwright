#ifdef __WIIU__
#include "WiiUController.h"
#include "GlobalCtx2.h"

#include <padscore/kpad.h>
#include <padscore/wpad.h>

namespace Ship {
    WiiUController::WiiUController(int32_t dwControllerNumber) : Controller(dwControllerNumber) {
    }

    WiiUController::~WiiUController() {
    }

    void WiiUController::ReadFromSource() {
        auto config = GlobalCtx2::GetInstance()->GetConfig();
        WPADChan chan = (WPADChan) GetControllerNumber();
        KPADStatus kStatus;
        KPADError kError;
        WPADExtensionType kType;

        if (WPADProbe(chan, &kType) != 0) {
            connected = false;
            return;
        }

        // Reload binding if type changed
        if (kType != extensionType) {
            extensionType = kType;

            if (!config->has(GetBindingConfSection())) {
                CreateDefaultBinding();
            }

            LoadBinding();
        }

        KPADReadEx(chan, &kStatus, 1, &kError);
        if (kError != KPAD_ERROR_OK) {
            return;
        }

        dwPressedButtons = 0;
        wStickX = 0;
        wStickY = 0;

        switch (kType) {
            case WPAD_EXT_PRO_CONTROLLER:
                for (uint32_t i = WPAD_PRO_BUTTON_UP; i <= WPAD_PRO_STICK_R_EMULATION_UP; i <<= 1) {
                    if (ButtonMapping.contains(i)) {
                        // check if the stick is mapped to an analog stick
                        if ((ButtonMapping[i] == BTN_STICKRIGHT || ButtonMapping[i] == BTN_STICKLEFT) && (i >= WPAD_PRO_STICK_L_EMULATION_LEFT)) {
                            float axis = i >= WPAD_PRO_STICK_R_EMULATION_LEFT ? kStatus.pro.rightStick.x : kStatus.pro.leftStick.x;
                            wStickX = axis * 84;
                        } else if ((ButtonMapping[i] == BTN_STICKDOWN || ButtonMapping[i] == BTN_STICKUP) && (i >= WPAD_PRO_STICK_L_EMULATION_LEFT)) {
                            float axis = i >= WPAD_PRO_STICK_R_EMULATION_LEFT ? kStatus.pro.rightStick.y : kStatus.pro.leftStick.y;
                            wStickY = axis * 84;
                        } else {
                            if (kStatus.pro.hold & i) {
                                dwPressedButtons |= ButtonMapping[i];
                            }
                        }
                    }
                }
                break;
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                for (uint32_t i = WPAD_CLASSIC_BUTTON_UP; i <= WPAD_CLASSIC_STICK_R_EMULATION_UP; i <<= 1) {
                    if (ButtonMapping.contains(i)) {
                        // check if the stick is mapped to an analog stick
                        if ((ButtonMapping[i] == BTN_STICKRIGHT || ButtonMapping[i] == BTN_STICKLEFT) && (i >= WPAD_CLASSIC_STICK_L_EMULATION_LEFT)) {
                            float axis = i >= WPAD_CLASSIC_STICK_R_EMULATION_LEFT ? kStatus.classic.rightStick.x : kStatus.classic.leftStick.x;
                            wStickX = axis * 84;
                        } else if ((ButtonMapping[i] == BTN_STICKDOWN || ButtonMapping[i] == BTN_STICKUP) && (i >= WPAD_CLASSIC_STICK_L_EMULATION_LEFT)) {
                            float axis = i >= WPAD_CLASSIC_STICK_R_EMULATION_LEFT ? kStatus.classic.rightStick.y : kStatus.classic.leftStick.y;
                            wStickY = axis * 84;
                        } else {
                            if (kStatus.classic.hold & i) {
                                dwPressedButtons |= ButtonMapping[i];
                            }
                        }
                    }
                }
                break;
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK:
            case WPAD_EXT_CORE:
                for (uint32_t i = WPAD_BUTTON_LEFT; i <= WPAD_BUTTON_HOME; i <<= 1) {
                    if (ButtonMapping.contains(i)) {
                        if (kStatus.hold & i) {
                            dwPressedButtons |= ButtonMapping[i];
                        }
                    }
                }
                wStickX += kStatus.nunchuck.stick.x * 84;
                wStickY += kStatus.nunchuck.stick.y * 84;
                break;
        }
    }

    void WiiUController::WriteToSource(ControllerCallback* controller) {
        WPADControlMotor((WPADChan) GetControllerNumber(), controller->rumble);
    }

    void WiiUController::CreateDefaultBinding() {
        std::string ConfSection = GetBindingConfSection();
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();

        switch (extensionType) {
            case WPAD_EXT_PRO_CONTROLLER:
                Conf[ConfSection][STR(BTN_CRIGHT)] = std::to_string(WPAD_PRO_STICK_R_EMULATION_RIGHT);
                Conf[ConfSection][STR(BTN_CLEFT)] = std::to_string(WPAD_PRO_STICK_R_EMULATION_LEFT);
                Conf[ConfSection][STR(BTN_CDOWN)] = std::to_string(WPAD_PRO_STICK_R_EMULATION_DOWN);
                Conf[ConfSection][STR(BTN_CUP)] = std::to_string(WPAD_PRO_STICK_R_EMULATION_UP);
                Conf[ConfSection][STR(BTN_CRIGHT_2)] = std::to_string(WPAD_PRO_BUTTON_X);
                Conf[ConfSection][STR(BTN_CLEFT_2)] = std::to_string(WPAD_PRO_BUTTON_Y);
                Conf[ConfSection][STR(BTN_CDOWN_2)] = std::to_string(WPAD_PRO_TRIGGER_R);
                Conf[ConfSection][STR(BTN_CUP_2)] = std::to_string(WPAD_PRO_TRIGGER_L);
                Conf[ConfSection][STR(BTN_R)] = std::to_string(WPAD_PRO_TRIGGER_ZR);
                Conf[ConfSection][STR(BTN_L)] = std::to_string(WPAD_PRO_BUTTON_MINUS);
                Conf[ConfSection][STR(BTN_DRIGHT)] = std::to_string(WPAD_PRO_BUTTON_RIGHT);
                Conf[ConfSection][STR(BTN_DLEFT)] = std::to_string(WPAD_PRO_BUTTON_LEFT);
                Conf[ConfSection][STR(BTN_DDOWN)] = std::to_string(WPAD_PRO_BUTTON_DOWN);
                Conf[ConfSection][STR(BTN_DUP)] = std::to_string(WPAD_PRO_BUTTON_UP);
                Conf[ConfSection][STR(BTN_START)] = std::to_string(WPAD_PRO_BUTTON_PLUS);
                Conf[ConfSection][STR(BTN_Z)] = std::to_string(WPAD_PRO_TRIGGER_ZL);
                Conf[ConfSection][STR(BTN_B)] = std::to_string(WPAD_PRO_BUTTON_B);
                Conf[ConfSection][STR(BTN_A)] = std::to_string(WPAD_PRO_BUTTON_A);
                Conf[ConfSection][STR(BTN_STICKRIGHT)] = std::to_string(WPAD_PRO_STICK_L_EMULATION_RIGHT);
                Conf[ConfSection][STR(BTN_STICKLEFT)] = std::to_string(WPAD_PRO_STICK_L_EMULATION_LEFT);
                Conf[ConfSection][STR(BTN_STICKDOWN)] = std::to_string(WPAD_PRO_STICK_L_EMULATION_DOWN);
                Conf[ConfSection][STR(BTN_STICKUP)] = std::to_string(WPAD_PRO_STICK_L_EMULATION_UP);
                break;
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                Conf[ConfSection][STR(BTN_CRIGHT)] = std::to_string(WPAD_CLASSIC_STICK_R_EMULATION_RIGHT);
                Conf[ConfSection][STR(BTN_CLEFT)] = std::to_string(WPAD_CLASSIC_STICK_R_EMULATION_LEFT);
                Conf[ConfSection][STR(BTN_CDOWN)] = std::to_string(WPAD_CLASSIC_STICK_R_EMULATION_DOWN);
                Conf[ConfSection][STR(BTN_CUP)] = std::to_string(WPAD_CLASSIC_STICK_R_EMULATION_UP);
                Conf[ConfSection][STR(BTN_CRIGHT_2)] = std::to_string(WPAD_CLASSIC_BUTTON_X);
                Conf[ConfSection][STR(BTN_CLEFT_2)] = std::to_string(WPAD_CLASSIC_BUTTON_Y);
                Conf[ConfSection][STR(BTN_CDOWN_2)] = std::to_string(WPAD_CLASSIC_BUTTON_ZR);
                Conf[ConfSection][STR(BTN_CUP_2)] = std::to_string(WPAD_CLASSIC_BUTTON_ZL);
                Conf[ConfSection][STR(BTN_R)] = std::to_string(WPAD_CLASSIC_BUTTON_R);
                Conf[ConfSection][STR(BTN_L)] = std::to_string(WPAD_CLASSIC_BUTTON_MINUS);
                Conf[ConfSection][STR(BTN_DRIGHT)] = std::to_string(WPAD_CLASSIC_BUTTON_RIGHT);
                Conf[ConfSection][STR(BTN_DLEFT)] = std::to_string(WPAD_CLASSIC_BUTTON_LEFT);
                Conf[ConfSection][STR(BTN_DDOWN)] = std::to_string(WPAD_CLASSIC_BUTTON_DOWN);
                Conf[ConfSection][STR(BTN_DUP)] = std::to_string(WPAD_CLASSIC_BUTTON_UP);
                Conf[ConfSection][STR(BTN_START)] = std::to_string(WPAD_CLASSIC_BUTTON_PLUS);
                Conf[ConfSection][STR(BTN_Z)] = std::to_string(WPAD_CLASSIC_BUTTON_L);
                Conf[ConfSection][STR(BTN_B)] = std::to_string(WPAD_CLASSIC_BUTTON_B);
                Conf[ConfSection][STR(BTN_A)] = std::to_string(WPAD_CLASSIC_BUTTON_A);
                Conf[ConfSection][STR(BTN_STICKRIGHT)] = std::to_string(WPAD_CLASSIC_STICK_L_EMULATION_RIGHT);
                Conf[ConfSection][STR(BTN_STICKLEFT)] = std::to_string(WPAD_CLASSIC_STICK_L_EMULATION_LEFT);
                Conf[ConfSection][STR(BTN_STICKDOWN)] = std::to_string(WPAD_CLASSIC_STICK_L_EMULATION_DOWN);
                Conf[ConfSection][STR(BTN_STICKUP)] = std::to_string(WPAD_CLASSIC_STICK_L_EMULATION_UP);
                break;
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK:
            case WPAD_EXT_CORE:
                Conf[ConfSection][STR(BTN_R)] = std::to_string(WPAD_BUTTON_1);
                Conf[ConfSection][STR(BTN_L)] = std::to_string(WPAD_BUTTON_2);
                Conf[ConfSection][STR(BTN_DRIGHT)] = std::to_string(WPAD_BUTTON_RIGHT);
                Conf[ConfSection][STR(BTN_DLEFT)] = std::to_string(WPAD_BUTTON_LEFT);
                Conf[ConfSection][STR(BTN_DDOWN)] = std::to_string(WPAD_BUTTON_DOWN);
                Conf[ConfSection][STR(BTN_DUP)] = std::to_string(WPAD_BUTTON_UP);
                Conf[ConfSection][STR(BTN_START)] = std::to_string(WPAD_BUTTON_PLUS);
                Conf[ConfSection][STR(BTN_Z)] = std::to_string(WPAD_BUTTON_MINUS);
                Conf[ConfSection][STR(BTN_B)] = std::to_string(WPAD_BUTTON_B);
                Conf[ConfSection][STR(BTN_A)] = std::to_string(WPAD_BUTTON_A);
                break;
        }

        Conf.Save();
    }

    std::string WiiUController::GetControllerExtension() {
        switch (extensionType) {
            case WPAD_EXT_PRO_CONTROLLER:
                return "ProController";
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                return "ClassicController";
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK:
            case WPAD_EXT_CORE:
                return "WiiRemote";
        }

        return "Controller";
    }

    std::string WiiUController::GetControllerType() {
        return "WIIU";
    }

    std::string WiiUController::GetConfSection() {
        return GetControllerType() + " " + GetControllerExtension() + " " + std::to_string(GetControllerNumber() + 1);
    }

    std::string WiiUController::GetBindingConfSection() {
        return GetControllerType() + " " + GetControllerExtension() + " BINDING " + std::to_string(GetControllerNumber() + 1);
    }
}
#endif
