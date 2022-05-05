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
			bool Connected() const { return true; };
			bool CanRumble() const { return true; };

			bool HasPadConf() const { return false; };
			std::optional<std::string> GetPadConfSection() { return {}; };

		protected:
			std::string GetControllerType();
			std::string GetConfSection();
			std::string GetBindingConfSection();

		private:
			uint8_t rumblePattern[15];
	};
}
