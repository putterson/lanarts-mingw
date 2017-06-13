/*
 * combat_stats.h:
 *  All the stats used by a combat entity.
 *  TODO: either rename this or 'stats.h' ?
 */

#ifndef COMBAT_STATS_H_
#define COMBAT_STATS_H_

#include <vector>

#include "EquipmentStats.h"
#include "SpellsKnown.h"
#include "effects.h"
#include "stats.h"

struct lua_State;
class CombatGameInst;
class GameState;
class SerializeBuffer;

/* Represents stats related to a single attack option */
struct AttackStats {
	AttackStats(Item weapon = Item(), Item projectile = Item()) :
			weapon(weapon), projectile(projectile) {
	}

	bool is_ranged() const;
	WeaponEntry& weapon_entry() const;
	ProjectileEntry& projectile_entry() const;

	int atk_cooldown() const;

	int atk_damage(MTwist& mt, const EffectiveStats& stats) const;
	int atk_power(MTwist& mt, const EffectiveStats& stats) const;
	float atk_percentage_magic() const;
	float atk_percentage_physical() const;

	/* members */
	Weapon weapon;
	Projectile projectile;
};

AttackStats parse_attack_stats(const LuaField& value);

/* Represents all the stats used by a combat entity */
struct CombatStats {
	CombatStats(const ClassStats& class_stats = ClassStats(),
			const CoreStats& core = CoreStats(),
			const CooldownStats& cooldowns = CooldownStats(),
			const EquipmentStats& equipment = EquipmentStats(),
			const std::vector<AttackStats>& attacks =
					std::vector<AttackStats>(), float movespeed = 0.0f);

	void init();
	void step(GameState* gs, CombatGameInst* inst,
			const EffectiveStats& effective_stats);

	bool has_died();

	EffectiveStats effective_stats(GameState* gs, CombatGameInst* inst) const;

	void gain_level(CombatGameInst* inst);
	int gain_xp(int amnt, CombatGameInst* inst);

	void serialize(GameState* gs, SerializeBuffer& serializer);
	void deserialize(GameState* gs, SerializeBuffer& serializer);

	/* members */
	CoreStats core;
	CooldownStats cooldowns;
	ClassStats class_stats;
	EquipmentStats equipment;
	SpellsKnown spells;

	std::vector<AttackStats> attacks;

	float movespeed;
};

CombatStats parse_combat_stats(const LuaField& value);

#endif /* COMBAT_STATS_H_ */
