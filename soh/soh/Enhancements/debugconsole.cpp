#include "debugconsole.h"
#include "../libultraship/SohImGuiImpl.h"
#include <vector>
#include <string>

#define Path _Path
#define PATH_HACK
#include <Utils/StringHelper.h>
#include <Utils/File.h>
#undef PATH_HACK
#undef Path

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern GlobalContext* gGlobalCtx;
}

#include "Cvar.h"

#ifndef NO_SOH_CONSOLE
#define CMD_REGISTER SohImGui::BindCmd
#else
#define CMD_REGISTER(...)
#endif

static bool ActorSpawnHandler(const std::vector<std::string>& args) {
    if ((args.size() != 9) && (args.size() != 3) && (args.size() != 6)) {
        ERROR("Not enough arguments passed to actorspawn");
        return CMD_FAILED;
    }

    if (gGlobalCtx == nullptr) {
        ERROR("GlobalCtx == nullptr");
        return CMD_FAILED;
    }

    Player* player = GET_PLAYER(gGlobalCtx);
    PosRot spawnPoint;
    const s16 actorId = std::stoi(args[1]);
    const s16 params = std::stoi(args[2]);

    spawnPoint = player->actor.world;

    switch (args.size()) {
        case 9:
            if (args[6][0] != ',') {
                spawnPoint.rot.x = std::stoi(args[6]);
            }
            if (args[7][0] != ',') {
                spawnPoint.rot.y = std::stoi(args[7]);
            }
            if (args[8][0] != ',') {
                spawnPoint.rot.z = std::stoi(args[8]);
            }
        case 5:
            if (args[3][0] != ',') {
                spawnPoint.pos.x = std::stoi(args[3]);
            }
            if (args[4][0] != ',') {
                spawnPoint.pos.y = std::stoi(args[4]);
            }
            if (args[5][0] != ',') {
                spawnPoint.pos.z = std::stoi(args[5]);
            }
    }

    if (Actor_Spawn(&gGlobalCtx->actorCtx, gGlobalCtx, actorId, spawnPoint.pos.x, spawnPoint.pos.y, spawnPoint.pos.z,
                    spawnPoint.rot.x, spawnPoint.rot.y, spawnPoint.rot.z, params) == NULL) {
        ERROR("Failed to spawn actor. Actor_Spawn returned NULL");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}


static bool KillPlayerHandler([[maybe_unused]] const std::vector<std::string>&) {
    gSaveContext.health = 0;

    INFO("[SOH] You've met with a terrible fate, haven't you?");
    return CMD_SUCCESS;
}

static bool SetPlayerHealthHandler(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        ERROR("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    int health;

    try {
        health = std::stoi(args[1]);
    } catch (std::invalid_argument const& ex) {
        ERROR("[SOH] Health value must be an integer.");
        return CMD_FAILED;
    }

    if (health < 0) {
        ERROR("[SOH] Health value must be a positive integer");
        return CMD_SUCCESS;
    }

    gSaveContext.health = health * 0x10;

    INFO("[SOH] Player health updated to %d", health);
    return CMD_SUCCESS;
}


static bool LoadSceneHandler(const std::vector<std::string>&) {
    gSaveContext.respawnFlag = 0;
    gSaveContext.seqId = 0xFF;
    gSaveContext.gameMode = 0;

    return CMD_SUCCESS;
}

static bool RuppeHandler(const std::vector<std::string>& args) {
    if (args.size() < 2)
        return CMD_FAILED;

    int rupeeAmount;
    try {
        rupeeAmount = std::stoi(args[1]);
    }
    catch (std::invalid_argument const& ex) { 
        ERROR("[SOH] Rupee count must be an integer.");
        return CMD_FAILED;
    }

    if (rupeeAmount < 0) {
        ERROR("[SOH] Rupee count must be positive");
        return CMD_FAILED;
    }

    gSaveContext.rupees = rupeeAmount;

    INFO("Set rupee count to %u", rupeeAmount);
    return CMD_SUCCESS;
}

static bool SetPosHandler(const std::vector<std::string> args) {
    if (gGlobalCtx == nullptr) {
        ERROR("GlobalCtx == nullptr");
        return CMD_FAILED;
    }

    Player* player = GET_PLAYER(gGlobalCtx);

    if (args.size() == 1) {
        INFO("Player position is [ %.2f, %.2f, %.2f ]", player->actor.world.pos.x, player->actor.world.pos.y,
             player->actor.world.pos.z);
        return CMD_SUCCESS;
    }
    if (args.size() < 4)
        return CMD_FAILED;

    player->actor.world.pos.x = std::stof(args[1]);
    player->actor.world.pos.y = std::stof(args[2]);
    player->actor.world.pos.z = std::stof(args[3]);

    INFO("Set player position to [ %.2f, %.2f, %.2f ]", player->actor.world.pos.x, player->actor.world.pos.y,
         player->actor.world.pos.z);
    return CMD_SUCCESS;
}

static bool ResetHandler(std::vector<std::string> args) {
    if (gGlobalCtx == nullptr) {
        ERROR("GlobalCtx == nullptr");
        return CMD_FAILED;
    }
    
    SET_NEXT_GAMESTATE(&gGlobalCtx->state, TitleSetup_Init, GameState);
    gGlobalCtx->state.running = false;
    return CMD_SUCCESS;
}

const static std::map<std::string, uint16_t> ammoItems{ 
    { "sticks", ITEM_STICK }, { "deku_sticks", ITEM_STICK },
    { "nuts", ITEM_NUT },     { "deku_nuts", ITEM_NUT },
    { "bombs", ITEM_BOMB },      { "arrows", ITEM_BOW },
    { "bombchus", ITEM_BOMBCHU }, { "chus", ITEM_BOMBCHU },
    { "beans", ITEM_BEAN },
    { "seeds", ITEM_SLINGSHOT }, { "deku_seeds", ITEM_SLINGSHOT },
    { "magic_beans", ITEM_BEAN },
};

static bool AmmoHandler(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        ERROR("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    int count;

    try {
        count = std::stoi(args[2]);
    } catch (std::invalid_argument const& ex) { 
        ERROR("Ammo count must be an integer");
        return CMD_FAILED;
    }

    if (count < 0) {
        ERROR("Ammo count must be positive");
        return CMD_FAILED;
    }
    
    const auto& it = ammoItems.find(args[1]);

    if (it == ammoItems.end()) {
        ERROR("Invalid item passed");
        return CMD_FAILED;
    }

    // I dont think you can do OOB with just this
    AMMO(it->second) = count;
    
    //To use a change by uncomment this
    //Inventory_ChangeAmmo(it->second, count);
}

const static std::map<std::string, uint16_t> bottleItems{
    { "green_potion", ITEM_POTION_GREEN },
    { "red_potion", ITEM_POTION_RED },
    { "blue_potion", ITEM_POTION_BLUE },
    { "milk", ITEM_MILK },    { "half_milk", ITEM_MILK_HALF },           { "fairy", ITEM_FAIRY },
    { "bugs", ITEM_BUG }, { "fish", ITEM_FISH },         { "poe", ITEM_POE },
    { "big_poe", ITEM_BIG_POE },  { "blue_fire", ITEM_BLUE_FIRE }, { "rutos_letter", ITEM_LETTER_RUTO },
};

static bool BottleHandler(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        ERROR("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    unsigned int slot;
    try {
        slot = std::stoi(args[2]);
    } catch (std::invalid_argument const& ex) { 
        ERROR("[SOH] Bottle slot must be an integer.");
        return CMD_FAILED;
    }

    if ((slot < 1) || (slot > 4)) {
        ERROR("Invalid slot passed");
        return CMD_FAILED;
    }

    const auto& it = bottleItems.find(args[1]);

    if (it ==  bottleItems.end()) {
        ERROR("Invalid item passed");
        return CMD_FAILED;
    }

    // I dont think you can do OOB with just this
    gSaveContext.inventory.items[0x11 + slot] = it->second;

    return CMD_SUCCESS;
}

static bool BHandler(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        ERROR("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    gSaveContext.equips.buttonItems[0] = std::stoi(args[1]);
    return CMD_SUCCESS;
}

static bool ItemHandler(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        ERROR("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    gSaveContext.inventory.items[std::stoi(args[1])] = std::stoi(args[2]); 

    return CMD_SUCCESS;
}

static bool EntranceHandler(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        ERROR("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    unsigned int entrance;

    try {
        entrance = std::stoi(args[1], nullptr, 16);
    } catch (std::invalid_argument const& ex) {
        ERROR("[SOH] Entrance value must be a Hex number.");
        return CMD_FAILED;
    }
    gGlobalCtx->nextEntranceIndex = entrance;

    gGlobalCtx->sceneLoadFlag = 0x14;
    gGlobalCtx->fadeTransition = 11;
    gSaveContext.nextTransition = 11;
}

#define VARTYPE_INTEGER 0
#define VARTYPE_FLOAT   1
#define VARTYPE_STRING  2

static int CheckVarType(const std::string& input)
{
    int result = VARTYPE_STRING;

    for (size_t i = 0; i < input.size(); i++)
    {
        if (!(std::isdigit(input[i]) || input[i] == '.'))
        {
            break;
        }
        else
        {
            if (input[i] == '.')
                result = VARTYPE_FLOAT;
            else if (std::isdigit(input[i]) && result != VARTYPE_FLOAT)
                result = VARTYPE_INTEGER;
        }
    }

    return result;
}

void DebugConsole_LoadCVars();
void DebugConsole_SaveCVars();

static bool SetCVarHandler(const std::vector<std::string>& args) {
    if (args.size() < 3)
        return CMD_FAILED;

    int vType = CheckVarType(args[2]);

    if (vType == VARTYPE_STRING)
        CVar_SetString((char*)args[1].c_str(), (char*)args[2].c_str());
    else if (vType == VARTYPE_FLOAT)
        CVar_SetFloat((char*)args[1].c_str(), std::stof(args[2]));
    else
        CVar_SetS32((char*)args[1].c_str(), std::stoi(args[2]));

    DebugConsole_SaveCVars();

    //INFO("[SOH] Updated player position to [ %.2f, %.2f, %.2f ]", pos.x, pos.y, pos.z);
    return CMD_SUCCESS;
}


static bool GetCVarHandler(const std::vector<std::string>& args) {
    if (args.size() < 2)
        return CMD_FAILED;

    CVar* cvar = CVar_GetVar((char*) args[1].c_str());

    if (cvar != nullptr)
    {
        if (cvar->type == CVAR_TYPE_S32)
            INFO("[SOH] Variable %s is %i", args[1].c_str(), cvar->value.valueS32);
        else if (cvar->type == CVAR_TYPE_FLOAT)
            INFO("[SOH] Variable %s is %f", args[1].c_str(), cvar->value.valueFloat);
        else if (cvar->type == CVAR_TYPE_STRING)
            INFO("[SOH] Variable %s is %s", args[1].c_str(), cvar->value.valueStr);
    }
    else
    {
        INFO("[SOH] Could not find variable %s", args[1].c_str());
    }


    return CMD_SUCCESS;
}

void DebugConsole_Init(void) {
    CMD_REGISTER("kill", { KillPlayerHandler, "Commit suicide." });
    CMD_REGISTER("map",  { LoadSceneHandler, "Load up kak?" });
    CMD_REGISTER("rupee", { RuppeHandler, "Set your rupee counter.", {
        {"amount", ArgumentType::NUMBER }
    }});
    CMD_REGISTER("bItem", { BHandler, "Set an item to the B button.", { { "Item ID", ArgumentType::NUMBER } } });
    CMD_REGISTER("health", { SetPlayerHealthHandler, "Set the health of the player.", {
        {"health", ArgumentType::NUMBER }
    }});
    CMD_REGISTER("spawn", { ActorSpawnHandler, "Spawn an actor.", {
        { "actor_id", ArgumentType::NUMBER },
        { "data",     ArgumentType::NUMBER },
        { "x",        ArgumentType::PLAYER_POS, true },
        { "y",        ArgumentType::PLAYER_POS, true },
        { "z",        ArgumentType::PLAYER_POS, true },
        { "rx",       ArgumentType::PLAYER_ROT, true },
        { "ry",       ArgumentType::PLAYER_ROT, true },
        { "rz",       ArgumentType::PLAYER_ROT, true }
    }});
    CMD_REGISTER("pos", { SetPosHandler, "Sets the position of the player.", {
        { "x",        ArgumentType::PLAYER_POS, true },
        { "y",        ArgumentType::PLAYER_POS, true },
        { "z",        ArgumentType::PLAYER_POS, true }
    }});
    CMD_REGISTER("set", { SetCVarHandler,
                          "Sets a console variable.",
                          { { "varName", ArgumentType::TEXT }, { "varValue", ArgumentType::TEXT } } });
    CMD_REGISTER("get", { GetCVarHandler, "Gets a console variable.", { { "varName", ArgumentType::TEXT } } });
    CMD_REGISTER("reset", { ResetHandler, "Resets the game." });
    CMD_REGISTER("ammo", { AmmoHandler, "Changes ammo of an item.",
                            { { "item", ArgumentType::TEXT },
                              { "count", ArgumentType::NUMBER } } });

    CMD_REGISTER("bottle", { BottleHandler,
                       "Changes item in a bottle slot.",
                       { { "item", ArgumentType::TEXT }, { "slot", ArgumentType::NUMBER } } });

    CMD_REGISTER("item", { ItemHandler,
                             "Sets item ID in arg 1 into slot arg 2. No boundary checks. Use with caution.",
                             { { "slot", ArgumentType::NUMBER }, { "item id", ArgumentType::NUMBER } } });
    CMD_REGISTER("entrance",
                 { EntranceHandler, "Sends player to the entered entrance (hex)", { { "entrance", ArgumentType::NUMBER } } });

    DebugConsole_LoadCVars();
}

void DebugConsole_LoadCVars()
{
#ifndef NO_SOH_CONSOLE
    if (File::Exists("cvars.cfg")) {
        const auto lines = File::ReadAllLines("cvars.cfg");

        for (const std::string& line : lines) {
            SohImGui::console->Dispatch(line);
        }
    }
#endif
}

void DebugConsole_SaveCVars()
{
    std::string output;

    for (const auto &cvar : cvars) {
        if (cvar.second->type == CVAR_TYPE_STRING)
            output += StringHelper::Sprintf("set %s %s\n", cvar.first.c_str(), cvar.second->value.valueStr);
        else if (cvar.second->type == CVAR_TYPE_S32)
            output += StringHelper::Sprintf("set %s %i\n", cvar.first.c_str(), cvar.second->value.valueS32);
        else if (cvar.second->type == CVAR_TYPE_FLOAT)
            output += StringHelper::Sprintf("set %s %f\n", cvar.first.c_str(), cvar.second->value.valueFloat);
    }

    File::WriteAllText("cvars.cfg", output);
}