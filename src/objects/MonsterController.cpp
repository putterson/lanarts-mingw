/*
 * MonsterController.cpp:
 *  Centralized location of all pathing decisions of monsters, with collision avoidance
 */

#include <algorithm>
#include <cmath>

#include <rvo2/RVO.h>

#include <ldraw/draw.h>


#include "draw/colour_constants.h"
#include "draw/TileEntry.h"
#include "gamestate/GameState.h"

#include "gamestate/PlayerData.h"
#include "gamestate/TeamIter.h"

#include "stats/items/WeaponEntry.h"
#include "stats/effect_data.h"

#include <lcommon/math_util.h>

#include "objects/PlayerInst.h"
#include "objects/CombatGameInstFunctions.h"
#include "objects/collision_filters.h"

#include "EnemyInst.h"
#include "MonsterController.h"

const int HUGE_DISTANCE = 1000000;

MonsterController::MonsterController(bool wander) :
        monsters_wandering_flag(wander) {
}

MonsterController::~MonsterController() {
}

void MonsterController::register_enemy(GameInst* enemy) {
    mids.push_back(enemy->id);
    RVO::Vector2 enemy_position(enemy->x, enemy->y);
    EnemyInst* e = (EnemyInst*)enemy;
    EffectiveStats& estats = e->effective_stats();
    EnemyBehaviour& eb = e->behaviour();
}

void MonsterController::partial_copy_to(MonsterController & mc) const {
    mc.mids = this->mids;
//    mc.coll_avoid.clear();
}

void MonsterController::finish_copy(GameMapState* level) {
    for (int i = 0; i < mids.size(); i++) {
        EnemyInst* enemy = (EnemyInst*)level->game_inst_set().get_instance(
                mids[i]);
        if (!enemy)
            continue;
//        int simid = coll_avoid.add_object(enemy);
//        enemy->collision_simulation_id() = simid;
    }
}

void MonsterController::serialize(SerializeBuffer& serializer) {
    serializer.write_container(mids);
    serializer.write(monsters_wandering_flag);
}

void MonsterController::deserialize(SerializeBuffer& serializer) {
    serializer.read_container(mids);
    serializer.read(monsters_wandering_flag);
}

CombatGameInst* MonsterController::find_actor_to_target(GameState* gs, EnemyInst* e) {
    //Use a 'GameView' object to make use of its helper methods
    GameView view(0, 0, PLAYER_PATHING_RADIUS * 2, PLAYER_PATHING_RADIUS * 2,
            gs->width(), gs->height());

    //Determine which players we are currently in view of
    BBox ebox = e->bbox();
    int mindistsqr = HUGE_DISTANCE;
    CombatGameInst* closest_actor = NULL;
    for_all_enemies(gs->team_data(), e, [&](CombatGameInst* actor) {
        bool isvisible = is_visible(gs, e, actor);
        if (isvisible) {
            PlayerInst* p = NULL;
            // HACK TODO
            if ( (p = dynamic_cast<PlayerInst*>(actor) )) {
                p->rest_cooldown() = REST_COOLDOWN;
            }
        }
        view.sharp_center_on(actor->x, actor->y);
        bool chasing = e->behaviour().chase_timeout > 0
                && actor->id == e->behaviour().chasing_actor;
        bool forced_wander = (e->effects().get(get_effect_by_name("Dazed")));
        event_log("View %d %d\n", view.x, view.y);
        if (view.within_view(ebox) && (chasing || isvisible) && !forced_wander) {
            e->behaviour().current_action = EnemyBehaviour::CHASING_PLAYER;

            int dx = e->x - actor->x, dy = e->y - actor->y;
            int distsqr = dx * dx + dy * dy;
            event_log("Enemy id=%d name=%s considering target id=%d, dx=%d, dy=%d\n", e->id, e->etype().name.c_str(), actor->id, dx, dy);
            if (distsqr > 0 /*overflow check*/&& distsqr < mindistsqr) {
                mindistsqr = distsqr;
                closest_actor = actor;
            }
        }
    });
    return closest_actor;
}

void MonsterController::pre_step(GameState* gs) {
    perf_timer_begin(FUNCNAME);

    CollisionAvoidance& coll_avoid = gs->collision_avoidance();
    std::vector<EnemyOfInterest> eois;

    players = gs->players_in_level();

    //Update 'mids' to only hold live objects
    std::vector<obj_id> mids2;
    mids2.reserve(mids.size());
    mids.swap(mids2);

    for (int i = 0; i < mids2.size(); i++) {
        EnemyInst* e = (EnemyInst*)gs->get_instance(mids2[i]);
        if (e == NULL)
            continue;
        EnemyBehaviour& eb = e->behaviour();
        eb.step();

        //Add live instances back to monster id list
        mids.push_back(mids2[i]);

        CombatGameInst* actor = find_actor_to_target(gs, e);

        // Part of: Implement status effects.
        bool forced_wander = (e->effects().get(get_effect_by_name("Dazed")));
        if (forced_wander) {
            if (eb.current_action == EnemyBehaviour::CHASING_PLAYER) {
                eb.current_action = EnemyBehaviour::INACTIVE;
            }
        } else {
            if (eb.current_action == EnemyBehaviour::INACTIVE
                    && e->cooldowns().is_hurting()) {
                eb.current_action = EnemyBehaviour::CHASING_PLAYER;
            }
            if (actor == NULL
                    && eb.current_action == EnemyBehaviour::CHASING_PLAYER) {
                eb.current_action = EnemyBehaviour::INACTIVE;
                e->target() = NONE;
            }
        }


        if (actor != NULL && eb.current_action == EnemyBehaviour::CHASING_PLAYER) {
                event_log("Enemy id=%d has enemy of interest %d\n", e->id, actor->id);
                eois.push_back(
                    EnemyOfInterest(e, actor->id, inst_distance(e, actor))
                );
        } else {
            e->vx = 0, e->vy = 0;
            if (e->has_paths_data() && eb.current_action != EnemyBehaviour::FOLLOWING_PATH) {
                GameInst* inst = get_nearest_ally(gs, e);
                if (inst) {
                    if (distance_between(e->ipos(), inst->ipos()) > TILE_SIZE * 3) {
                        Pos p = e->direction_towards_ally_player(gs);
                        e->vx = p.x, e->vy = p.y;
                        float speed = e->effective_stats().movespeed;
                        normalize(e->vx, e->vy, speed);
                    }
                }
            }

            if (e->vx == 0 && e->vy == 0) {
                if (eb.current_action == EnemyBehaviour::INACTIVE) {
                        monster_wandering(gs, e);
                } else {
                        //if (eb.current_action == EnemyBehaviour::FOLLOWING_PATH)
                        monster_follow_path(gs, e);
                }
            }
        }
    }

    set_monster_headings(gs, eois);

    //Update player positions for collision avoidance simulator
    for (int i = 0; i < players.size(); i++) {
        PlayerInst* p = players[i];
        coll_avoid.set_position(p->collision_simulation_id(), p->x, p->y);
    }

    for (int i = 0; i < mids.size(); i++) {
        EnemyInst* e = (EnemyInst*)gs->get_instance(mids[i]);
                if (!e) {
                    continue;
                }
        lua_State* L = gs->luastate();
        lua_gameinst_callback(L, e->etype().step_event.get(L), e);
        update_velocity(gs, e);
        simul_id simid = e->collision_simulation_id();
        coll_avoid.set_position(simid, e->rx, e->ry);
        coll_avoid.set_preferred_velocity(simid, e->vx, e->vy);
    }

    coll_avoid.step();

    for (int i = 0; i < mids.size(); i++) {
        EnemyInst* e = (EnemyInst*)gs->get_instance(mids[i]);
                if (!e) {
                    continue;
                }
        update_position(gs, e);
    }

    perf_timer_end(FUNCNAME);
}

void MonsterController::update_velocity(GameState* gs, EnemyInst* e) {
    float movespeed = e->effective_stats().movespeed;

    bool has_fear = (e->effects().get(get_effect_by_name("Fear")));
    if (e->cooldowns().is_hurting()) {
        // Ogre mages in specific don't slow movement, and neither do fleeing enemies:
        if (e->etype().name == "Ogre Mage" || has_fear) {
        //// Hell forged's move faster, dissuading kiting:
        //} else if (e->etype().name == "Hell Forged") {
        //    e->vx *= 2, e->vy *= 2;
        //    movespeed *= 2;
        } else {
            e->vx /= 2, e->vy /= 2;
            movespeed /= 2;
        }
    }

    // Fear forces enemies to move backwards:
    if (has_fear) {
        e->vx *= -1, e->vy *= -1;
    }

    CollisionAvoidance& coll_avoid = gs->collision_avoidance();
    coll_avoid.set_preferred_velocity(e->collision_simulation_id(), e->vx,
            e->vy);
    coll_avoid.set_maxspeed(e->collision_simulation_id(), movespeed);
}
void MonsterController::update_position(GameState* gs, EnemyInst* e) {
    CollisionAvoidance& coll_avoid = gs->collision_avoidance();
    simul_id simid = e->collision_simulation_id();
    PosF pos = coll_avoid.get_position(simid);

    PosF new_xy = e->attempt_move_to_position(gs, pos);
    coll_avoid.set_position(simid, new_xy.x, new_xy.y);
    coll_avoid.set_maxspeed(simid, e->effective_stats().movespeed);
}

void MonsterController::post_draw(GameState* gs) {
    PlayerInst* player = gs->local_player();
    if (!player) {
        return;
    }
    EnemyInst* target = (EnemyInst*)gs->get_instance(player->target());
    if (!target) {
        return;
    }

    ldraw::draw_circle_outline(COL_GREEN.alpha(140),
            on_screen(gs, target->ipos()), target->target_radius + 5, 2);
}

void MonsterController::clear() {
    mids.clear();
    astarcontext = AStarPath();
}
