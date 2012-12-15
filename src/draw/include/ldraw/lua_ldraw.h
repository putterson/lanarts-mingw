/*
 * lua_ldraw.h:
 *  Bindings for the ldraw library in lua.
 */

#ifndef LDRAW_LUA_LDRAW_H_
#define LDRAW_LUA_LDRAW_H_

class LuaValue;

namespace ldraw {
void lua_register_ldraw(lua_State* L, const LuaValue& module);
}

#endif /* LDRAW_LUA_LDRAW_H_ */