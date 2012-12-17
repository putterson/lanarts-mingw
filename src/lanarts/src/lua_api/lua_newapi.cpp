/*
 * lua_newapi.cpp:
 *  New API, rename to lua_api once completed
 */

#include <luawrap/luawrap.h>

#include "lua_newapi.h"

static const char* gamestatekey = "gamestate";

namespace lua_api {	//
	// Register all the lanarts API functions and types
	static void register_gamestate(GameState* gs, lua_State* L) {
		lua_pushlightuserdata(L, (void*)(gamestatekey));
		lua_pushlightuserdata(L, gs);
		lua_settable(L, LUA_REGISTRYINDEX);
	}

	// Grab the GameState back-pointer
	// Used to implement lua_api functions that affect GameState
	GameState* gamestate(lua_State* L) {
		lua_pushlightuserdata(L, (void*)(gamestatekey));
		lua_gettable(L, LUA_REGISTRYINDEX);
		GameState* gs = (GameState*)(lua_touserdata(L, -1));
		lua_pop(L, 1);
		return gs;
	}


	// Convenience function that performs above on captured lua state
	GameState* gamestate(const LuaStackValue& val) {
		return gamestate(val.luastate());
	}

	// Register all the lanarts API functions and types
	void register_api(GameState* gs, lua_State* L) {
		LuaValue globals = luawrap::globals(L);
		LuaValue gamestate = globals["game"].ensure_table();

		register_gamestate(gs, L);

		register_io_api(L);
		register_net_api(L);
		register_gamestate_api(L);
		register_gameworld_api(L);
	}
}
