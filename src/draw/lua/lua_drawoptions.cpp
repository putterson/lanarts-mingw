/*
 * lua_drawoptions.cpp:
 *  Bindings for drawing options <-> lua table
 */

#include <common/lua/slb_valuetype.h>
#include <common/lua/lua_geometry.h>
#include <lua/lua.h>

#include "lua_colour.h"
#include "lua_drawoptions.h"

void lua_push_drawoptions(lua_State *L, const ldraw::DrawOptions & options) {
	lua_newtable(L);
	int tblidx = lua_gettop(L);

	SLB::table_set(L, tblidx, "origin", int(options.draw_origin));
	SLB::table_set(L, tblidx, "colour", options.draw_colour);
	SLB::table_set(L, tblidx, "region", options.draw_region);
}

ldraw::DrawOptions lua_get_drawoptions(lua_State *L, int idx) {
	ldraw::DrawOptions options;
	SLB::table_get<int>(L, idx, "origin", options.draw_origin);
	SLB::table_get(L, idx, "colour", options.draw_colour);
	SLB::table_get(L, idx, "region", options.draw_region);
	return options;
}

bool lua_check_drawoptions(lua_State *L, int idx) {
	return lua_istable(L, idx);
}

void lua_register_draworigin_constants(lua_State *L, SLB::Table* table) {
	using namespace ldraw;
#define BIND_ORIGIN_CONST(origin) \
	lua_pushstring(L, #origin);\
	lua_pushnumber(L, int(origin)); \
	table->setCache(L)

	BIND_ORIGIN_CONST(LEFT_TOP);
	BIND_ORIGIN_CONST(LEFT_CENTER);
	BIND_ORIGIN_CONST(LEFT_BOTTOM);
	BIND_ORIGIN_CONST(CENTER_TOP);
	BIND_ORIGIN_CONST(CENTER);
	BIND_ORIGIN_CONST(CENTER_BOTTOM);
	BIND_ORIGIN_CONST(RIGHT_TOP);
	BIND_ORIGIN_CONST(RIGHT_CENTER);
	BIND_ORIGIN_CONST(RIGHT_BOTTOM);

}

