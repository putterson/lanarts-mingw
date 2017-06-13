/*
 * GameChatCheatCodes.cpp:
 *  Implementation of special codes that can be typed in the chat
 */

#include <lua.hpp>

#include <luawrap/luawrap.h>
#include "gamestate/GameState.h"

#include <lcommon/math_util.h>

#include "stats/items/ItemEntry.h"

#include "objects/PlayerInst.h"

#include "lua_api/lua_api.h"

#include "GameChat.h"

static const char* skip_whitespace(const char* cstr) {
	while (*cstr != '\0' && isspace(*cstr)) {
		cstr++;
	}
	return cstr;
}
static bool starts_with(const std::string& str, const char* prefix,
		const char** content) {
	int length = strlen(prefix);
	bool hasprefix = strncmp(str.c_str(), prefix, length) == 0;
	if (hasprefix) {
		*content = skip_whitespace(str.c_str() + length);
		return true;
	}
	return false;
}

//Used for !spawn command
static void generate_enemies(GameState* gs, enemy_id etype,
                int amount) {
        MTwist& mt = gs->rng();
        GameTiles& tiles = gs->tiles();

        for (int i = 0; i < amount; i++) {
                Pos epos;
                int tries = 0;
                do {
                        int rand_x = mt.rand(tiles.tile_width());
                        int rand_y = mt.rand(tiles.tile_height());
                        epos = centered_multiple(Pos(rand_x, rand_y), TILE_SIZE);
                } while (gs->solid_test(NULL, epos.x, epos.y, 15));
                gs->add_instance(new EnemyInst(etype, epos.x, epos.y));
        }
}


static bool handle_spawn_enemies(GameState* gs, const std::string& command) {
	ChatMessage printed;
	const char* content;
	//Spawn monster
	if (starts_with(command, "!spawn ", &content)) {
		const char* rest = content;
		int amnt = strtol(content, (char**)&rest, 10);
		if (content == rest)
			amnt = 1;
		rest = skip_whitespace(rest);

		int enemy = get_enemy_by_name(rest, false);
		if (enemy == -1) {
			printed.message = "No such monster, '" + std::string(rest) + "'!";
			printed.message_colour = Colour(255, 50, 50);
		} else {
			printed.message = std::string(rest) + " has spawned !";
			generate_enemies(gs, enemy, amnt);
			printed.message_colour = Colour(50, 255, 50);
		}
                gs->game_chat().add_message(printed);
		return true;
	}
	return false;
}

static bool handle_set_gamespeed(GameState* gs, const std::string& command) {

	ChatMessage printed;
	const char* content;
	//Set game speed
	if (starts_with(command, "!gamespeed ", &content)) {
		int gamespeed = squish(atoi(content), 1, 200);
		gs->game_settings().time_per_step = gamespeed;
		printed.message = std::string("Game speed set.");
		printed.message_colour = Colour(50, 255, 50);

                gs->game_chat().add_message(printed);
		return true;
	}
	return false;

}

static bool handle_create_item(GameState* gs, const std::string& command) {
	ChatMessage printed;
	const char* content;
	PlayerInst* p = gs->local_player();
	//Create item
	if (starts_with(command, "!item ", &content)) {
		const char* rest = content;
		int amnt = strtol(content, (char**)&rest, 10);
		if (content == rest)
			amnt = 1;
		rest = skip_whitespace(rest);

		item_id item = get_item_by_name(rest, false);
		if (item == -1) {
			printed.message = "No such item, '" + std::string(rest) + "'!";
			printed.message_colour = Colour(255, 50, 50);
		} else {
			printed.message = std::string(rest) + " put in your inventory !";
			p->stats().equipment.inventory.add(Item(item, amnt));
			printed.message_colour = Colour(50, 255, 50);
		}
        gs->game_chat().add_message(printed);
		return true;
	}
	return false;
}

static bool handle_dolua(GameState* gs, const std::string& command) {
	const char* content;
	lua_State* L = gs->luastate();

        // Include local player as easily accessible object:
        luawrap::push(gs->luastate(), gs->local_player());
        luawrap::globals(gs->luastate())["player"].pop();

        bool ran = false;
	//Run lua command
	if (starts_with(command, "!lua ", &content)) {
		int prior_top = lua_gettop(L);

		luaL_loadstring(L, content);
		if (lua_isstring(L, -1)) {
			const char* val = lua_tostring(L, -1);
            gs->game_chat().add_message(val, /*iserr ? Colour(255,50,50) :*/
                Colour(120, 120, 255));
		} else {
                    bool iserr = (lua_pcall(L, 0, LUA_MULTRET, 0) != 0);

                    int current_top = lua_gettop(L);

                    for (; prior_top < current_top; prior_top++) {
                            if (lua_isstring(L, -1)) {
                                    const char* val = lua_tostring(L, -1);
                                    gs->game_chat().add_message(val,
                                                iserr ? Colour(255, 50, 50) : Colour(120, 120, 255));
                            }
                            lua_pop(L, 1);
                    }
                }
                ran = true;
	}
	//Run lua file
	if (starts_with(command, "!!", &content)) {
                std::string filename = (std::string("debug_scripts/") + content) + ".lua";
		int prior_top = lua_gettop(L);
		int err_func = luaL_loadfile(L, filename.c_str());
		if (err_func) {
			const char* val = lua_tostring(L, -1);
            gs->game_chat().add_message(val, Colour(120, 120, 255));
			lua_pop(L, 1);
		} else {
                    bool err_call = (lua_pcall(L, 0, 0, 0) != 0);
                    if (err_call) {
                            const char* val = lua_tostring(L, -1);
                            gs->game_chat().add_message(val, Colour(120, 120, 255));
                            lua_pop(L, 1);
                    }
                }
                ran = true;
	}

        // Clear local player variable:
        lua_pushnil(L);
        luawrap::globals(gs->luastate())["player"].pop();
	return ran;
}

bool GameChat::handle_special_commands(GameState* gs,
		const std::string& command) {
	// These codes will cause problems in multiplayer
	// Make them single-player only, unless we are in network debug mode
	if (gs->net_connection().is_connected()
			&& !gs->game_settings().network_debug_mode) {
		return false;
	}

	ChatMessage printed;
	const char* content;

	if (handle_spawn_enemies(gs, command)) {
		return true;
	}

	if (handle_set_gamespeed(gs, command)) {
		return true;
	}
	//Gain XP
	if (starts_with(command, "!gainxp ", &content)) {
		int xp = atoi(content);
		if (xp > 0 && xp <= 9999999) {
			printed.message = std::string("You have gained ") + content
					+ " experience.";
			printed.message_colour = Colour(50, 255, 50);
			add_message(printed);
			PlayerInst* p = gs->local_player();
			p->gain_xp(gs, xp);
		} else {
			printed.message = "Invalid experience amount!";
			printed.message_colour = Colour(255, 50, 50);
			add_message(printed);
		}
		return true;
	}

	//Kill all monsters
	if (starts_with(command, "!killall", &content)) {
		MonsterController& mc = gs->monster_controller();
		for (int i = 0; i < mc.monster_ids().size(); i++) {
			EnemyInst* inst = (EnemyInst*)gs->get_instance(mc.monster_ids()[i]);
			if (inst) {
				inst->damage(gs, 99999);
			}
		}
		printed.message = "Killed all monsters.";
		printed.message_colour = Colour(50, 255, 50);
		add_message(printed);
		return true;
	}

	if (handle_dolua(gs, command)) {
		return true;
	}
	if (handle_create_item(gs, command)) {
		return true;
	}

	return false;
}
