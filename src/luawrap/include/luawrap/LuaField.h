/**
 * LuaField.h:
 *	PLEASE READ BEFORE USING.
 *
 *  Represents a value inside another lua object.
 *  This is represented by an index chain of sorts and *MUST* be stored on the stack.
 *  There is some trade-off of usability and performance here, but generally it is very fast.
 *
 *  It should NOT be used as a class member. It primarily exists as a proxy convenience class.
 *
 *  You have been warned.
 */

#ifndef LUAFIELD_H_
#define LUAFIELD_H_

#include <cstdio>
#include <lua.hpp>
#include <string>

struct lua_State;

class LuaStackValue;
class LuaValue;

/* Represents a field inside an object.
 * ONLY FOR STACK USE. Please read notes above before using! */
class LuaField {
public:
	/** Constructors **/

	/* Looks up an object in the registry */
	LuaField(lua_State* L, void* parent, const char* index);
	LuaField(lua_State* L, void* parent, int index);

	/* Looks up another LuaField */
	LuaField(const LuaField* parent, const char* index);
	LuaField(const LuaField* parent, int index);

	/* Looks up a lua stack value */
	LuaField(lua_State* L, int stackidx, const char* index);
	LuaField(lua_State* L, int stackidx, int index);

	/** Getters **/
	lua_State* luastate() const {
		return L;
	}

	/* Returning path to object */
	std::string index_path() const;
	void index_path(std::string& str) const;

	/** Stack methods **/
	void push() const;
	void pop() const;

	/** Container methods **/
	LuaField operator[](const char* key) const;
	LuaField operator[](int index) const;

	void operator=(const LuaField& field);
	void operator=(const LuaValue& value);

	/** Luawrap convenience methods **/
	template<typename T>
	void operator=(const T& value);

	operator LuaValue() const;

	template<typename T>
	T as() const;

	template<typename T>
	bool is() const;

	const LuaField& assert_not_nil() const;

	void bind_function(lua_CFunction luafunc) const;
	template<typename Function>
	void bind_function(const Function& function) const;

	/** Lua api convenience methods **/
	bool has(const char* key) const;
	void newtable() const;
	void set_nil() const;
	bool isnil() const;
	int objlen() const;
	void* to_userdata() const;
	double to_num() const;
	bool to_bool() const;
	int to_int() const;
	const char* to_str() const;

	LuaValue metatable() const;

private:
	void error_and_pop(const std::string& expected_type) const;
	void handle_nil_parent() const;
	void push_parent() const;

	union Index {
		int integer;
		const char* string;
	};

	union Parent {
		const LuaField* field;
		void* registry;
		int stack_index;
	};

	void init(lua_State* L, char index_type, char parent_type, Index index,
			Parent parent);

	/* members */
	lua_State* L;
	char _index_type; // 0 for int, 1 for string
	char _parent_type; // 0 for field, 1 for registry, 2 for stack

	Index _index;
	Parent _parent;
};

#endif /* LUAFIELD_H_ */
