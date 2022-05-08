#pragma once
#include "Controller.h"
#include <string>

namespace Ship {
	class WiiUGamepad : public Controller {
		public:
			WiiUGamepad(int32_t dwControllerNumber);
			~WiiUGamepad();

			void ReadFromSource();
			void WriteToSource(ControllerCallback* controller);
			bool Connected() const { return connected; };
			bool CanRumble() const { return true; };

			bool HasPadConf() const { return false; };
			std::optional<std::string> GetPadConfSection() { return {}; };

		protected:
			void CreateDefaultBinding();
			std::string GetControllerType();
			std::string GetConfSection();
			std::string GetBindingConfSection();

		private:
			bool connected = true;
			uint8_t rumblePattern[15];
	};
}
