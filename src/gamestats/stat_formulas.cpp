/*
 * stat_formulas.cpp
 *  Represents formulas used in combat
 */

#include <cmath>

#include "../data/weapon_data.h"

#include "../world/GameState.h"

#include "combat_stats.h"
#include "stat_formulas.h"
#include "stats.h"

/* What power, resistance difference causes damage to be raised by 100% */
const int POWER_MULTIPLE_INTERVAL = 50;

static float damage_multiplier(float power, float resistance) {
	float powdiff = power - resistance;
	float intervals = powdiff / POWER_MULTIPLE_INTERVAL;
	if (intervals < 0) {
		//100% / (1+intervals)
		return 1.0f / (1.0f - intervals);
	} else {
		//100% + 100% * intervals
		return 1.0f + intervals;
	}
}

static int basic_damage_formula(const EffectiveAttackStats& attacker,
		const DerivedStats& defender) {
	float mult = damage_multiplier(attacker.power, defender.resistance);
	int base = attacker.damage - defender.reduction;
	if (base < 0)
		return 0;
	return round(mult * base);
}

static int physical_damage_formula(const EffectiveAttackStats& attacker,
		const EffectiveStats& defender) {
	return basic_damage_formula(attacker, defender.physical);
}

static int magic_damage_formula(const EffectiveAttackStats& attacker,
		const EffectiveStats& defender) {
	return basic_damage_formula(attacker, defender.magic);
}

int damage_formula(const EffectiveAttackStats& attacker,
		const EffectiveStats& defender) {
	float mdmg = magic_damage_formula(attacker, defender);
	float pdmg = physical_damage_formula(attacker, defender);

	return mdmg * attacker.magic_percentage
			+ pdmg * attacker.physical_percentage();
}

static void derive_from_equipment(MTwist& mt, EffectiveStats& effective,
		const Equipment& equipment) {
	CoreStats& core = effective.core;
	WeaponEntry& wentry = equipment.weapon.weapon_entry();
	effective.physical.resistance = core.defence / 2.5f;
	effective.magic.resistance = core.willpower / 2.5f;
	effective.physical.reduction = core.physical_reduction + core.defence / 2.0;
	effective.magic.reduction = core.magic_reduction + core.willpower / 2.0;
}

EffectiveStats effective_stats(GameState* gs, const CombatStats& stats) {
	lua_State* L = gs->get_luastate();
	EffectiveStats ret;
	ret.core = stats.core;
	ret.movespeed = stats.movespeed;
	derive_from_equipment(gs->rng(), ret, stats.equipment);
	stats.effects.process(L, stats, ret);
	return ret;
}

int experience_needed_formula(int xplevel) {
	float proportion = pow(xplevel, 1.75);
	return round(proportion) * 50 + 50;
}

