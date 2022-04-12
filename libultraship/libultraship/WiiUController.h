#pragma once
#include "Controller.h"
#include <string>

extern "C" {
	extern void WPADControlMotor(int controller, int state);
}

namespace Ship {
	class WiiUController : public Controller {
		public:
			WiiUController(int32_t dwControllerNumber);
			~WiiUController();

			void ReadFromSource();
			void WriteToSource(ControllerCallback* controller);

		protected:
			std::string GetControllerType();
			std::string GetConfSection();
			std::string GetBindingConfSection();

		private:
			uint8_t rumblePattern[15];
	};
}
