/*
 * lua_newapi.cpp:
 *  New API, rename to lua_api once completed
 */

#include <luawrap/luawrap.h>
#include <luawrap/calls.h>
#include <luawrap/luameta.h>
#include <ldraw/lua_ldraw.h>

#include "lua_api/lua_yaml.h"
#include "lua_api/lua_api.h"

#include "lua_newapi.h"

// NB: the -address- of this key is used, not the actual string.
static char GAMESTATE_KEY[] = "";

namespace lua_api {

	void luacall_game_won(lua_State* L) {
		luawrap::globals(L)["engine"].push();
		lua_getfield(L, -1, "game_won");
		luawrap::call<void>(L);
		// pop engine table
		lua_pop(L, 1);
	}

	void event_player_death(lua_State* L, PlayerInst* player) {
		luawrap::globals(L)["engine"].push();
		lua_getfield(L, -1, "event_occurred");
		luawrap::call<void>(L, "PlayerDeath", (GameInst*) player);
		// pop engine table
		lua_pop(L, 1);
	}

	void event_projectile_hit(lua_State* L, ProjectileInst* projectile, EnemyInst* enemy) {
		luawrap::globals(L)["engine"].push();
		lua_getfield(L, -1, "event_occurred");
		luawrap::call<void>(L, "ProjectileHit", (GameInst*) projectile, (GameInst*) enemy);

	}
	void event_monster_death(lua_State* L, EnemyInst* enemy) {
		luawrap::globals(L)["engine"].push();
		lua_getfield(L, -1, "event_occurred");
		luawrap::call<void>(L, "MonsterDeath", (GameInst*) enemy);
		// pop engine table
		lua_pop(L, 1);
	}

	LuaValue global_getters(lua_State* L) {
		luawrap::globals(L).push();
		lua_getmetatable(L, -1);
		LuaValue globalsmeta = LuaValue::pop_value(L);
		return luameta_getters(globalsmeta);
	}

	LuaValue global_setters(lua_State* L) {
		luawrap::globals(L).push();
		lua_getmetatable(L, -1);
		LuaValue globalsmeta = LuaValue::pop_value(L);
		return luameta_setters(globalsmeta);
	}

	void add_search_path(lua_State* L, const char* path) {
		LuaValue package = luawrap::globals(L)["package"];
		package["path"].push();
		lua_pushfstring(L, "%s;%s", lua_tostring(L, -1), path);
		package["path"].pop();
		lua_pop(L, 1);
	}

	void require(lua_State* L, const char* path) {
		lua_getglobal(L, "require");
		lua_pushstring(L, path);
		lua_call(L, 1, 0);
	}

	static void configure_lua_packages(lua_State* L) {
		LuaValue package = luawrap::globals(L)["package"];
		lua_pushnil(L);
		package["loadlib"].pop(); // Remove for now
		package["path"] = "./?.lua";
	}

	/* Creates a lua state with a custom global metatable.
	 * All further registration assumes the lua state was created with this function. */
	lua_State* create_luastate() {
		lua_State* L = lua_open();

		LuaValue globalsmeta = luameta_new(L, "<GlobalVariables>");
		LuaValue contents = luameta_constants(globalsmeta);

		// Set contents as global table temporarily
		contents.push();
		lua_replace(L, LUA_GLOBALSINDEX);

		// Do openlibs before making globals table a proxy
		luaL_openlibs(L);

		configure_lua_packages(L);

		luameta_defaultsetter(globalsmeta, contents);
		luameta_defaultgetter(globalsmeta, contents);

		lua_newtable(L);

//		/* push global table */
		globalsmeta.push();
		lua_setmetatable(L, -2); /* set global meta-table*/

		lua_replace(L, LUA_GLOBALSINDEX);

		return L;
	}

	// Install types for use with luawrap
	void preinit_state(lua_State* L) {
		luawrap::install_type<YAML::Node, lua_pushyaml>();
		// NB: ldraw registers lcommon by default
		ldraw::lua_register_ldraw(L, luawrap::globals(L));
	}

	// Register all the lanarts API functions and types
	static void register_gamestate(GameState* gs, lua_State* L) {
		lua_pushlightuserdata(L, (void*)(GAMESTATE_KEY));
		lua_pushlightuserdata(L, gs);
		lua_settable(L, LUA_REGISTRYINDEX);
	}

	// Grab the GameState back-pointer
	// Used to implement lua_api functions that affect GameState
	GameState* gamestate(lua_State* L) {
		lua_pushlightuserdata(L, (void*)(GAMESTATE_KEY));
		lua_gettable(L, LUA_REGISTRYINDEX);
		GameState* gs = (GameState*)(lua_touserdata(L, -1));
		lua_pop(L, 1);
		return gs;
	}

	// Convenience function that performs above on captured lua state
	GameState* gamestate(const LuaStackValue& val) {
		return gamestate(val.luastate());
	}

	void globals_set_mutability(lua_State* L, bool mutability) {
		lua_getmetatable(L, LUA_GLOBALSINDEX); /* set global meta-table*/
		LuaValue globalsmeta = LuaValue::pop_value(L);

		LuaValue fallback = luameta_constants(globalsmeta);

		if (!mutability) {
			fallback.init(L); // set to 'nil'
		}

		luameta_defaultsetter(globalsmeta, fallback);
		luameta_defaultgetter(globalsmeta, fallback);
	}

	bool globals_get_mutability(lua_State* L) {
		lua_getmetatable(L, LUA_GLOBALSINDEX); /* set global meta-table*/
		LuaValue globalsmeta = LuaValue::pop_value(L);
		LuaValue fallback = luameta_constants(globalsmeta);
		return !fallback.isnil();
	}

	// Register all the lanarts API functions and types
	void register_api(GameState* gs, lua_State* L) {
		lua_lanarts_api(gs, L); // TODO: Deprecated

		LuaValue globals = luawrap::globals(L);
		LuaValue gamestate = globals["game"].ensure_table();
		// Holds engine hooks
		LuaValue engine = globals["engine"].ensure_table();

		register_gamestate(gs, L);
		register_general_api(L);

		register_io_api(L);
		register_net_api(L);
		register_gamestate_api(L);
		register_gameworld_api(L);
		register_display_api(L);
	}

	void luacall_postdraw(lua_State* L) {
		luawrap::globals(L)["postdraw"].push();
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
		} else {
			luawrap::call<void>(L);
		}
	}

	bool luacall_handle_event(lua_State* L, SDL_Event* e) {
		bool handled = false;

		luawrap::globals(L)["engine"].push();
		LuaStackValue(L, -1)["io_event_occurred"].push();

		if (!lua_isnil(L, -1)) {
			luawrap::push(L, e);
			lua_call(L, 1, 1);
			handled = lua_toboolean(L, -1);
		}

		lua_pop(L, 2); // pop game table & function/result

		return handled;
	}


	void luacall_hitsound(lua_State* L) {
		luawrap::globals(L)["play_hit_sound"].push();
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
		} else {
			luawrap::call<void>(L);
		}
	}

}
