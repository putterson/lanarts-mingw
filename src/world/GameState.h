/*
 * GameState.h
 *
 *  Created on: 2011-09-29
 *      Author: 100397561
 */

#ifndef GAMESTATE_H_
#define GAMESTATE_H_
#include <vector>
#include <SDL.h>
#include "objects/GameInst.h"
#include "controllers/MonsterController.h"
#include "controllers/PlayerController.h"
#include "GameSettings.h"
#include "GameView.h"
#include "GameHud.h"
#include "GameWorld.h"
#include "../display/font.h"
#include "../procedural/mtwist.h"
#include "../fov/fov.h"
#include "net/GameNetConnection.h"

struct lua_State;
struct GameLevelState;
struct GameTiles;

class GameState {
public:

	GameState(const GameSettings& settings, int width, int height, int vieww =
			640, int viewh = 480, int hudw = 160);
	~GameState();

	void init_game();
	void draw(bool drawhud = true);
	void step();
	bool pre_step();
	bool update_iostate(bool resetprev = true);

	GameInst *get_instance(obj_id id);
	obj_id add_instance(GameInst *inst);
	void remove_instance(GameInst *inst, bool deallocate = true);

	//All solid test functions return true if a solid object is hit
	bool tile_radius_test(int x, int y, int rad, bool issolid = true,
			int ttype = -1, Pos *hitloc = NULL);
	bool tile_line_test(int x, int y, int w, int h);
	int object_radius_test(GameInst *obj, GameInst **objs = NULL, int obj_cap =
			0, col_filterf f = NULL, int x = -1, int y = -1, int radius = -1);
	bool solid_test(GameInst *obj, GameInst **objs = NULL, int obj_cap = 0,
			col_filterf f = NULL, int x = -1, int y = -1, int radius = -1) {
		int lx = (x == -1 ? obj->x : x), ly = (y == -1 ? obj->y : y);
		if (radius == -1)
			radius = obj->radius;

		return tile_radius_test(lx, ly, radius)
				|| object_radius_test(obj, objs, obj_cap, f, x, y, radius);
	}
	bool solid_test(GameInst *obj, int x, int y, int radius = -1) {
		return solid_test(obj, NULL, 0, NULL, x, y, radius);
	}

	bool object_visible_test(GameInst *obj, GameInst *player = NULL);
	void ensure_connectivity(int roomid1, int roomid2);
	GameView & window_view() {
		return view;
	}

	GameTiles & tile_grid();
	GameWorld & game_world() {
		return world;
	}

	MonsterController & monster_controller();
	PlayerController & player_controller();

	const font_data & primary_font() {
		return pfont;
	}

	const font_data & menu_font() {
		return menufont;
	}

	int mouse_x() {
		return mousex;
	}

	int mouse_y() {
		return mousey;
	}

	bool mouse_left_click() {
		return mouse_leftclick;
	}

	bool mouse_right_click() {
		return mouse_rightclick;
	}

	bool mouse_left_down() {
		return mouse_leftdown;
	}

	bool mouse_right_down() {
		return mouse_rightdown;
	}

	bool mouse_upwheel() {
		return mouse_didupwheel;
	}

	bool mouse_downwheel() {
		return mouse_diddownwheel;
	}

	int & frame() {
		return frame_n;
	}

	obj_id local_playerid();

	int key_down_state(int keyval);
	int key_press_state(int keyval);
	int width() {
		return world_width;
	}

	int height() {
		return world_height;
	}

	GameLevelState*& level() {
		return world.get_current_level();
	}

	void serialize(FILE *file);
	MTwist & rng() {
		return mtwist;
	}

	void level_move(int id, int x, int y, int roomid1, int roomid2);
	GameSettings & game_settings() {
		return settings;
	}

	GameNetConnection & net_connection() {
		return connection;
	}

	lua_State *get_luastate() {
		return lua_state;
	}

	void skip_next_id();

private:
	GameSettings settings;
	std::vector<GameLevelState*> level_states;
	void restart();
	int handle_event(SDL_Event *event);
	int world_width, world_height;
	int frame_n;
	GameNetConnection connection;
	GameLevelState *lvl;
	GameHud hud;
	GameView view;
	GameWorld world;
	MTwist mtwist;
	font_data pfont, menufont;
	lua_State *lua_state;
	char key_down_states[SDLK_LAST];
	char key_press_states[SDLK_LAST];
	int mousex, mousey;
	bool mouse_leftdown, mouse_rightdown;
	bool mouse_leftclick, mouse_rightclick;
	bool mouse_didupwheel, mouse_diddownwheel;
};

#endif /* GAMESTATE_H_ */
