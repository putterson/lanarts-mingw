#include <lcommon/unittest.h>
#include <lcommon/lua_lcommon.h>

#include <luawrap/luawrap.h>
#include <luawrap/calls.h>
#include <luawrap/functions.h>

#include "stats/ClassEntry.h"

SUITE(ClassEntry_tests) {

	// TODO EVERYTHING

	static void set_class_metatable(lua_State* L, int idx) {
		lua_pushvalue(L, idx);

		lua_newtable(L);

		/* Set metatable of table at 'idx' */
		lua_setmetatable(L, -2);

		lua_pop(L, 1);
	}

	// TODO EVERYTHING
	static int class_create(lua_State* L) {
		LuaValue globals = luawrap::globals(L);
		LuaValue defined = globals["definitions"].ensure_table();
		LuaValue newclass = defined[lua_tostring(L, 1)].ensure_table();

		newclass.push();
		set_class_metatable(L, -1);

		return 1;
	}

	// TODO EVERYTHING
	TEST(proof_of_concept) {

		std::string program = "data.class_create {\n"
				"    name = \"Mage\","
				"    sprites = { \"wizard\", \"wizard2\" },\n"
				"   available_spells = {\n"
				"    },\n"
				"    start_stats = {\n"
				"        movespeed = 4,\n"
				"        hp = 110,\n"
				"        mp = 40,\n"
				"        hpregen = 3.3, --per second\n"
				"        mpregen = 2.64,\n"
				"        strength = 6,\n"
				"        defence = 6,\n"
				"        willpower = 3,\n"
				"        magic = 2,\n"
				"        equipment = {\n"
				"        }\n"
				"    },\n"
				"    on_level_gain = function(obj, stats)\n"
				"        stats.hp = stats.hp + 15\n"
				"        stats.mp = stats.mp + 20\n"
				"        stats.hpregen = stats.hpregen + 0.010\n"
				"        stats.mpregen = stats.mpregen + 0.007\n"
				"        stats.magic = stats.magic + 2\n"
				"        stats.strength = stats.strength + 1\n"
				"        stats.defence = stats.defence + 1\n"
				"        stats.willpower = stats.willpower + 2\n"
				"    end\n"
				"}";

		TestLuaState L;

		LuaValue globals = luawrap::globals(L);
		LuaValue defines = globals["definitions"].ensure_table();
		LuaValue data = globals["data"].ensure_table();

		data["class_create"].bind_function(class_create);

		lua_safe_dostring(L, program.c_str());
		CHECK(!defines["Mage"].isnil());

		LuaValue mage = defines["Mage"];

		CHECK(!mage["sprites"].isnil());
		CHECK(!mage["available_spells"].isnil());
		CHECK(!mage["start_stats"].isnil());
		CHECK(!mage["on_level_gain"].isnil());

		L.finish_check();
	}
}