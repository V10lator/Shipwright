#pragma once
#include "Controller.h"
#include <string>

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
	};
}
