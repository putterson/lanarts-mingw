/*
 * lua_gameworld.cpp:
 *  Query the state of the game world.
 */

#include <lua.hpp>
#include <luawrap/luawrap.h>
#include <luawrap/functions.h>
#include <luawrap/types.h>

#include <SDL.h>

#include "gamestate/GameState.h"
#include "gamestate/GameLevelState.h"
#include "gamestate/GameSettings.h"

#include "lua_newapi.h"

#include "lua/lua_api.h"

static int level_regenerate(lua_State* L) {
	GameState* gs = lua_api::gamestate(L);
	int levelid = gs->get_level()->id();

	if (lua_gettop(L) > 0) {
		levelid = lua_tonumber(L, 1);
	}

	gs->game_world().regen_level(levelid);
	return 0;
}

static int level_objects_list(lua_State* L) {
	GameState* gs = lua_api::gamestate(L);
	GameInstSet& insts = gs->get_level()->game_inst_set();
	luawrap::push(L, insts.to_vector());
	return 1;
}

static int level_objects(lua_State* L) {
	lua_pushcfunction(L, lua_api::l_itervalues);
	level_objects_list(L);
	lua_call(L, 1, 1);
	return 1;
}

// TODO add test for specific player
static bool level_object_visible(const LuaStackValue& obj) {
	GameState* gs = lua_api::gamestate(obj);
	return gs->object_visible_test(obj.as<GameInst*>());
}

static int level_object_place_free(lua_State* L) {
	int nargs = lua_gettop(L);
	GameState* gs = lua_api::gamestate(L);
	GameInst* inst = luawrap::get<GameInst*>(L, 1);
	Pos p = inst->pos();

	if (nargs > 1) {
		p = luawrap::get<Pos>(L, 2);
	}

	lua_pushboolean(L, !gs->solid_test(inst, p.x, p.y));
	return 1;
}

static bool level_place_free(const LuaStackValue& pos) {
	GameState* gs = lua_api::gamestate(pos);
	Pos p = pos.as<Pos>();
	return !gs->solid_test(NULL, p.x, p.y, 1);
}

static bool level_radius_place_free(const LuaStackValue& radius, const LuaStackValue& pos) {
	GameState* gs = lua_api::gamestate(radius);
	Pos p = pos.as<Pos>();
	return !gs->solid_test(NULL, p.x, p.y, radius.as<int>());
}

bool temporary_isgameinst(lua_State* L, int idx) {
	return lua_isuserdata(L, idx);
}

//TODO Figure out how other levels can be queried

namespace lua_api {
	void register_gameworld_api(lua_State* L) {
		LuaValue globals = luawrap::globals(L);
		luawrap::install_type<GameInst*, lua_push_gameinst, lua_gameinst_arg, temporary_isgameinst>();

		LuaValue world = globals["world"].ensure_table();
		LuaValue level = globals["level"].ensure_table();

		// Debug/special-case-only functions:
		level["regenerate"].bind_function(level_regenerate);

		// Query functions:
		level["objects_list"].bind_function(level_objects_list);
		level["objects"].bind_function(level_objects);

		level["object_visible"].bind_function(level_object_visible);
		level["object_place_free"].bind_function(level_object_place_free);
		level["place_free"].bind_function(level_place_free);
		level["radius_place_free"].bind_function(level_radius_place_free);
	}
}