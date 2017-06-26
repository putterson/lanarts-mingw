/*
 * ProjectileEntry.h:
 *  Represents spell/weapon/enemy's projectile data loaded from the yaml
 */

#ifndef PROJECTILEENTRY_H_
#define PROJECTILEENTRY_H_

#include "../Attack.h"

#include "EquipmentEntry.h"

class ProjectileEntry: public EquipmentEntry {
public:
	ProjectileEntry() :
			radius(0), drop_chance(0), speed(0), number_of_target_bounces(0), can_wall_bounce(
					false) {
	}
	virtual ~ProjectileEntry() {
	}

	virtual void initialize(lua_State* L) {
		EquipmentEntry::initialize(L);
		attack.init(L);
	}

	CoreStatMultiplier& damage_stats() {
		return attack.damage_stats();
	}

	CoreStatMultiplier& power_stats() {
		return attack.power_stats();
	}

	float magic_percentage() {
		return attack.magic_percentage();
	}
	float physical_percentage() {
		return attack.physical_percentage();
	}

	int range() {
		return attack.range;
	}

	int cooldown() {
		return attack.cooldown;
	}
	sprite_id attack_sprite() {
		return attack.attack_sprite;
	}
	LuaLazyValue& action_func() {
		return attack.attack_action.action_func;
	}

	bool is_standalone() const {
		return weapon_class == "unarmed" || weapon_class == "magic";
	}

    virtual void parse_lua_table(const LuaValue& table);

	std::string weapon_class;
	Attack attack;
	int radius, drop_chance, speed;
	//XXX: remove these and put into lua
	int number_of_target_bounces;
	bool deals_special_damage = false;
	bool can_wall_bounce;
	bool can_pass_through = false;
    LuaValue attack_stat_func;
	LuaValue projectile_metatable;
};

projectile_id get_projectile_by_name(const char* name);
ProjectileEntry& get_projectile_entry(projectile_id id);

#endif /* PROJECTILEENTRY_H_ */
