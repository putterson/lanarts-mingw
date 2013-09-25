local Display = import "core.Display"

local Apts = import "@stats.AptitudeTypes"
local SpellsKnown = import "@SpellsKnown"
local StatContext = import "@StatContext"
local SpellType = import "@SpellType"
local Stats = import "@Stats"
local SkillType = import "@SkillType"
local Attacks = import "@Attacks"

local M = nilprotect {} -- Submodule

M.RESOURCE_METATABLE = {
    __index = function (t,k) return assert(rawget(t, "type"))[k] end,
    __copy = function(t1,t2) table.copy(t1,t2, --[[Do not invoke meta]] false) end
}

-- Derive sprite from name
function M.derive_sprite(name, --[[Optional]] stack_idx)
    local path = path_resolve("sprites/" .. name:gsub(' ', '_'):lower() .. ".png", (stack_idx or 1) + 1)
    return image_cached_load(path)
end

function M.resolve_sprite(args, --[[Optional]] stack_idx)
    local sprite = args.sprite or M.derive_sprite(args.lookup_key or args.name, (stack_idx or 1) + 1)
    if type(sprite) == "string" then
        if sprite:find("%(") or sprite:find("%%") then
            sprite = path_resolve(sprite, (stack_idx or 1) + 1)
            return Display.animation_create(Display.images_load(sprite), 1.0)
        else
            return Display.image_load(sprite)
        end
    end
    return sprite
end

-- Resolves bonuses of the form [Apts.FOOBAR] = {0,1,1,0}
-- or [Apts.FOOBAR] = 2, equivalent to {2,2,2,2}
-- Expects a mutable 'table' that is being used to define a resource type
-- It will remove the 'convenience variables' used to define the aptitudes.
function M.resolve_aptitude_bonuses(table, --[[Optional]] result)
    result = result or {effectiveness={},damage={},resistance={},defence={}}
    for key,val in pairs(table) do
        if Apts.allowed_aptitudes[key] then
            local eff,dam,res,def
            if type(val) == 'number' then
                eff,dam,res,def = val,val,val,val
            else
                assert(#val == 4)
                eff,dam,res,def = unpack(val)
            end

            -- Add the bonus
            local curr_eff, curr_dam = result.effectiveness[key] or 0, result.damage[key] or 0 
            local curr_res, curr_def = result.resistance[key] or 0, result.defence[key] or 0
            result.effectiveness[key], result.damage[key] = curr_eff + eff, curr_dam + dam 
            result.resistance[key], result.defence[key] = curr_res + res, curr_def + def
 
            table[key] = nil -- Cleanup the aptitude convenience variable
        end
    end
    return result
end

function M.resolve_skills(table)
    local C = table.skill_costs or {}
    local skills = {}
    for k,v in pairs(C) do
        local type = assert(SkillType.lookup(k), k .. " is not a skill!")
        local skill = type:on_create()
        skill.cost_multiplier = skill.cost_multiplier * v
        _G.table.insert(skills, skill) 
    end
    table.skill_costs = nil
    return skills
end

-- Expects a mutable 'table' that is being used to define a resource type
-- It will remove the 'convenience variables' used to define the aptitudes.
function M.resolve_embedded_stats(table, --[[Optional]] resolve_skills)
    table.aptitudes = M.resolve_aptitude_bonuses(table)
    if resolve_skills then
        table.skills = M.resolve_skills(table)
    end
    local spells = table.spells
    if not getmetatable(table.spells) == SpellsKnown then
        spells = SpellsKnown.create()
        for spell in values(table.spells) do
            if type(spell) == "table" and not spell.id then
                spell = SpellType.define(spell)
            end 
            spells:add_spell(spell)
        end
    end
    table.spells = spells
    return Stats.stats_create(table)
end

-- Returns an attack effect TODO: Rename to derive_attack_effect
function M.derive_attack(args, --[[Optional, default false]] cleanup_member)
    local attack = args.unarmed_attack or args.attack
    local types = args.aptitude_types or args.multipliers or args.types -- TODO Only allow 'aptitude_types'!!
    if not types then return nil end

    -- First resolve 
    if attack and #attack > 0 then -- Resolve argument table
        attack = Attacks.attack_create(attack)
    end
    if not attack then
        attack = Attacks.attack_create(
            args.effectiveness or 0, args.damage or 0, types, 
            --[[Optional]] args.delay, --[[Optional]] args.damage_multiplier, -- TODO only use apt types 
            --[[Optional]] args.range
        )
    end
    if cleanup_member then
        args.effectiveness, args.damage = nil
        args.attack, args.unarmed_attack, args.multipliers, args.type = nil
        args.delay, args.damage_multiplier, args.range = nil
    end
    return attack
end

function M.combine_on_prereq(p1, p2)
    return  --[[If]] p2 and --[[Then]] func_apply_and(p1, p2) or --[[Else]] p1
end

function M.combine_on_use(p1, p2)
    return  --[[If]] p2 and --[[Then]] func_apply_sequence(p1, p2) or --[[Else]] p1
end 

return M
