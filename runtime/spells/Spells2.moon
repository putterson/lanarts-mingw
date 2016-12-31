EventLog = require "ui.EventLog"
GameObject = require "core.GameObject"
Map = require "core.Map"
Bresenham = require "core.Bresenham"
Display = require "core.Display"
SpellObjects = require "objects.SpellObjects"

ally_list = (inst) ->
    if not inst.is_enemy
        return Map.players_list() or {}
    else
        return Map.monsters_list() or {}

enemy_list = (inst) ->
    if inst.is_enemy
        return Map.players_list() or {}
    else
        return Map.monsters_list() or {}

Data.spell_create {
    name: "Ludaze"
    spr_spell: "spr_spells.ludaze"
    description: "Lightly dazes all enemies in sight." 
    mp_cost: 5
    cooldown: 0
    spell_cooldown: 800
    can_cast_with_held_key: true
    fallback_to_melee: false
    action_func: (caster, x, y) ->
        caster\add_effect("Ludaze", 30)
        play_sound "sound/ludaze.ogg"
        for mon in *(enemy_list caster)
            if not Map.object_visible(mon)
                continue
            mon\add_effect("Dazed", 100)
        if caster\is_local_player()
            EventLog.add("You daze all enemies in sight!", {200,200,255})
        elseif caster.name == "Your ally"
            EventLog.add(caster.name .. " dazes all enemies in sight!", {200,200,255})
    autotarget_func: (caster) -> caster.x, caster.y
    prereq_func: (caster) -> 
        if not caster\has_effect("Berserk") and not caster\has_effect("Exhausted")  and not caster\has_effect("Ice Form")
            for mon in *(enemy_list caster)
                if Map.object_visible(mon)
                    return true
            EventLog.add("No monsters in sight!", COL_PALE_RED)
            return false
        return false
}


Data.spell_create {
    name: "Ice Form"
    spr_spell: "spr_spells.iceform"
    description: "Initiates Ice Form, a powerful ability for safe dungeoneering, preventing attacks and spells, and lowering speed drastically, but providing near immunity for 10 seconds." 
    mp_cost: 40
    cooldown: 100
    spell_cooldown: 1600
    can_cast_with_held_key: false
    fallback_to_melee: false
    action_func: (caster, x, y) ->
        caster\add_effect("Ice Form", 300)
        if caster\is_local_player()
            EventLog.add("You enter a glacial state!", {200,200,255})
        elseif caster.name == "Your ally"
            EventLog.add(caster.name .. " enters a glacial state!", {200,200,255})
    autotarget_func: (caster) -> caster.x, caster.y
    prereq_func: (caster) -> 
        if not caster.can_rest
            EventLog.add("Ice Form requires perfect concentration!", {200,200,255})
            return false
        return not caster\has_effect("Berserk") and not caster\has_effect("Exhausted")  and not caster\has_effect("Ice Form")
}

Data.effect_create {
    name: "Ludaze"
    effected_sprite: "spr_spells.ludaze"
    can_use_rest: false
    effected_colour: {200, 200, 255, 200}
    fade_out: 10
}

Data.effect_create {
    name: "Ice Form"
    effected_sprite: "spr_spells.iceform"
    can_use_rest: false
    can_use_spells: false
    can_use_stairs: true -- Important!!
    can_use_weapons: false
    effected_colour: {200, 200, 255}
    stat_func: (effect, obj, old, new) ->
        new.reduction += 20
        new.magic_reduction += 20
        new.resistance += 20
        new.magic_resistance += 20
        new.speed = math.min(new.speed, 0.5)
    fade_out: 55
}

-- Methods for implementing Aura's.
-- No extension mechanism, just called explicitly.
AuraBase = {
    init: () =>
        @n_steps = 0
        @total_time = @time_left
        @max_alpha = 0.15
        @n_ramp = 25
    step: () =>
        assert @range ~= nil, "'range' must be assigned to Aura effects."
    draw: (inner_col, outer_col, x, y) =>
        @n_steps += 1
        min = math.min(@n_ramp, if @n_steps > @total_time / 2 then math.abs(@n_steps - @total_time) else @n_steps)
        alpha = (min / @n_ramp)
        alpha = math.max(0.2, math.min(alpha, 1))
        alpha *= @max_alpha
        xy = Display.to_screen_xy {x, y}
        Display.draw_circle(with_alpha(inner_col, alpha), xy, @range)
        Display.draw_circle_outline(with_alpha(outer_col, alpha), xy, @range, 2)
}

-- Effects:

Data.effect_create {
    name: "HealthGainOnKill"
    category: "EquipEffect"
    init_func: (caster) =>
        @kill_tracker = caster.kills
    step_func: (caster) =>
        while caster.kills > @kill_tracker
            if caster\is_local_player()
                EventLog.add("You regain health for killing!", COL_PALE_BLUE)
                caster\heal_hp(5 + caster.stats.level)
            @kill_tracker += 1
}


Data.effect_create {
    name: "ManaGainOnKill"
    category: "EquipEffect"
    init_func: (caster) =>
        @kill_tracker = caster.kills
    step_func: (caster) =>
        while caster.kills > @kill_tracker
            if caster\is_local_player()
                EventLog.add("You regain mana for killing!", COL_PALE_BLUE)
                caster\heal_mp(5 + caster.stats.level)
            @kill_tracker += 1
}


Data.effect_create {
    name: "Summoning"
    category: "Summon"
    effected_colour: COL_PALE_RED
    fade_out: 10
    --stat_func: (effect, obj, old, new) ->
    --    new.speed *= 2
    init_func: (caster) =>
        @n_steps = 0
        @delay = 1
        @duration = @time_left
    step_func: (caster) =>
        @n_steps += 1
        if @n_steps == @delay
            ability = SpellObjects.SummonAbility.create {monster: @monster, :caster, xy: {caster.x, caster.y}, duration: @duration}
            GameObject.add_to_level(ability)
}

Data.effect_create {
    name: "Fear Aura"
    category: "Aura"
    effected_colour: {200, 200, 255}
    fade_out: 100
    effected_sprite: "spr_spells.cause_fear"
    init_func: (caster) =>
        AuraBase.init(@, caster)
        @max_alpha = 0.4
    step_func: (caster) =>
        AuraBase.step(@, caster)
        for mon in *(enemy_list caster)
            if mon\has_effect("Fear")
                continue
            dist = vector_distance({mon.x, mon.y}, {caster.x, caster.y})
            if dist < @range
                mon\add_effect("Fear", 300)
    draw_func: (caster, top_left_x, top_left_y) =>
        AuraBase.draw(@, COL_GRAY, COL_BLACK, caster.x, caster.y)
}

Data.effect_create {
    name: "Healing"
    category: "Aura"
    effected_sprite: "spr_amulets.healing"
    fade_out: 25
    step_func: (obj) =>
        obj\heal_hp(20 / 60)
}

Data.effect_create {
    name: "Healing Aura"
    category: "Aura"
    effected_sprite: "spr_amulets.healing"
    fade_out: 100
    init_func: (caster) =>
        AuraBase.init(@, caster)
        @max_alpha = 0.4
    step_func: (caster) =>
        AuraBase.step(@, caster)
        for ally in *ally_list(caster)
            if ally\has_effect("Healing")
                continue
            dist = vector_distance({ally.x, ally.y}, {caster.x, caster.y})
            if dist < @range
                ally\add_effect("Healing", 200)
    draw_func: (caster, top_left_x, top_left_y) =>
        AuraBase.draw(@, COL_GRAY, COL_WHITE, caster.x, caster.y)
}

Data.effect_create {
    name: "Daze Aura"
    category: "Aura"
    effected_sprite: "spr_amulets.light"
    fade_out: 100
    init_func: (caster) =>
        AuraBase.init(@, caster)
    step_func: (caster) =>
        AuraBase.step(@, caster)
        for mon in *(enemy_list caster)
            if mon\has_effect("Dazed")
                continue
            dist = vector_distance({mon.x, mon.y}, {caster.x, caster.y})
            if dist < @range
                mon\add_effect("Dazed", 200)
    draw_func: (caster, top_left_x, top_left_y) =>
        AuraBase.draw(@, COL_PALE_YELLOW, COL_YELLOW, caster.x, caster.y)
}

Data.effect_create {
    name: "Pain Aura"
    category: "Aura"
    effected_sprite: "spr_spells.greaterpain"
    fade_out: 100
    init_func: (caster) =>
        AuraBase.init(@, caster)
        @mp_gain = 10
        if caster.is_enemy
            @range = 90
    step_func: (caster) =>
        AuraBase.step(@, caster)
        if caster.is_enemy and not Map.object_visible(caster)
            return
        for mon in *(enemy_list caster)
            if mon\has_effect("Pained")
                continue
            dist = vector_distance({mon.x, mon.y}, {caster.x, caster.y})
            if dist < @range
                mon\add_effect("Pained", 50)
                caster\add_effect("Pained", 50)
                if mon\damage(random(4,15) * 2 + caster.stats.magic * 2, random(6,10) + caster.stats.magic * 0.2, 1.0, 2.0)
                    {:stats} = caster
                    stats.mp = math.min(stats.max_mp, stats.mp + @mp_gain)
                    -- Summon zombies by probability!?
                    if not caster.is_enemy and caster\is_local_player() 
                        EventLog.add("You drain the enemy's life force as MP!", {200,200,255})
                    elseif not caster.is_enemy
                        EventLog.add(caster.name .. " drains the enemy's life force as MP!", {200,200,255})
    draw_func: (caster, top_left_x, top_left_y) =>
        @max_alpha = 0.35
        if not caster.is_enemy and not caster\has_effect "Pained"
            @max_alpha /= 2
        AuraBase.draw(@, COL_PALE_RED, COL_RED, caster.x, caster.y)
}

-- TODO separate into ItemEffects.moon

Data.effect_create {
    name: "VampiricWeapon"
    on_melee_func: (attacker, defender, damage, attack_stats) =>
        if attacker\is_local_player() 
            EventLog.add("You steal the enemy's life!", {200,200,255})
        attacker\heal_hp(damage / 10)
        return damage
}


Data.effect_create {
    name: "Abolishment"
    stat_func: (effect, obj, old, new) ->
        new.strength += math.ceil(new.magic / 3)
        new.magic = 0
}

Data.effect_create {
    name: "EnemyHyperProjectile"
    stat_func: (effect, obj, old, new) ->
        new.ranged_cooldown_multiplier *= 0.1
}

Data.effect_create {
    name: "KnockbackWeapon"
    on_melee_func: (attacker, defender, damage, attack_stats) =>
        if defender\has_effect("Thrown")
            return
        if chance(.1)
            thrown = defender\add_effect("Thrown", 10)
            thrown.angle = vector_direction({attacker.x, attacker.y}, {defender.x, defender.y})
        return damage
}

Data.effect_create {
    name: "PoisonedWeapon"
    on_melee_func: (attacker, defender, damage, attack_stats) =>
        if defender\has_effect("Poison")
            return
        if chance(.25)
            eff = defender\add_effect("Poison", 100)
            eff.poison_rate = 25
            eff.damage = attack_stats.damage
            eff.power = attack_stats.power
            eff.magic_percentage = attack_stats.magic_percentage
        return damage
}

Data.effect_create {
    name: "FearWeapon"
    on_melee_func: (attacker, defender, damage, attack_stats) =>
        if defender\has_effect("Fear")
            return
        if chance(.1)
            eff = defender\add_effect("Fear", 100)
        return damage
}

Data.effect_create {
    name: "ConfusingWeapon"
    on_melee_func: (attacker, defender, damage, attack_stats) =>
        if defender\has_effect("Dazed")
            return
        if chance(.1)
            eff = defender\add_effect("Dazed", 400)
        return damage
}
