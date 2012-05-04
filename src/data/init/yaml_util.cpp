#include "yaml_util.h"

std::vector<const YAML::Node*> flatten_seq_mappings(const YAML::Node & n) {
	std::vector<const YAML::Node*> ret;
	for (int i = 0; i < n.size(); i++) {
		if (n[i].Type() == YAML::NodeType::Map) {
			const YAML::Node & map = n[i];
			YAML::Iterator it = map.begin();

			for (; it != map.end(); ++it) {
				ret.push_back(&*it);
			}
		} else
			ret.push_back(&n[i]);

	}
	return ret;
}

int parse_sprite_number(const YAML::Node & n, const char *key) {
	if (!hasnode(n, key))
		return -1;

	std::string s;
	n[key] >> s;
	return get_sprite_by_name(s);
}

int parse_enemy_number(const YAML::Node & n, const char *key) {
	if (!hasnode(n, key))
		return -1;

	std::string s;
	n[key] >> s;
	for (int i = 0; i < game_enemy_data.size(); i++) {
		if (s == game_enemy_data[i].name) {
			return i;
		}
	}

	return -1;
}

const char *parse_cstr(const YAML::Node & n) {
	std::string s;
	n >> s;
	return tocstring(s);
}

StatModifier parse_modifiers(const YAML::Node & n) {
	StatModifier stat;
	optional_set(n, "strength", stat.strength_mult);
	optional_set(n, "defence", stat.defence_mult);
	optional_set(n, "magic", stat.magic_mult);
	return stat;
}

GenRange parse_range(const YAML::Node & n) {
	GenRange gr;
	if (n.Type() == YAML::NodeType::Sequence) {
		std::vector<int> components;
		n >> components;
		gr.min = components[0];
		gr.max = components[1];
	} else {
		n >> gr.min;
		gr.max = gr.min;
	}
	return gr;
}

void optional_set(const YAML::Node & node, const char *key, bool & value) {
	if (hasnode(node, key)) {
		int val;
		node[key] >> val;
		value = val;
	}
}

char* tocstring(const std::string & s) {
	char *ret = new char[s.size() + 1];
	memcpy(ret, s.c_str(), s.size() + 1);
	return ret;
}

bool hasnode(const YAML::Node & n, const char *key) {
	return n.FindValue(key);
}

Stats parse_stats(const YAML::Node & n, const std::vector<Attack> & attacks) {
	Stats ret_stats;
	n["movespeed"] >> ret_stats.movespeed;
	n["hp"] >> ret_stats.max_hp;
	ret_stats.max_mp = parse_defaulted(n, "mp", 0);
	ret_stats.hpregen = parse_defaulted(n, "hpregen", 0.0);
	ret_stats.mpregen = parse_defaulted(n, "mpregen", 0.0);
	ret_stats.hp = ret_stats.max_hp;
	ret_stats.mp = ret_stats.max_hp;
	ret_stats.strength = parse_defaulted(n, "strength", 0);
	ret_stats.defence = parse_defaulted(n, "defence", 0);
	ret_stats.magic = parse_defaulted(n, "magic", 0);
	ret_stats.xpneeded = parse_defaulted(n, "xpneeded", 125);
	ret_stats.xplevel = parse_defaulted(n, "xplevel", 1);
	for (int i = 0; i < attacks.size(); i++) {
		if (!attacks[i].isprojectile)
			ret_stats.meleeatk = attacks[i];

		if (attacks[i].isprojectile)
			ret_stats.magicatk = attacks[i];

	}
	return ret_stats;
}

