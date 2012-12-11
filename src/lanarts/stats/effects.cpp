extern "C" {
#include <lua/lua.h>
#include <lua/lauxlib.h>
}

#include <luawrap/luawrap.h>

#include <common/geometry.h>
#include <common/SerializeBuffer.h>
#include <common/lua/lua_serialize.h>

#include "../data/lua_game_data.h"
#include "../gamestate/GameState.h"

#include "../lua/lua_api.h"

#include "../objects/CombatGameInst.h"

#include "effect_data.h"

#include "effects.h"

bool EffectStats::has_active_effect() const {
	for (int i = 0; i < EFFECTS_MAX; i++) {
		if (effects[i].t_remaining > 0) {
			return true;
		}
	}
	return false;
}

static float draw_alpha(Effect& effect) {
	EffectEntry& eentry = game_effect_data.at(effect.effectid);
	if (!eentry.fades_out || effect.t_remaining > 100) {
		return 1.0f;
	}
	return effect.t_remaining / 100.0f;
}

Colour EffectStats::effected_colour() {
	int r = 255 * 256, g = 255 * 256, b = 255 * 256, a = 255 * 256;
	for (int i = 0; i < EFFECTS_MAX; i++) {
		if (effects[i].t_remaining > 0) {
			EffectEntry& eentry = game_effect_data.at(effects[i].effectid);
			Colour c = eentry.effected_colour.mute_colour(
					draw_alpha(effects[i]));
			r *= c.r + 1, g *= c.g + 1, b *= c.b + 1, a *= c.a + 1;
			r /= 256, g /= 256, b /= 256, a /= 256;
		}
	}
	return Colour(r / 256, g / 256, b / 256, a / 256);
}

void EffectStats::draw_effect_sprites(GameState* gs, const Pos& p) {
	GameView& view = gs->view();
	for (int i = 0; i < EFFECTS_MAX; i++) {
		if (effects[i].t_remaining > 0) {
			EffectEntry& eentry = game_effect_data.at(effects[i].effectid);
			if (eentry.effected_sprite > -1) {
				Colour drawcolour(255, 255, 255, 255 * draw_alpha(effects[i]));
				gl_draw_sprite(view, eentry.effected_sprite, p.x, p.y,
						drawcolour);
			}
		}
	}
}

bool EffectStats::can_rest() {
	for (int i = 0; i < EFFECTS_MAX; i++) {
		if (effects[i].t_remaining > 0) {
			EffectEntry& eentry = game_effect_data.at(effects[i].effectid);
			return false;
		}
	}
	return true;
}

AllowedActions EffectStats::allowed_actions(GameState* gs) const {
	AllowedActions actions;
	for (int i = 0; i < EFFECTS_MAX; i++) {
		if (effects[i].t_remaining > 0) {
			EffectEntry& ee = game_effect_data.at(effects[i].effectid);
			actions = actions.only_in_both(ee.allowed_actions);
		}
	}
	return actions;
}

void EffectStats::process(GameState* gs, CombatGameInst* inst,
		EffectiveStats& effective) const {
	if (!has_active_effect())
		return;
	lua_State* L = gs->get_luastate();
	lua_push_combatstats(L, inst->stats());
	lua_push_effectivestats(L, effective);

	int affind = lua_gettop(L);
	int baseind = affind - 1;

	for (int i = 0; i < EFFECTS_MAX; i++) {
		if (effects[i].t_remaining > 0) {
			game_effect_data.at(effects[i].effectid).stat_func.push(L);
			if (!lua_isnil(L, -1)) {
				effects[i].state.push(L);
				lua_push_gameinst(L, inst);

				lua_pushvalue(L, baseind);
				lua_pushvalue(L, affind);
				lua_call(L, 4, 0);
			} else {
				lua_pop(L, 1);
			}
		}
	}
//	basestats = lua_get_combatstats(L, baseind);
	effective = lua_get_effectivestats(L, affind);
	lua_pop(L, 2);
	//pop base&affected
}

static void lua_effect_func_callback(lua_State* L, LuaValue& value,
		Effect& effect, CombatGameInst* inst) {
	value.push(L);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	effect.state.push(L);
	lua_push_gameinst(L, inst);
	lua_call(L, 2, 0);
}

void EffectStats::step(GameState* gs, CombatGameInst* inst) {
	lua_State* L = gs->get_luastate();
	for (int i = 0; i < EFFECTS_MAX; i++) {
		Effect& e = effects[i];
		if (e.t_remaining > 0) {
			e.t_remaining--;

			EffectEntry& eentry = game_effect_data.at(e.effectid);

			e.state.get(L, "time_left") = e.t_remaining;
			lua_effect_func_callback(L, eentry.step_func, e, inst);
			e.t_remaining = e.state.get(L, "time_left");

			if (e.t_remaining == 0) {
				lua_effect_func_callback(L, eentry.finish_func, e, inst);
			}
		}
	}
}

Effect* EffectStats::get(int effect) {
	for (int i = 0; i < EFFECTS_MAX; i++) {
		if (effects[i].t_remaining > 0 && effects[i].effectid == effect) {
			return &effects[i];
		}
	}

	return NULL;
}

static void lua_init_metatable(lua_State* L, LuaValue& value,
		effect_id effect) {
	value.push(L);
	/* Set self as metatable*/
	lua_pushvalue(L, -1);
	lua_setmetatable(L, -2);

	/* Set index as effect object */
	EffectEntry& eentry = game_effect_data.at(effect);
	lua_effects.get(L, eentry.name).push();
	lua_setfield(L, -2, "__index");
	/* Pop self */
	lua_pop(L, 1);
}

static void lua_init_effect(lua_State* L, LuaValue& value, effect_id effect) {
	value.table_initialize(L);
	lua_init_metatable(L, value, effect);
}

LuaValue EffectStats::add(GameState* gs, CombatGameInst* inst, effect_id effect,
		int length) {
	lua_State* L = gs->get_luastate();
	for (int i = 0; i < EFFECTS_MAX; i++) {
		Effect& e = effects[i];
		if (e.t_remaining == 0 || e.effectid == effect) {
			e.effectid = effect;
			e.t_remaining = std::max(e.t_remaining, length);
			lua_init_effect(L, e.state, effect);

			e.state.get(L, "time_left") = e.t_remaining;

			EffectEntry& eentry = game_effect_data.at(e.effectid);
			lua_effect_func_callback(L, eentry.init_func, e, inst);
			return e.state;
		}
	}
	return LuaValue();
}

void EffectStats::serialize(GameState* gs, SerializeBuffer& serializer) {
	lua_State* L = gs->get_luastate();

	for (int i = 0; i < EFFECTS_MAX; i++) {
		serializer.write_int(effects[i].effectid);
		lua_serialize(serializer, L, effects[i].state);
		serializer.write_int(effects[i].t_remaining);
	}
}

void EffectStats::deserialize(GameState* gs, SerializeBuffer& serializer) {
	lua_State* L = gs->get_luastate();

	for (int i = 0; i < EFFECTS_MAX; i++) {
		serializer.read_int(effects[i].effectid);
		lua_deserialize(serializer, L, effects[i].state);
		if (!effects[i].state.isnil(L)) {
			lua_init_metatable(L, effects[i].state, effects[i].effectid);
		}
		serializer.read_int(effects[i].t_remaining);
	}
}
