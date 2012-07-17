/*
 * main_menu.cpp:
 *  Implements start menu and other actions
 */

#include <iostream>
#include <vector>
#include <ctime>

#include <SDL.h>
#include <SDL_opengl.h>

#include <net/timing.h>

extern "C" {
#include <lua/lua.h>
#include <lua/lauxlib.h>
}

#include "data/game_data.h"

#include "display/display.h"

#include "lua/lua_api.h"

#include "world/GameState.h"

#include "world/objects/PlayerInst.h"
#include "world/objects/EnemyInst.h"

#include "world/utility_objects/AnimatedInst.h"
#include "world/utility_objects/ButtonInst.h"

static void continue_as_loner(GameState* gs, GameInst* _, void* flag) {
	*(bool*)flag = true;
	gs->game_settings().conntype = GameSettings::NONE;
}

static void continue_as_loner_save_replay(GameState* gs, GameInst* _,
		void* flag) {
	*(bool*)flag = true;
	gs->game_settings().conntype = GameSettings::NONE;
	gs->game_settings().savereplay_file = "replays/replay";
}

static void continue_as_load_replay(GameState* gs, GameInst* _, void* flag) {
	*(bool*)flag = true;
	gs->game_settings().conntype = GameSettings::NONE;
	gs->game_settings().loadreplay_file = "replays/replay";
}

static void continue_as_client(GameState* gs, GameInst* _, void* flag) {
	*(bool*)flag = true;
	gs->game_settings().conntype = GameSettings::CLIENT;
}

static void continue_as_server(GameState* gs, GameInst* _, void* flag) {
	*(bool*)flag = true;
	gs->game_settings().conntype = GameSettings::SERVER;
}

static const char HELP_TEXT[] = "Movement: WASD or Arrow Keys\n"
		"Use Attack or Attack Spell: j or left-click\n"
		"Switch Auto-Target (for use with 'j'): k\n"
		"Switch Attack Spell: space\n"
		"Switch to Melee: m or e\n"
		"Use Blink: h or right-click\n"
		"Use Item: click item or keys 1 through 9\n"
		"Use Stairs: < and > or mouse wheel\n"
		"Regenerate: Hold 'r'\n";

static void setup_buttons(GameState* gs, bool* exit, int x, int y) {
	ObjCallback single(continue_as_loner, exit);
	ObjCallback savereplay(continue_as_loner_save_replay, exit);
	ObjCallback loadreplay(continue_as_load_replay, exit);
	ObjCallback client(continue_as_client, exit);
	ObjCallback server(continue_as_server, exit);
	gs->add_instance(new ButtonInst("Single-Player", x, y, single));
	y += 50;
	gs->add_instance(new ButtonInst("Save Replay", x - 95, y, savereplay));
	gs->add_instance(new ButtonInst("Load Replay", x + 95, y, loadreplay));
	y += 50;
	gs->add_instance(new ButtonInst("Client", x - 65, y, client));
	gs->add_instance(new ButtonInst("Server", x + 65, y, server));
	y += 50;
}

void menu_loop(GameState* gs, int width, int height) {
	bool exit = false;
	int halfw = width / 2;

	GameView prevview = gs->window_view();
	GameLevelState* oldlevel = gs->get_level();

	gs->set_level(new GameLevelState(0, 0, 0, width, height));
	gs->get_level()->level_number = -1;
	gs->window_view().x = 0;
	gs->window_view().y = 0;

	gs->add_instance(new AnimatedInst(halfw, 100, get_sprite_by_name("logo")));
	gs->add_instance(
			new AnimatedInst(halfw - 100, 500, -1, -1, 0.0f, 0.0f,
					AnimatedInst::DEPTH, HELP_TEXT, Colour(255, 255, 255)));

	setup_buttons(gs, &exit, halfw, 300);

	for (; gs->update_iostate() && !gs->key_down_state(SDLK_RETURN) && !exit;) {
		gs->get_level()->inst_set.step(gs);
		gs->draw(false);
	}

	delete gs->get_level();

	gs->set_level(oldlevel);
	gs->window_view() = prevview;
}
