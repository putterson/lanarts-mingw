#include <fstream>

#include <lua.hpp>

#include "lua_api/lua_yaml.h"
#include "lua_api/lua_api.h"
#include "data/game_data.h"
#include "data/yaml_util.h"

#include "items/ItemEntry.h"

#include "load_stats.h"

using namespace std;

void parse_equipment_entry(lua_State* L, const YAML::Node& n, EquipmentEntry& entry) {
	parse_item_entry(L, n, entry);
	entry.stackable = false;

	entry.use_action = LuaAction(LuaValue());
	entry.stat_modifiers = parse_stat_modifiers(n);
	entry.cooldown_modifiers = parse_cooldown_modifiers(n);
	std::string type = parse_defaulted(n, "equipment_type", std::string());
	if (type == "armour") {
		entry.type = EquipmentEntry::BODY_ARMOUR;
	} else if (type == "ring") {
		entry.type = EquipmentEntry::RING;
	} else if (type == "boots") {
		entry.type = EquipmentEntry::BOOTS;
    } else if (type == "helmet") {
        entry.type = EquipmentEntry::HEADGEAR;
    } else if (type == "amulet") {
        entry.type = EquipmentEntry::AMULET;
    } else if (type == "belt") {
        entry.type = EquipmentEntry::BELT;
    } else if (type == "legwear") {
        entry.type = EquipmentEntry::LEGWEAR;
	} else if (type == "gloves") {
		entry.type = EquipmentEntry::GLOVES;
	} else {
		LANARTS_ASSERT(type.empty());
	}
}

void load_equipment_callbackf(const YAML::Node& node, lua_State* L,
		LuaValue* value) {
	EquipmentEntry* entry = new EquipmentEntry;
	parse_equipment_entry(L, node, *entry);

	game_item_data.push_back(entry);
	/* Lua loading code */
	lua_pushyaml(L, node);
	lua_pushvalue(L, -1); // Duplicate
	(*value)[entry->name].pop();
	lua_pushstring(L, "armour");
	lua_setfield(L, -2, "type");
	lua_pop(L, 1);

}

void lapi_data_create_equipment(const LuaStackValue& table) {
	EquipmentEntry* entry = new EquipmentEntry;
	game_item_data.push_back(entry);

	int idx = game_item_data.size();
	entry->init(idx, table);
        LuaField equipment_table = luawrap::globals(table.luastate())["items"];
	equipment_table[entry->name] = table;
}
