/*
 * LuaDrawable.cpp:
 *  Represents a drawable that encapsulates a lua closure and a draw duration.
 *  A draw duration of 0 (or nil in lua) signifies a non-animated drawable.
 */

#include <common/lua/lua_geometry.h>

#include <luawrap/luawrap.h>
#include <luawrap/calls.h>

#include "lua/lua_drawoptions.h"

#include "LuaDrawable.h"

namespace ldraw {


LuaDrawable::LuaDrawable(lua_State *L, const LuaValue & draw_closure,
		float _animation_duration) :
		L(L), draw_closure(draw_closure), _animation_duration(
				_animation_duration) {
}

LuaDrawable::LuaDrawable(lua_State* L, const std::string& draw_closure_src,
		float _animation_duration) :
		L(L), draw_closure_src(draw_closure_src), _animation_duration(
				_animation_duration) {
}

LuaDrawable::~LuaDrawable() {
}

void LuaDrawable::_push_closure() const {
	// Cache the result of evaluating the Lua string:
	if (!draw_closure_src.empty()) {
		draw_closure = luawrap::eval(L, draw_closure_src);
		draw_closure_src = std::string();
	}

	draw_closure.push(L);
}

void LuaDrawable::draw(const DrawOptions & options, const Posf & pos) const {
	_push_closure();
	luawrap::call<void>(L, options, pos);
}

bool LuaDrawable::operator ==(const LuaDrawable & o) const {
	if (L != o.L || animation_duration() != o.animation_duration()) {
		return false;
	}

	_push_closure();
	o._push_closure();
	bool eq = lua_equal(L, -1, -2);
	lua_pop(L, 2);
	return eq;
}

bool LuaDrawable::operator !=(const LuaDrawable & o) const {
	return !(*this == o);
}

}
