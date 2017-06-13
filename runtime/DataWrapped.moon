-- Normal effect. By default, takes the 'max' of all applied time_left's
effect_create = (args) ->
    wrapped_init = args.init_func
    wrapped_step = args.step_func
    wrapped_apply = args.apply_func
    wrapped_remove = args.remove_func
 
    args.init_func = (obj) =>
        @time_left = 0
        @active = false
        @n_derived = 0
        if wrapped_init
            wrapped_init(@, obj)
    args.apply_derived_func or= (obj, args) =>
        @active = true
        @n_derived += 1
        if wrapped_apply
            wrapped_apply(@, obj, args)
    args.remove_derived_func or= (obj, args) =>
        @n_derived = 0
        @active = (@time_left > 0)
    args.apply_buff_func or= (obj, args) =>
        @active = true
        if wrapped_apply 
            wrapped_apply(@, obj, args)
        time_left = if type(args) == 'number' then args else args.time_left
        @time_left = math.max(@time_left, time_left) 
    args.remove_func = (obj) =>
        if wrapped_remove
            wrapped_remove(@, obj)
        @init_func(obj)
    args.step_func = (obj) =>
        if wrapped_step 
            wrapped_step(@, obj)
        @time_left = math.max(@time_left - 1, 0)
        if @time_left <= 0 and @n_derived == 0
            @remove_func(obj)
            @time_left = 0
    Data.effect_create(args)

-- List of subeffects, with an accumulated stat.
_subeffect_effect_create = (args, starting_value, accum) ->
    wrapped_init = args.init_func
    wrapped_step = args.step_func
    wrapped_apply = args.apply_func
    wrapped_remove = args.remove_func
 
    args.init_func = (obj) =>
        @active = false
        @subeffects = {}
        @current = starting_value
        @time_left = 0
    args.apply_derived_func or= (obj, args) =>
        @active = true
        @current = accum(@current, args)
    args.remove_derived_func or= (obj, args) =>
        @active = (#@subeffects > 0)
        @current = starting_value
    args.apply_buff_func or= (obj, args) =>
        @active = true
        assert(type(args.time_left) == 'number')
        append @subeffects, table.clone(args)
    args.remove_func = (obj) =>
        @active = false
        table.clear @subeffects
        @current = starting_value
    args._get_value = (obj) =>
        x = @current
        for eff in *@subeffects
            x = accum(x, eff)
        return x
    args.step_func = (obj) =>
        -- Step subeffects, remove those at t=0
        filter = false
        for eff in *@subeffects
            if wrapped_step 
                wrapped_step(@, obj, eff)
            eff.time_left = math.max(eff.time_left - 1, 0)
            if eff.time_left <= 0
                filter = true
        if filter
            @subeffects = table.filter(@subeffects, (x) -> x.time_left > 0)
        -- If nothing left to this effect, remove it (make it inactive)
        if @subeffects == 0 and @current ~= starting_value
            @remove_func(obj)
    Data.effect_create(args)

additive_effect_create = (args) ->
    key = args.key or 'value'
    accum = (x, next) -> x + next[key]
    return _subeffect_effect_create(args, 0, accum)

STANDARD_WEAPON_DPS = 10
STANDARD_RANGED_DPS = 5

weapon_create = (args, for_enemy = false) -> 
    damage_multiplier = args.damage_multiplier or 1.0
    dps = if args.type == "bows" then STANDARD_RANGED_DPS else STANDARD_WEAPON_DPS
    damage = damage_multiplier * (args.cooldown / 60 * dps)
    power = 0
    -- For enemy_create made weapons, directly set base damage:
    if type(args.damage) == 'number'
        damage = args.damage
        args.damage = {base: {math.floor(damage *0.9), math.ceil(damage * 1.1)}, strength: 0}
    else 
        args.damage or= {base: {math.floor(damage), math.ceil(damage)}, strength: 0}
    args.power or= {base: {power, power}, strength: 1}
    args.range or= 7
    items[args.name] = args -- HACK
    Data.weapon_create(args)

spell_create = (args) ->
    proj = args.projectile
    if proj
        damage_multiplier = proj.damage_multiplier or 1.0
        damage = damage_multiplier * (args.cooldown / 60 * STANDARD_WEAPON_DPS)
        proj.name or= args.name
        proj.weapon_class or= "magic"
        proj.spr_item or= "none"
        proj.spr_attack or= args.spr_spell
        proj.cooldown or= args.cooldown
        proj.range or= 300
        proj.damage_type or= {magic: 1.0}
        proj.damage or= {base: {math.floor(damage), math.ceil(damage)}, strength: 0}
        proj.power or= {base: {0, 0}, magic: 1}
        Data.projectile_create(proj)
        args.projectile = proj.name
    Data.spell_create(args)

projectile_create = (args, for_enemy = false) -> 
    damage_multiplier = args.damage_multiplier or 1.0
    damage = damage_multiplier * (args.cooldown / 60 * STANDARD_WEAPON_DPS)
    if for_enemy
        -- For enemies, we want all damage to come from 'damage'.
        -- The strength and magic stats work differently for enemies thusly.
        args.power or= {base: {0, 0}}
        damage = damage_multiplier * (args.cooldown / 60 * args.damage)
        args.damage or= {base: {0, 0}, strength: args.damage_type.physical, magic: args.damage_type.magic}
    else
        args.damage or= {base: {math.floor(damage), math.ceil(damage)}, strength: 0}
        args.power or= {base: {0, 0}, strength: args.damage_type.physical, magic: args.damage_type.magic}
    args.spr_item or= "none"
    args.range or= 300
    Data.projectile_create(args)

enemy_create = (args) ->
    args.stats.attacks or= {}
    w = args.weapon
    if w ~= nil
        w.name or= args.name .. " Melee"
        w.type or= "unarmed"
        w.spr_item or= "none"
        append args.stats.attacks, {weapon: w.name}
        weapon_create(w, true)
    p = args.projectile
    if p ~= nil
        p.name or= args.name .. " Projectile"
        p.spr_item or= "none"
        append args.stats.attacks, {projectile: p.name}
        projectile_create(p, true)
    Data.enemy_create(args)

return {:additive_effect_create, :effect_create, :weapon_create, :spell_create, :projectile_create, :enemy_create}
