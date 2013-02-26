/*
 * load_class_data.cpp:
 *  Load class stats and experience data
 */

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "lua_api/lua_newapi.h"

#include <luawrap/luawrap.h>
#include <luawrap/functions.h>

#include <lcommon/lua_lcommon.h>

#include "data/game_data.h"
#include "data/parse.h"

#include "objects/enemy/EnemyEntry.h"

#include "lua_api/lua_yaml.h"

#include "../ClassEntry.h"
#include "../stats.h"

#include "load_stats.h"

using namespace std;

ClassSpell parse_class_spell(const YAML::Node& n) {
	ClassSpell spell;

	std::string spellname = parse_str(n["spell"]);
	spell.spell = get_spell_by_name(spellname.c_str());
	spell.xplevel_required = parse_int(n["level_needed"]);

	return spell;
}

ClassSpellProgression parse_class_spell_progression(const YAML::Node& n) {
	ClassSpellProgression progression;

	for (int i = 0; i < n.size(); i++) {
		progression.available_spells.push_back(parse_class_spell(n[i]));
	}

	return progression;
}

ClassEntry parse_class(const YAML::Node& n) {

//	std::string name;
//	Stats starting_stats;
//	int hp_perlevel, mp_perlevel;
//	int str_perlevel, def_perlevel, mag_perlevel;
//	float mpregen_perlevel, hpregen_perlevel;
	ClassEntry classtype;

	const YAML::Node& level = n["gain_per_level"];

	n["name"] >> classtype.name;
	classtype.spell_progression = parse_class_spell_progression(n["spells"]);
	classtype.starting_stats = parse_combat_stats(n["start_stats"]);

	level["mp"] >> classtype.mp_perlevel;
	level["hp"] >> classtype.hp_perlevel;

	level["strength"] >> classtype.str_perlevel;
	level["defence"] >> classtype.def_perlevel;
	level["magic"] >> classtype.mag_perlevel;
	level["willpower"] >> classtype.will_perlevel;

	level["mpregen"] >> classtype.mpregen_perlevel;
	level["hpregen"] >> classtype.hpregen_perlevel;

	const YAML::Node& sprites = n["sprites"];
	for (int i = 0; i < sprites.size(); i++) {
		classtype.sprites.push_back(parse_sprite_number(sprites[i]));
	}

	return classtype;
}

void load_class_callbackf(const YAML::Node& node, lua_State* L,
		LuaValue* value) {
	ClassEntry entry = parse_class(node);
	(*value)[entry.name] = node;
	game_class_data.push_back(entry);
}

static void lapi_data_create_class(const LuaStackValue& table) {
	ClassEntry entry;
	entry.init(entry.class_id, table);
	entry.class_id = game_class_data.size();
	game_class_data.push_back(entry);
}


LuaValue load_class_data(lua_State* L, const FilenameList& filenames) {
//	LuaValue data = luawrap::globals(L)["data"].ensure_table();
//	data["class_create"].bind_function(lapi_data_create_class);
//	lua_safe_dofile(L, "res/classes/classes.lua");

	LuaValue ret(L);
	ret.newtable();

	game_class_data.clear();
	load_data_impl_template(filenames, "classes", load_class_callbackf, L,
			&ret);

	for (int i = 0; i < game_class_data.size(); i++) {
		CombatStats& cstats = game_class_data[i].starting_stats;
		cstats.class_stats.classid = i;
		cstats.init();
	}

	return ret;
}
