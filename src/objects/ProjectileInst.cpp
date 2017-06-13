/* ProjectileInst.h:
 *  Represents a projectile attack's trajectory
 */

#include <cmath>
#include <cstdio>
#include <typeinfo>

#include <lua.hpp>

#include <luawrap/luawrap.h>

#include "draw/colour_constants.h"
#include "draw/SpriteEntry.h"
#include "gamestate/GameState.h"

#include "lua_api/lua_api.h"

#include "stats/items/ProjectileEntry.h"
#include "stats/items/WeaponEntry.h"

#include "stats/stat_formulas.h"

#include <lcommon/math_util.h>

#include "EnemyInst.h"

#include "PlayerInst.h"

#include "AnimatedInst.h"
#include "ItemInst.h"
#include "ProjectileInst.h"
#include "collision_filters.h"

static lsound::Sound minor_missile_sound;
void play(lsound::Sound& sound, const char* path);

ProjectileInst::~ProjectileInst() {
}

void ProjectileInst::draw(GameState* gs) {
	GameView& view = gs->view();
	SpriteEntry& spr = game_sprite_data.get(sprite());
	int w = spr.width(), h = spr.height();
	int xx = x - w / 2, yy = y - h / 2;

	if (!view.within_view(xx, yy, w, h))
		return;
	if (!gs->object_visible_test(this))
		return;

	draw_sprite(view, sprite(), xx, yy, vx, vy, frame);
}

void ProjectileInst::deinit(GameState* gs) {
	ProjectileEntry& pentry = projectile.projectile_entry();
	int break_roll = gs->rng().rand(100);
	if (pentry.drop_chance > break_roll) {
		int nx = round_to_multiple(x, TILE_SIZE, true), ny = round_to_multiple(
				y, TILE_SIZE, true);
		ItemInst* item = new ItemInst(projectile.with_amount(1), Pos(nx, ny),
				origin_id, true /*auto-pickup*/);
		gs->add_instance(item);
	}
}

void ProjectileInst::copy_to(GameInst* inst) const {
	LANARTS_ASSERT(typeid(*this) == typeid(*inst));
	*(ProjectileInst*) inst = *this;
}

ProjectileInst::ProjectileInst(const Item& projectile,
		const EffectiveAttackStats& atkstats, obj_id origin_id,
		const Pos& start, const Pos& target, float speed, int range,
		obj_id sole_target, bool bounce, int hits, bool pass_through) :
				GameInst(start.x, start.y, projectile.projectile_entry().radius,
						false, DEPTH),
				rx(start.x),
				ry(start.y),
				speed(speed),
				origin_id(origin_id),
				sole_target(sole_target),
				projectile(projectile),
				atkstats(atkstats),
				range_left(range),
				bounce(bounce),
				hits(hits),
				damage_mult(1.0f),
				pass_through(pass_through) {
	direction_towards(start, target, vx, vy, speed);
}

ProjectileInst* ProjectileInst::clone() const {
	return new ProjectileInst(*this);
}

static void lua_hit_callback(lua_State* L, LuaValue& callback,
		const EffectiveAttackStats& atkstats, GameInst* projectile,
		GameInst* target) {
	if (!callback.empty() && !callback.isnil()) {
		callback.push();
		luawrap::push(L, projectile);
		luawrap::push(L, target);
		lua_push_effectiveattackstats(L, atkstats);
		lua_call(L, 3, 0);
	}
}

static bool enemy_filter(GameInst* g1, GameInst* g2) {
    static CombatGameInst* comparison = NULL;
    if (g1 == NULL) {
        comparison = dynamic_cast<CombatGameInst*>(g2);
        return false;
    }
    auto* c2 = dynamic_cast<CombatGameInst*>(g2);
    if (!c2) return false;
    return comparison->team != c2->team;
}

void ProjectileInst::step(GameState* gs) {
	lua_State* L = gs->luastate();

	Pos tile_hit;

	frame++;
	int newx = (int) round(rx + vx); //update based on rounding of true float
	int newy = (int) round(ry + vy);
	bool collides = gs->tile_radius_test(newx, newy, radius, true, -1,
			&tile_hit);
	if (bounce) {
		bool hitsx = gs->tile_radius_test(newx, y, radius, true, -1);
		bool hitsy = gs->tile_radius_test(x, newy, radius, true, -1);
		if (hitsy || hitsx || collides) {
			if (hitsx) {
				vx = -vx;
			}
			if (hitsy) {
				vy = -vy;
			}
			if (!hitsy && !hitsx) {
				vx = -vx;
				vy = -vy;
			}
		}
	} else if (collides) {
		gs->remove_instance(this);
	}

	x = (int) round(rx += vx); //update based on rounding of true float
	y = (int) round(ry += vy);

	range_left -= speed;

	GameInst* colobj = NULL;
	CombatGameInst* origin = (CombatGameInst*) gs->get_instance(origin_id);

	if (dynamic_cast<CombatGameInst*>(origin)) {
		if (sole_target)
			gs->object_radius_test(this, &colobj, 1, &bullet_target_hit2);
		else {
		    enemy_filter(NULL, origin);
			gs->object_radius_test(this, &colobj, 1, &enemy_filter);
		}
	}

        if (colobj) {
                    CombatGameInst* victim = (CombatGameInst*) colobj;
            event_log(
                    "ProjectileInst::step id=%d from player id=%d hit enemy id=%d\n",
                    id, origin->id, colobj->id);
            origin->signal_attacked_successfully();

            lua_hit_callback(L,
                    projectile.projectile_entry().action_func().get(L),
                    atkstats, this, victim);

            int damage = damage_formula(atkstats, victim->effective_stats());

            if (gs->game_settings().verbose_output) {
                char buff[100];
                snprintf(buff, 100, "Attack: [dmg %d pow %d mag %d%%] -> Damage: %d",
                        atkstats.damage, atkstats.power, int(atkstats.magic_percentage * 100),
                        damage);
                gs->game_chat().add_message(buff);

            }

            damage *= damage_mult;

			gs->for_screens([&]() {
				if (dynamic_cast<PlayerInst *>(origin) &&
					gs->local_player()->current_floor == dynamic_cast<PlayerInst *>(origin)->current_floor) {
					play(minor_missile_sound, "sound/minor_missile.ogg");
				}
			});
            if (!projectile.projectile_entry().deals_special_damage) {
                char buffstr[32];
                snprintf(buffstr, 32, "%d", damage);
                float rx = vx / speed * .5;
                float ry = vy / speed * .5;
                gs->add_instance(
                                new AnimatedInst(
                                                Pos(victim->x - 5 + rx * 5, victim->y + ry * 5), -1,
                                                25, PosF(rx, ry), PosF(), AnimatedInst::DEPTH,
                                                buffstr));
                if (victim->damage(gs, damage) && dynamic_cast<EnemyInst*>(victim) && dynamic_cast<EnemyInst*>(victim)->team != PLAYER_TEAM) {
                        PlayerInst* p = (PlayerInst*) origin;
                        PlayerData& pc = gs->player_data();
                        p->signal_killed_enemy();
                        double xpworth = ((EnemyInst*)victim)->xpworth();
                        double n_killed = (pc.n_enemy_killed(((EnemyInst*) victim)->enemy_type()) - 1) / pc.all_players().size();
                        int kills_before_stale = ((EnemyInst*) victim)->etype().kills_before_stale;
                        xpworth *= pow(0.9, n_killed * 25 / kills_before_stale); // sum(0.9**i for i in range(25)) => ~9.28x the monsters xp value over time
                        if (n_killed > kills_before_stale) {
                            xpworth = 0;
                        }
                        float multiplayer_bonus = 1.0f / ((1 + pc.all_players().size()/2.0f) / pc.all_players().size());

                        int amnt = round(xpworth * multiplayer_bonus / pc.all_players().size());

                        players_gain_xp(gs, amnt);
                        if (xpworth == 0) {
                            snprintf(buffstr, 32, "STALE", amnt);
                        } else {
                            snprintf(buffstr, 32, "%d XP", amnt);
                        }
                        gs->add_instance(
                                        new AnimatedInst(victim->ipos(), -1, 25, PosF(), PosF(),
                                                        AnimatedInst::DEPTH, buffstr, COL_GOLD));
                }
            }
        }

	if (colobj || range_left <= 0) {
		hits--;
		if (hits >= 0 && colobj) {
			MonsterController& mc = gs->monster_controller();
			int mindist = 200;
			if (sole_target == 0)
				damage_mult = 0.5;
			sole_target = NONE; //Clear target
			for (int i = 0; i < mc.monster_ids().size(); i++) {
				obj_id mid = mc.monster_ids()[i];
				auto* enemy = (EnemyInst*)gs->get_instance(mid);
				if (enemy && origin && enemy->team != origin->team && enemy != colobj) {

					float abs = distance_between(Pos(x, y),
							Pos(enemy->x, enemy->y));
					if (abs < 1)
						abs = 1;
					if (abs < mindist) {
						sole_target = mid;
						mindist = abs;
						direction_towards(Pos(x, y), Pos(enemy->x, enemy->y),
								vx, vy, speed);
					}
				}
			}
		}
		if ((hits == 0 || sole_target == 0) && !pass_through) {
			gs->add_instance(
					new AnimatedInst(ipos(), sprite(), 15, PosF(),
							PosF(vx, vy)));
			gs->remove_instance(this);
		}
	}

	// Trying to get floating point stability
	vx = round(vx * 256.0) / 256.0;
	vy = round(vy * 256.0) / 256.0;

	rx = round(rx * 256.0) / 256.0;
	ry = round(ry * 256.0) / 256.0;
	speed = round(speed * 256.0) / 256.0;
	event_log("ProjectileInst id=%d has rx=%f, ry=%f, vx=%f,vy=%f\n", id, rx,
			ry, vx, vy);
}

sprite_id ProjectileInst::sprite() const {
	ProjectileEntry& pentry = projectile.projectile_entry();
	return pentry.attack_sprite();
}

void ProjectileInst::serialize(GameState* gs, SerializeBuffer& serializer) {
	GameInst::serialize(gs, serializer);
	SERIALIZE_POD_REGION(serializer, this, rx, sole_target);
	projectile.serialize(gs, serializer);
	SERIALIZE_POD_REGION(serializer, this, atkstats, damage_mult);
}

void ProjectileInst::deserialize(GameState* gs, SerializeBuffer& serializer) {
	GameInst::deserialize(gs, serializer);
	DESERIALIZE_POD_REGION(serializer, this, rx, sole_target);
	projectile.deserialize(gs, serializer);
	DESERIALIZE_POD_REGION(serializer, this, atkstats, damage_mult);
}

bool ProjectileInst::bullet_target_hit2(GameInst* self, GameInst* other) {
	return ((ProjectileInst*) self)->sole_target == other->id;
}

