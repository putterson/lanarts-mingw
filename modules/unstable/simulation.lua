local StatContext = import "@StatContext"
local AttackResolution = import "@attack_resolution"
local Attacks = import "@Attacks"
local Stats = import "@Stats"
local AnsiCol = import "core.terminal.ansi_colors"
local Races = import "@Races"
local Relations = import "lanarts.objects.Relations"
local Items = import "@content.items"
local AptitudeTypes = import "@content.aptitude_types"
local LogUtils = import "@content.log_utils"

local function choose_option(...)
    local options = {...}
    local prompt = {}
    for idx, option in ipairs(options) do
        table.insert(prompt, idx .. ') ' .. option)
    end

    AnsiCol.println(("\n"):join(prompt), AnsiCol.YELLOW, AnsiCol.BOLD)
    local choice
    while not choice do
        local input = io.read("*line")
        choice = options[tonumber(input)]
        if not choice then
            AnsiCol.println("Sorry, that is not a valid choice.", AnsiCol.RED, AnsiCol.BOLD)
        end
    end

    return choice
end

-- Returns whether performed the spell
local function perform_spell(player, enemy)
    local spells = player.base.spells
    local choices = {"Back"}
    local choice2spell = {}
    for spell in spells:values() do
        table.insert(choices, spell.name)
        choice2spell[spell.name] = spell
    end

    local spell = choice2spell[choose_option(unpack(choices))]
    if not spell then return false end

    local used = StatContext.use_spell(player, spell)
    if not used then
        print("You must wait before using this.")
    end

    return used
end

-- Returns whether used item
local function use_item(player)
    local inventory = player.base.inventory
    local choices = {"Back"}
    local choice2item = {}
    for item in inventory:values() do
        table.insert(choices, item.type.name)
        choice2item[item.type.name] = item
    end

    local item = choice2item[choose_option(unpack(choices))]
    if not item then return false end

    inventory:use_item(player, item.type)
    return true
end

local function damage(attacker, target)
    local dmg = AttackResolution.damage_calc(attacker.obj.attacks[1], attacker, target)

    AnsiCol.println(
        LogUtils.resolve_conditional_message(attacker.obj, "{The }$You deal{s} " ..dmg .. " damage!"),
        AnsiCol.GREEN, AnsiCol.BOLD
    )

    StatContext.add_hp(target, -dmg)

    AnsiCol.println(
        LogUtils.resolve_conditional_message(target.obj, "{The }$You [have]{has} " .. target.base.hp .. "HP left"), 
        AnsiCol.BLUE, AnsiCol.ITALIC .. AnsiCol.BOLD
    )

end

local frame = 1

local function step(context)
    for i=1,50 do
        StatContext.on_step(context)
        StatContext.on_calculate(context)
        frame = frame + 1
    end
end

local function battle(player, enemy)
    local spells = import "@content.spells"

    StatContext.add_item(player, Items.health_potion)
    StatContext.add_spell(player, spells.berserk)

    step(player)
    while enemy.base.hp > 0 do
        local action = choose_option("Attack", "Spell", "Item")
        local moved = true
        if action == "Attack" then
            damage(player, enemy) 
        elseif action == "Spell" then
            moved = perform_spell(player, enemy)
        elseif action == "Item" then
            moved = use_item(player)
        end
        if moved then
            step(enemy)
            damage(enemy, player)
            step(player)
        end
    end
end

local function replace_event_log_with_print()
    local EventLog = import "core.ui.EventLog"
    EventLog.add = function(msg) 
        print(msg)
    end
end

local function main()
    local animals = import "@content.monsters.animals"

    import "@content.races"

    replace_event_log_with_print()

    -- Choose race
    local options = {}
    for race in values(Races.list) do
        table.insert(options, race.name)
    end

    local race = Races.lookup( choose_option(unpack(options)) )
    local stats = race.on_create("Tester", Relations.TEAM_PLAYER_DEFAULT)

    -- Create pseudo-objects
    local player = {
        is_local_player = function() return true end,
        base_stats = stats,
        traits = {"player"},
        derived_stats = table.deep_clone(stats),
        attacks = {Attacks.attack_create(0, 5, dup(AptitudeTypes.melee, 4))},
        name = "TesterMan"
    }

    local monster_stats = table.deep_clone(animals.rat.stats)
    local monster = {
        traits = {"monster"},
        is_local_player = function() return false end,
        base_stats = monster_stats,
        derived_stats = table.deep_clone(monster_stats),
        name = animals.rat.name,
        attacks = animals.rat.attacks
    }

    local scmake = StatContext.game_object_stat_context_create
    battle(scmake(player), scmake(monster))
end

main()