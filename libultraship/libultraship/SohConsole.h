#pragma once

#include <map>
#include <vector>
#include <string>
#include <functional>

#ifndef NO_IMGUI
#include "Lib/ImGui/imgui.h"

#define LOG(msg, ...) SohImGui::console->Append("Main", Priority::LOG_LVL, msg, ##__VA_ARGS__)
#define INFO(msg, ...) SohImGui::console->Append("Main", Priority::INFO_LVL, msg, ##__VA_ARGS__)
#define WARNING(msg, ...) SohImGui::console->Append("Main", Priority::WARNING_LVL, msg, ##__VA_ARGS__)
#define ERROR(msg, ...) SohImGui::console->Append("Main", Priority::ERROR_LVL, msg, ##__VA_ARGS__)
#else
// don't bother logging to the console if it's not displayed
#define LOG(msg, ...)
#define INFO(msg, ...)
#define WARNING(msg, ...)
#define ERROR(msg, ...)
#endif
#define CMD_SUCCESS true
#define CMD_FAILED false
#define MAX_BUFFER_SIZE 255
#define NULLSTR "None"

typedef std::function<bool(std::vector<std::string> args)> CommandHandler;

enum Priority {
	INFO_LVL,
	LOG_LVL,
	WARNING_LVL,
	ERROR_LVL
};

enum class ArgumentType {
	TEXT, NUMBER, PLAYER_POS, PLAYER_ROT
};

struct CommandArgument {
	std::string info;
	ArgumentType type = ArgumentType::NUMBER;
	bool optional = false;
};

struct CommandEntry {
	CommandHandler handler;
	std::string description;
	std::vector<CommandArgument> arguments;
};

struct ConsoleLine {
	std::string text;
	Priority priority = Priority::INFO_LVL;
	std::string channel = "Main";
};

class Console {
	int selectedId = -1;
	std::vector<int> selectedEntries;
	std::string filter;
	std::string level_filter = NULLSTR;
	std::vector<std::string> log_channels = { "Main", "SoH Logging"};
	std::vector<std::string> priority_filters = { "None", "Info", "Log", "Warning", "Error" };
#ifndef NO_IMGUI
	std::vector<ImVec4> priority_colors = {
		ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
		ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
		ImVec4(0.9f, 0.8f, 0.4f, 0.01f),
		ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
	};
#endif
public:
	std::map<std::string, std::vector<ConsoleLine>> Log;
	std::map<std::string, CommandEntry> Commands;
	std::vector<std::string> Autocomplete;
	std::vector<std::string> History;
	std::string CMDHint = NULLSTR;
	char* FilterBuffer = nullptr;
	char* InputBuffer = nullptr;
	bool OpenAutocomplete = false;
	int HistoryIndex = -1;
	std::string selected_channel = "Main";
	bool opened = false;
	void Init();
	void Update();
	void Draw();
#ifndef NO_IMGUI
	void Append(const std::string& channel, Priority priority, const char* fmt, ...) IM_FMTARGS(4);
	static int CallbackStub(ImGuiInputTextCallbackData* data);
#endif
	void Dispatch(const std::string& line);
};