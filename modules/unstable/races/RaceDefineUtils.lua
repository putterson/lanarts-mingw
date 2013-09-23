local RaceType = import "@RaceType"

local Apts = import "@stats.AptitudeTypes"
local Stats = import "@Stats"
local StatContext = import "@StatContext"
local ContentUtils = import "@stats.ContentUtils"
local CooldownUtils = import "@stats.CooldownUtils"
local Attacks = import "@Attacks"

local M = nilprotect {} -- Submodule

-- A more convenient race_define
function M.races_define(args)
    args.description = args.description:pack()
    if args.on_create then -- Full custom
        return RaceType.define(args)
    end

    args.types = args.types or {Apts.BLUNT, Apts.MELEE} -- For CooldownUtils.derive_attack_with_cooldown
    args.damage = args.damage or 5
    args.attack = CooldownUtils.derive_attack_with_cooldown(args)
    assert(args.attack.on_prerequisite and args.attack.on_use)
    -- Create based off embedded stats, aptitudes & spells
    local stat_template = ContentUtils.resolve_embedded_stats(args, --[[Resolve skill costs]] true)
    function args.on_create(name, team)
        local stats = Stats.stats_create(stat_template, --[[Add skills]] true)
        stats.name = name
        stats.team = team
        stats.race = args
        return stats
    end

    return RaceType.define(args)
end

return M