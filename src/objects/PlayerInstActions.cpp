/*
 * PlayerInstActions.cpp
 *  Implements the various actions that the player can perform, handles keyboard & mouse IO
 *  as well as networking communication of actions
 */

#include <limits>

#include <lua.hpp>

#include <luawrap/luawrap.h>

#include <lcommon/strformat.h>

#include "draw/draw_sprite.h"
#include "draw/SpriteEntry.h"
#include "draw/TileEntry.h"

#include "draw/colour_constants.h"

#include "gamestate/GameState.h"
#include "lua_api/lua_api.h"

#include "stats/items/ItemEntry.h"

#include "stats/items/ProjectileEntry.h"
#include "stats/items/WeaponEntry.h"

#include "util/game_replays.h"
#include <lcommon/math_util.h>

#include "lanarts_defines.h"

#include "objects/EnemyInst.h"

#include "objects/StoreInst.h"

#include "objects/AnimatedInst.h"
#include "objects/FeatureInst.h"
#include "objects/ItemInst.h"
#include "objects/ProjectileInst.h"
#include "objects/collision_filters.h"

#include "PlayerInst.h"

static FeatureInst* find_usable_portal(GameState* gs, GameInst* player) {
	std::vector<GameInst*> instances = gs->get_level()->game_inst_set().to_vector();
	for (int i = 0; i < instances.size(); i++) {
		FeatureInst* inst = dynamic_cast<FeatureInst*>(instances[i]);
		if (inst == NULL) {
			continue;
		}
		if (inst->feature_type() == FeatureInst::PORTAL) {
			Pos sqr = Pos(inst->x / TILE_SIZE, inst->y /TILE_SIZE);
			BBox portal_box(Pos(sqr.x*TILE_SIZE, sqr.y*TILE_SIZE), Size(TILE_SIZE, TILE_SIZE));
			bool hit_test = circle_rectangle_test(player->ipos(), player->target_radius, portal_box);
			if (hit_test) {
				return inst;
			}
		}
	}
	return NULL;
}

static bool queue_portal_use(GameState* gs, PlayerInst* player, ActionQueue& queue) {
	FeatureInst* portal = find_usable_portal(gs, player);
	if (portal != NULL) {
		queue.push_back(
				game_action(gs, player, GameAction::USE_PORTAL));
		return true;
	}
	return false;
}


// Determines if a projectile should be wielded from the ground
static bool projectile_should_autowield(EquipmentStats& equipment,
		const Item& item, const std::string& preferred_class) {
	if (!item.is_projectile())
		return false;

	Projectile equipped_projectile = equipment.projectile();
	if (!equipped_projectile.empty()) {
		return equipped_projectile.is_same_item(item);
	}

	ProjectileEntry& pentry = item.projectile_entry();

	if (pentry.weapon_class == "unarmed") {
		return false;
	}

	if (pentry.weapon_class == preferred_class) {
		if (projectile_compatible_weapon(equipment.inventory, item) != -1) {
			return true;
		}
	}

	return equipment.valid_to_use(item);
}

const int AUTOUSE_HEALTH_POTION_THRESHOLD = 20;

void PlayerInst::enqueue_io_equipment_actions(GameState* gs,
		bool do_stopaction) {
	GameView& view = gs->view();
	bool mouse_within = gs->mouse_x() < gs->view().width;
	int rmx = view.x + gs->mouse_x(), rmy = view.y + gs->mouse_y();

	int level = gs->get_level()->id();
	int frame = gs->frame();
	bool item_used = false;
	IOController& io = gs->io_controller();

	Pos p(gs->mouse_x() + view.x, gs->mouse_y() + view.y);
	obj_id target = this->current_target;
	GameInst* targetted = gs->get_instance(target);
	if (targetted)
		p = Pos(targetted->x, targetted->y);

// We may have already used an item eg due to auto-use of items
	bool used_item = false;

//Item use
	IOEvent::event_t item_events[] = {IOEvent::USE_ITEM_N, IOEvent::SELL_ITEM_N};
	for (int i = 0; i < 9 && !used_item; i++) {
	    for (auto& event : item_events) {
            if (io.query_event(IOEvent(event, i))) {
                if (inventory().get(i).amount() > 0) {
                    item_used = true;
                    GameAction::action_t action_type = (event == IOEvent::USE_ITEM_N ? GameAction::USE_ITEM : GameAction::SELL_ITEM);
                    queued_actions.push_back(
                            GameAction(id, action_type, frame, level, i, p.x, p.y));
                }
            }
	    }
	}
	if (!used_item && gs->game_settings().autouse_health_potions
			&& core_stats().hp < AUTOUSE_HEALTH_POTION_THRESHOLD) {
		int item_slot = inventory().find_slot(
				get_item_by_name("Health Potion"));
		if (item_slot > -1) {
			queued_actions.push_back(
					game_action(gs, this, GameAction::USE_ITEM, item_slot));
			used_item = true;
		}
	}

//Item pickup
	GameInst* inst = NULL;
	if (cooldowns().can_pickup()
			&& gs->object_radius_test(this, &inst, 1, &item_colfilter)) {
		ItemInst* iteminst = (ItemInst*)inst;
		Item& item = iteminst->item_type();

		bool was_dropper = iteminst->last_held_by() == id;
		bool dropper_autopickup = iteminst->autopickup_held();

		bool autopickup = (item.is_normal_item() && !was_dropper
				&& !dropper_autopickup) || (was_dropper && dropper_autopickup);

		bool wieldable_projectile = projectile_should_autowield(equipment(),
				item, this->last_chosen_weaponclass);

//		bool pickup_io = gs->key_down_state(SDLK_LSHIFT)
//				|| gs->key_down_state(SDLK_RSHIFT);

		bool pickup_io = false; // No dedicated pickup key for now
		if (do_stopaction || wieldable_projectile || pickup_io || autopickup)
			queued_actions.push_back(
					GameAction(id, GameAction::PICKUP_ITEM, frame, level,
							iteminst->id));
	}
}

static Pos follow(FloodFillPaths& paths, const Pos& from_xy) {
        FloodFillNode* node = paths.node_at(from_xy);
        return Pos(from_xy.x + node->dx, from_xy.y + node->dy);
}
// TODO find appropriate place for this function
static bool has_visible_monster(GameState* gs, PlayerInst* p = NULL) {
    return get_nearest_visible_enemy(gs, p) != NULL; // TODO evaluate
    //const std::vector<obj_id>& mids = gs->monster_controller().monster_ids();
    //for (int i = 0; i < mids.size(); i++) {
    //    GameInst* inst = gs->get_instance(mids[i]);
    //    if (inst && gs->object_visible_test(inst, p)) {
    //        return true;
    //    }
    //}
    //return false;
}

Pos PlayerInst::direction_towards_unexplored(GameState* gs) {
    static bool LAST_WAS_STOP = false;
    Pos closest = {-1,-1};
    float min_dist = 10000;//std::numeric_limits<float>::max();
    bool found_item = false;
    int dx = 0, dy = 0;
    if (min_dist == 10000) {//std::numeric_limits<float>::max()) {
        FOR_EACH_BBOX(paths_to_object().location(), x, y) {
            auto* node = paths_to_object().node_at({x, y});
            if (node->solid) {
                continue;
            }
            float dist = node->distance;
            if (dist == 0 && (node->dx != 0 || node->dy != 0)) {
                continue;
            }
            bool near_unseen = false;
            for (int dy = -1; dy <= +1; dy++) {
                for (int dx = -1; dx <= +1; dx++) {
                    if (!(*gs->tiles().previously_seen_map())[{x+dx,y+dy}]) {
                        near_unseen = true;
                        break;
                    }
                }
            }
            // " Ludamad: Automatic item picking up was interesting but way too good.
            //   Dashing into a dungeon and quickly picking up items around should be a conscious effort by the player."
            // Ludamad: OK, let's try with just stackable items.
            bool is_item = (gs->object_radius_test(this, NULL, 0, &autopickup_colfilter, (x*32+16), (y*32+16), 1));
            if (near_unseen || is_item) {
                if (dist != 0 && (min_dist >= dist || (is_item > found_item)) && (is_item >= found_item)) {
                    closest = {x,y};
                    min_dist = dist;
                    found_item = is_item;
                }
            }
        }
    }
    if (LAST_WAS_STOP) {
        dx = rand() % 3 - 1;
        dy = rand() % 3 - 1;
        // FIXED SYNC BUG: Do not use rng().rand() here.
        // dx = gs->rng().rand(Range {-1, +1});
        // dy = gs->rng().rand(Range {-1, +1});
    } else if (gs->object_radius_test(this, NULL, 0, &autopickup_colfilter)) {
        dx = 0, dy = 0;
    } else if (min_dist != 10000) {//std::numeric_limits<float>::max()) {
        Pos iter = closest;
        Pos next_nearest;
        while (true) {
           Pos next = follow(paths_to_object(), iter);
           auto* next_node = paths_to_object().node_at(next);
           if (next_node->distance == 0) {
               next_nearest = iter;
                break;
           }
           iter = next;
        }
        dx = (next_nearest.x * TILE_SIZE + TILE_SIZE / 2) - x;
        dy = (next_nearest.y * TILE_SIZE + TILE_SIZE / 2) - y;
        if (abs(dx) < effective_stats().movespeed) {
            dx = 0;
        }
        if (abs(dy) < effective_stats().movespeed) {
            dy = 0;
        }
        if (abs(dx) + abs(dy) < effective_stats().movespeed * 2) {
            dx = 0;
            dy = 0;
        }
        if (abs(dx) > 0) dx /= abs(dx);
        if (abs(dy) > 0) dy /= abs(dy);
    }
    LAST_WAS_STOP = (min_dist != 10000) && (dx == 0 && dy == 0);
    return {dx, dy};
}

void PlayerInst::enqueue_io_movement_actions(GameState* gs, int& dx, int& dy) {
//Arrow/wasd movement
	if (gs->key_down_state(SDLK_UP) || gs->key_down_state(SDLK_w)) {
		dy -= 1;
	}
	if (gs->key_down_state(SDLK_RIGHT) || gs->key_down_state(SDLK_d)) {
		dx += 1;
	}
	if (gs->key_down_state(SDLK_DOWN) || gs->key_down_state(SDLK_s)) {
		dy += 1;
	}
	if (gs->key_down_state(SDLK_LEFT) || gs->key_down_state(SDLK_a)) {
		dx -= 1;
	}
        bool explore_used = false;
        if (dx == 0 && dy == 0) {
            if (gs->key_down_state(SDLK_e) && !has_visible_monster(gs, this)) {
                explore_used = true;
                Pos towards = direction_towards_unexplored(gs);
                if (towards != Pos{0,0}) {
                    dx = towards.x;
                    dy = towards.y;
                } else {
                    gs->game_chat().add_message("There is nothing to travel to in sight!",
                                    Colour(255, 100, 100));
                }
            } else if (gs->key_down_state(SDLK_e)) {
                gs->game_chat().add_message("Deal with the enemy before exploring!",
                                Colour(255, 100, 100));
            }
        }
	if (dx != 0 || dy != 0) {
		queued_actions.push_back(
				game_action(gs, this, GameAction::MOVE, 0, dx, dy, explore_used));
		moving = true;
	} else {
		moving = false;
	}
}

void PlayerInst::enqueue_actions(const ActionQueue& queue) {
	if (!actions_set_for_turn) {
		LANARTS_ASSERT(queued_actions.empty());
		queued_actions = queue;
		actions_set_for_turn = true;
	}
}

void PlayerInst::enqueue_io_actions(GameState* gs) {
	LANARTS_ASSERT(is_local_player() && gs->local_player() == this);

	if (actions_set_for_turn) {
		return;
	}

	bool single_player = (gs->player_data().all_players().size() <= 1);

	actions_set_for_turn = true;

	GameSettings& settings = gs->game_settings();
	GameView& view = gs->view();

	PlayerDataEntry& pde = gs->player_data().local_player_data();

	if (pde.action_queue.has_actions_for_frame(gs->frame())) {
		pde.action_queue.extract_actions_for_frame(queued_actions, gs->frame());
		event_log("Player %d has %d actions", player_entry(gs).net_id,
				(int) queued_actions.size());
		return;
	}
	if (!single_player) {
		gs->set_repeat_actions_counter(settings.frame_action_repeat);
	}

	int dx = 0, dy = 0;
	bool mouse_within = gs->mouse_x() < gs->view().width;
	int rmx = view.x + gs->mouse_x(), rmy = view.y + gs->mouse_y();

	bool was_moving = moving, do_stopaction = false;
	IOController& io = gs->io_controller();

	if (!settings.loadreplay_file.empty()) {
		load_actions(gs, queued_actions);
	}

	enqueue_io_movement_actions(gs, dx, dy);

	if (was_moving && !moving && cooldowns().can_do_stopaction()) {
		do_stopaction = true;
	}
//Shifting target
	if (gs->key_press_state(SDLK_k)) {
		shift_autotarget(gs);
	}

	if (gs->key_press_state(SDLK_m))
		spellselect = -1;

	bool attack_used = false;
	if (!gs->game_hud().handle_io(gs, queued_actions)) {
		attack_used = enqueue_io_spell_and_attack_actions(gs, dx, dy);
		enqueue_io_equipment_actions(gs, do_stopaction);
	}

	bool action_usage = io.query_event(IOEvent::ACTIVATE_SPELL_N)
			|| io.query_event(IOEvent::USE_WEAPON)
			|| io.query_event(IOEvent::AUTOTARGET_CURRENT_ACTION)
			|| io.query_event(IOEvent::MOUSETARGET_CURRENT_ACTION);
	if ((do_stopaction && !action_usage) || gs->key_down_state(SDLK_PERIOD)
			|| gs->mouse_downwheel()) {
		queue_portal_use(gs, this, queued_actions);
	}

// If we haven't done anything, rest
	if (queued_actions.empty()) {
		queued_actions.push_back(game_action(gs, this, GameAction::USE_REST));
	}

	ActionQueue only_passive_actions;

	for (int i = 0; i < queued_actions.size(); i++) {
		GameAction::action_t act = queued_actions[i].act;
		if (act == GameAction::MOVE || act == GameAction::USE_REST) {
			only_passive_actions.push_back(queued_actions[i]);
		}
	}

	GameNetConnection& net = gs->net_connection();
	if (net.is_connected()) {
		net_send_player_actions(net, gs->frame(),
				player_get_playernumber(gs, this), queued_actions);
	}

	int repeat = single_player ? 0 : settings.frame_action_repeat;
	for (int i = 1; i <= repeat; i++) {
		for (int j = 0; j < only_passive_actions.size(); j++) {
			only_passive_actions[j].frame = gs->frame() + i;
		}
		pde.action_queue.queue_actions_for_frame(only_passive_actions,
				gs->frame() + i);

		if (net.is_connected()) {
			net_send_player_actions(net, gs->frame() + i,
					player_get_playernumber(gs, this), only_passive_actions);
		}
	}

}

static bool item_do_lua_pickup(lua_State* L, ItemEntry& type, GameInst* user, int amnt) {
        if (!type.pickup_call.empty() && !type.pickup_call.isnil()) {
            type.pickup_call.push();
            luayaml_push_item(L, type.name.c_str());
            luawrap::push(L, user);
            lua_pushnumber(L, amnt);
            lua_call(L, 3, 1);
            bool ret = lua_toboolean(L, -1);
            lua_pop(L, 1);
            return ret;
        }
        return false;
}

void PlayerInst::pickup_item(GameState* gs, const GameAction& action) {
	const int PICKUP_RATE = 5;
	GameInst* inst = gs->get_instance(action.use_id);
	if (!inst) {
		return;
	}
	ItemInst* iteminst = dynamic_cast<ItemInst*>(inst);
	LANARTS_ASSERT(iteminst);

	const Item& type = iteminst->item_type();
	int amnt = iteminst->item_quantity();

	bool inventory_full = false;
        if (item_do_lua_pickup(gs->luastate(), type.item_entry(), this, amnt)) {
                // Do nothing, as commanded by Lua
        } else if (type.id == get_item_by_name("Gold")) {
		gold(gs) += amnt;
                //if (gs->local_player() == this) {
                    play("sound/gold.ogg");
                //}
	} else {
		itemslot_t slot = inventory().add(type);
		if (slot == -1) {
			inventory_full = true;
		} else if (projectile_should_autowield(equipment(), type,
				this->last_chosen_weaponclass)) {
			projectile_smart_equip(inventory(), slot);
		}
                //if (gs->local_player() == this) {
                    play("sound/item.ogg");
                //}
	}

	if (!inventory_full) {
		cooldowns().reset_pickup_cooldown(PICKUP_RATE);
		gs->remove_instance(iteminst);
	}
}

void PlayerInst::perform_queued_actions(GameState* gs) {
	perf_timer_begin(FUNCNAME);
	GameSettings& settings = gs->game_settings();

	if (settings.saving_to_action_file()) {
		save_actions(gs, queued_actions);
	}

	for (int i = 0; i < queued_actions.size(); i++) {
		perform_action(gs, queued_actions[i]);
	}
	queued_actions.clear();
	actions_set_for_turn = false;
	perf_timer_end(FUNCNAME);
}

void PlayerInst::drop_item(GameState* gs, const GameAction& action) {
	ItemSlot& itemslot = inventory().get(action.use_id);
	// use_id2 == 0 means drop all, use_id2 == 1 means drop half
	LANARTS_ASSERT(action.use_id2 == 0 || action.use_id2 == 1);
	int dropx = round_to_multiple(x, TILE_SIZE, true), dropy =
			round_to_multiple(y, TILE_SIZE, true);
	Item dropped_item = itemslot.item;
	if (action.use_id2 == 1) {
	    dropped_item.amount = std::max(1, dropped_item.amount / 2);
	}
	bool already_item_here = gs->object_radius_test(dropx, dropy,
			ItemInst::RADIUS, item_colfilter);
	if (!already_item_here) {
		gs->add_instance(new ItemInst(dropped_item, Pos(dropx, dropy), id));
		itemslot.deequip();
		itemslot.remove_copies(dropped_item.amount);
	}
	if (this->local) {
		gs->game_hud().reset_slot_selected();
	}
}

void PlayerInst::purchase_from_store(GameState* gs, const GameAction& action) {
	StoreInst* store = (StoreInst*)gs->get_instance(action.use_id);
	if (!store) {
		return;
	}
	LANARTS_ASSERT(dynamic_cast<StoreInst*>(gs->get_instance(action.use_id)));
	StoreInventory& inv = store->inventory();
	StoreItemSlot& slot = inv.get(action.use_id2);
	if (gold(gs) >= slot.cost) {
		inventory().add(slot.item);
		gold(gs) -= slot.cost;
		slot.item.clear();
                play("sound/inventory_sound_effects/sellbuy.ogg");
	}
}

void PlayerInst::reposition_item(GameState* gs, const GameAction& action) {
	ItemSlot& itemslot1 = inventory().get(action.use_id);
	ItemSlot& itemslot2 = inventory().get(action.use_id2);

	std::swap(itemslot1, itemslot2);
        if (action.use_id != action.use_id2) {
            play("sound/inventory_sound_effects/leather_inventory.ogg");
        }
	gs->game_hud().reset_slot_selected();
}

void PlayerInst::perform_action(GameState* gs, const GameAction& action) {
	event_log("Player id=%d performing act=%d, xy=(%d,%d), frame=%d, origin=%d, room=%d, use_id=%d, use_id2=%d\n",
			this->player_entry(gs).net_id,
			action.act, action.action_x,
			action.action_y, action.frame, action.origin, action.room,
			action.use_id, action.use_id2);
	switch (action.act) {
	case GameAction::MOVE:
		return use_move(gs, action);
	case GameAction::USE_WEAPON:
		return use_weapon(gs, action);
	case GameAction::USE_SPELL:
		return use_spell(gs, action);
	case GameAction::USE_REST:
		return use_rest(gs, action);
	case GameAction::USE_PORTAL:
		return use_dngn_portal(gs, action);
    case GameAction::USE_ITEM:
        return use_item(gs, action);
    case GameAction::SELL_ITEM:
        return sell_item(gs, action);
	case GameAction::PICKUP_ITEM:
		return pickup_item(gs, action);
	case GameAction::DROP_ITEM:
		return drop_item(gs, action);
	case GameAction::DEEQUIP_ITEM:
		return equipment().deequip_type(action.use_id);
	case GameAction::REPOSITION_ITEM:
		return reposition_item(gs, action);
	case GameAction::CHOSE_SPELL:
		spellselect = action.use_id;
		return;
	case GameAction::PURCHASE_FROM_STORE:
		return purchase_from_store(gs, action);
	default:
		printf("PlayerInst::perform_action() error: Invalid action id %d!\n",
				action.act);
		break;
	}
}

static bool item_check_lua_prereq(lua_State* L, ItemEntry& type,
		GameInst* user) {
	LuaValue& prereq = type.inventory_use_prereq_func().get(L);
	if (prereq.empty())
		return true;

	prereq.push();
	luayaml_push_item(L, type.name.c_str());
	luawrap::push(L, user);
	lua_call(L, 2, 1);

	bool ret = lua_toboolean(L, lua_gettop(L));
	lua_pop(L, 1);

	return ret;
}
static void item_do_lua_action(lua_State* L, ItemEntry& type, GameInst* user,
		const Pos& p, int amnt) {
	LuaValue& usefunc = type.inventory_use_func().get(L);
	usefunc.push();
	luayaml_push_item(L, type.name.c_str());
	luawrap::push(L, user);
	lua_pushnumber(L, p.x);
	lua_pushnumber(L, p.y);
	lua_pushnumber(L, amnt);
	lua_call(L, 5, 0);
}


void PlayerInst::use_item(GameState* gs, const GameAction& action) {
	if (!effective_stats().allowed_actions.can_use_items) {
		return;
	}
	itemslot_t slot = action.use_id;
	ItemSlot& itemslot = inventory().get(slot);
	Item& item = itemslot.item;
	ItemEntry& type = itemslot.item_entry();

	lua_State* L = gs->luastate();

	if (item.amount > 0) {
		if (item.is_equipment()) {
			if (itemslot.is_equipped()) {
				inventory().deequip(slot);
			} else {
				if (item.is_projectile()) {
					// Best-effort to equip, may not be possible:
					projectile_smart_equip(inventory(), slot);
				} else if (item.is_weapon()) {
					const Projectile& p = equipment().projectile();
					if (!p.empty()) {
						if (!p.projectile_entry().is_standalone()) {
							inventory().deequip_type(
									EquipmentEntry::AMMO);
						}
					}
					equipment().equip(slot);
					// Try and equip a projectile
					WeaponEntry& wentry = item.weapon_entry();
					if (wentry.uses_projectile) {
						const Projectile& p = equipment().projectile();
						if (p.empty()) {
							projectile_smart_equip(inventory(),
									wentry.weapon_class);
						}
					}
				} else {
					equipment().equip(slot);
				}

				if (item.is_weapon() || item.is_projectile()) {
					last_chosen_weaponclass =
							weapon().weapon_entry().weapon_class;
				}
			}
		} else if (equipment().valid_to_use(item)
				&& item_check_lua_prereq(L, type, this)) {
			item_do_lua_action(L, type, this,
					Pos(action.action_x, action.action_y), item.amount);
			if (is_local_player() && !type.inventory_use_message().empty()) {
				gs->game_chat().add_message(type.inventory_use_message(),
						Colour(100, 100, 255));
			}
			if (item.is_projectile())
				itemslot.clear();
			else
				item.remove_copies(1);
			reset_rest_cooldown();
		}
	}
}

void PlayerInst::sell_item(GameState* gs, const GameAction& action) {
    if (!effective_stats().allowed_actions.can_use_items) {
        return;
    }
    itemslot_t slot = action.use_id;
    ItemSlot& itemslot = inventory().get(slot);
    Item& item = itemslot.item;
    ItemEntry& type = itemslot.item_entry();

    if (item.amount > 0 && !itemslot.is_equipped()) {
        if (type.sellable) {
            int sell_amount = 1; // std::min(item.amount, 1); // TODO consider higher than 1 sell amounts
            int gold_gained = type.sell_cost() * sell_amount;
            auto message = format("Transaction: %s x %d for %d GP.", type.name.c_str(), sell_amount, gold_gained);
            item.remove_copies(sell_amount);
            gs->game_chat().add_message(message, COL_PALE_YELLOW);
            gold(gs) += gold_gained;
            play("sound/inventory_sound_effects/sellbuy.ogg");
        } else {
            gs->game_chat().add_message("Cannot sell this item!", COL_RED);
        }
    } else {
        gs->game_chat().add_message("Cannot sell currently equipped items!", COL_RED);
    }
}

void PlayerInst::use_rest(GameState* gs, const GameAction& action) {
        // Includes whether rest is currently legal (due to effects, not 'can rest' cooldown):
        if (!can_benefit_from_rest()) {
            return;
        }
        // Have we seen an enemy too recently?
        if (!cooldowns().can_rest()) {
            return;
        }
        CoreStats& ecore = effective_stats().core;
        int emax_hp = ecore.max_hp, emax_mp = ecore.max_mp;
        core_stats().heal_hp(ecore.hpregen * 8, emax_hp);
        core_stats().heal_mp(ecore.mpregen * 8, emax_mp);
        ecore.hp = core_stats().hp;
        ecore.mp = core_stats().mp;
        is_resting = true;
}
void PlayerInst::use_move(GameState* gs, const GameAction& action) {
	perf_timer_begin(FUNCNAME);

        // Get the effective move speed:
	float mag = effective_stats().movespeed;
	LANARTS_ASSERT(action.use_id2 == 0 || action.use_id2 == 1);
        if (action.use_id2 == 1) { // Did we use autoexplore?
            mag = std::min(3.0f, mag); // Autoexplore penalty
        }

        // Get the move direction:
	int dx = action.action_x, dy = action.action_y;
        // Multiply by the move speed to get the displacement. 
        // Note that players technically move faster when moving diagonally.
	float ddx = dx * mag, ddy = dy * mag;

        Pos newpos(round(rx + ddx), round(ry + ddy));

	if (!gs->tile_radius_test(newpos.x, newpos.y, radius)) {
		vx = ddx;
		vy = ddy;
	} else if (!gs->tile_radius_test(newpos.x, y, radius)) {
		vx = ddx;
	} else if (!gs->tile_radius_test(x, newpos.y, radius)) {
		vy = ddy;
	} else if (ddx != 0 && ddy != 0) {
		//Alternatives in opposite directions for x & y
		Pos newpos_alt1(round(vx + ddx), round(vy - ddy));
		Pos newpos_alt2(round(vx - ddx), round(vy + ddy));
		if (!gs->tile_radius_test(newpos_alt1.x, newpos_alt1.y, radius)) {
			vx += ddx;
			vy -= ddy;
		} else if (!gs->tile_radius_test(newpos_alt2.x, newpos_alt2.y,
				radius)) {
			vx -= ddx;
			vy += ddy;
		}

	}

    if (vx != 0 || vy != 0) {
        _last_moved_direction = PosF(vx, vy);
    }
        //Smaller radius enemy pushing test, can intercept enemy radius but not too far
	EnemyInst* alreadyhitting[5] = { NULL, NULL, NULL, NULL, NULL };
	gs->object_radius_test(this, (GameInst**)alreadyhitting, 5,
			&enemy_colfilter, x, y, radius);
	bool reduce_vx = false, reduce_vy = false;
	for (int i = 0; i < 5; i++) {
		if (alreadyhitting[i]) {
			if (vx < 0 == ((alreadyhitting[i]->x - x + vx * 2) < 0)) {
				reduce_vx = true;
			}
			if (vy < 0 == ((alreadyhitting[i]->y - y + vy * 2) < 0)) {
				reduce_vy = true;
			}
		}
	}
        if (reduce_vx) {
            vx *= 0.5;
        }
        if (reduce_vy) {
            vy *= 0.5;
        }
	event_log("Player id: %d using move for turn %d, vx=%f, vy=%f\n", id, gs->frame(), vx, vy);
	perf_timer_end(FUNCNAME);
}

void PlayerInst::use_dngn_portal(GameState* gs, const GameAction& action) {
	if (!effective_stats().allowed_actions.can_use_stairs) {
		if (is_local_player()) {
			gs->game_chat().add_message(
					"You cannot use the exit in this state!");
		}
		return;
	}

	FeatureInst* portal = find_usable_portal(gs, this);
        if (portal == NULL) {
            return;
        }
        if (gs->local_player()->current_floor == current_floor) {
            play("sound/stairs.ogg");
        }
	cooldowns().reset_stopaction_timeout(50);
	portal->player_interact(gs, this);
	reset_rest_cooldown();

	std::string subject_and_verb = "You travel";
	if (!is_local_player()) {
		subject_and_verb = player_entry(gs).player_name + " travels";
	}

	const std::string& map_label =
			gs->game_world().get_level(current_floor)->label();

	bool label_has_digit = false; // Does it have a number in the label?
	for (int i = 0; i < map_label.size(); i++) {
		if (isdigit(map_label[i])) {
			label_has_digit = true;
			break;
		}
	}

	gs->game_chat().add_message(
			format("%s to %s%s", subject_and_verb.c_str(),
					label_has_digit ? "" : "the ", map_label.c_str()),
			is_local_player() ? COL_WHITE : COL_YELLOW);
        if (map_label == "Plain Valley") {
            loop("sound/overworld.ogg");
        } else {
            loop("sound/dungeon.ogg");
        }
}

void PlayerInst::gain_xp(GameState* gs, int xp) {
	int levels_gained = stats().gain_xp(xp, this);
	if (levels_gained > 0) {
		char level_gain_str[128];
		snprintf(level_gain_str, 128, "%s reached level %d!",
				is_local_player() ? "You have" : "Your ally has",
				class_stats().xplevel);
		gs->game_chat().add_message(level_gain_str, Colour(50, 205, 50));
	}
}
