local GlobalData = require "core.GlobalData"
local MiscSpellAndItemEffects = require "core.MiscSpellAndItemEffects"
local EventLog = require "ui.EventLog"

assert(GlobalData.keys_picked_up)

Data.item_create {
    name = "Gold", -- An entry named gold must exist, it is handled specially
    spr_item = "gold"
}

Data.item_create {
    name = "Azurite Key",
    type = "key",
    description = "Now that you have picked up this key, you can open Azurite doors.",
    use_message = "Now that you have picked up this key, you can open Azurite doors.",
    spr_item = "key1",
    pickup_func = function(self, user)
        if not GlobalData.keys_picked_up[self.name] then
            play_sound "sound/win sound 2-1.ogg"
        end
        GlobalData.keys_picked_up[self.name] = true 
    end,
    prereq_func = function (self, user)
        return false
    end,
    stackable = false,
    sellable = false
}

Data.item_create {
    name = "Dandelite Key",
    description = "Now that you have picked up this key, you can open Dandelite doors.",
    type = "key",
    use_message = "Now that you have picked up this key, you can open Dandelite doors.",
    spr_item = "key2",
    pickup_func = function(self, user)
        if not GlobalData.keys_picked_up[self.name] then
            play_sound "sound/win sound 2-1.ogg"
        end
        GlobalData.keys_picked_up[self.name] = true 
    end,
    prereq_func = function (self, user)
        return false
    end,
    sellable = false,
    stackable = false
}

Data.item_create {
    name = "Burgundite Key",
    description = "Now that you have picked up this key, you can open Burgundite doors.",
    type = "key",
    use_message = "Now that you have picked up this key, you can open Burgundite doors.",
    spr_item = "key3",
    pickup_func = function(self, user)
        if not GlobalData.keys_picked_up[self.name] then
            play_sound "sound/win sound 2-1.ogg"
        end
        GlobalData.keys_picked_up[self.name] = true 
    end,
    prereq_func = function (self, user)
        return false
    end,
    sellable = false,
    stackable = false
}
for _, entry in ipairs {
    {"Snake Lanart", "The ancient Snake Lanart.", "spr_runes.rune_snake"},
    {"Abyssal Lanart", "The arcane Abyssal Lanart.", "spr_runes.rune_abyss"},
    {"Swarm Lanart", "The feared Swarm Lanart.", "spr_runes.rune_swamp"},
    {"Rage Lanart", "The fabled Rage Lanart.", "spr_runes.rune_tartarus"},
    {"Tomb Lanart", "The grimsly Tomb Lanart.", "spr_runes.rune_tomb"},
    {"Obliteration Lanart", "The Obliteration Lanart of legend, in your hands.", "spr_runes.rune_cerebov"},
    {"Dragon Lanart", "A legendary artifact. This Lanart can act as the replacement for a dragon's heart.", "spr_runes.rune_demonic_4"},
} do
    local name, description, sprite = entry[1],entry[2],entry[3]
    Data.item_create {
        name = name,
        description = description, use_message = description,
        type = "lanart",
        spr_item = sprite,
        pickup_func = function(self, user)
            if not GlobalData.lanarts_picked_up[self.name] then
                play_sound "sound/win sound 2-3.ogg"
            end
            GlobalData.lanarts_picked_up[self.name] = true 
        end,
        prereq_func = function (self, user)
            return false
        end,
        sellable = false,
        stackable = false
    }
end

Data.item_create {
    name = "Mana Potion",
    description = "A magical potion of energizing infusions, it restores 50 MP to the user.",
    type = "potion",

    shop_cost = {15, 35},

    spr_item = "mana_potion",

    prereq_func = function (self, user)
        return user.stats.mp < user.stats.max_mp
    end,

    action_func = function(self, user)
        user:heal_mp(50)
    end
}

Data.item_create {
    name = "Scroll of Fear",
    description = "Bestows the user with a terrible apparition, scaring away all enemies.",
    use_message = "You appear frightful!",

    shop_cost = {55,105},

    spr_item = "spr_scrolls.fear",

    action_func = function(self, user)
        user:add_effect("Fear Aura", 800).range = 120
    end
}

Data.item_create {
    name = "Scroll of Experience",
    description = "Bestows the user with a vision, leading to increased experience.",
    use_message = "Experience is bestowed upon you!",

    shop_cost = {55,105},

    spr_item = "scroll_exp",

    action_func = function(self, user)
        user:gain_xp(100)
    end
}

Data.item_create {
    name = "Defence Scroll",
    description = "A mantra of unnatural modification, it bestows the user with a permanent, albeit small, increase to defence.",
    use_message = "Defence is bestowed upon you!",

    shop_cost = {55,105},

    spr_item = "scroll_defence",

    action_func = function(self, user)
        user.stats.defence = user.stats.defence + 1
    end
}

Data.item_create {
    name = "Health Potion",
    description = "A blessed potion of healing infusions, it restores 50 HP to the user.",
    type = "potion",

    shop_cost = {15,35},

    spr_item = "health_potion",

    prereq_func = function (self, user) 
        return user.stats.hp < user.stats.max_hp 
    end,

    action_func = function(self, user)
        user:heal_hp(50)
    end
}

Data.item_create {
    name = "Strength Scroll",
    description = "A mantra of unnatural modification, it bestows the user with a permanent, albeit small, increase to strength.",
    use_message = "Strength is bestowed upon you!",

    shop_cost = {55,105},

    spr_item = "scroll_strength",

    action_func = function(self, user)
        user.stats.strength = user.stats.strength + 1
    end
}

Data.item_create  {
    name = "Haste Scroll",
    description = "A scroll that captures the power of the winds, it bestows the user with power and speed for a limited duration.",
    use_message = "You feel yourself moving faster and faster!",

    shop_cost = {25,45},

    spr_item = "scroll_haste",

    action_func = function(self, user)
        play_sound "sound/haste.ogg"
        user:add_effect(effects.Haste.name, 800)
    end
}

Data.item_create  {
    name = "Will Scroll",
    description = "A mantra of unnatural modification, it bestows the user with a permanent, albeit small, increase to will.",
    use_message = "Will is bestowed upon you!",

    shop_cost = {55,105},

    spr_item = "scroll_will",

    action_func = function(self, user)
        user.stats.willpower = user.stats.willpower + 1
    end
}

Data.item_create {
    name = "Magic Scroll",
    use_message = "Magic is bestowed upon you!",
    description = "A mantra of unnatural modification, it bestows the user with a permanent, albeit small, increase to magic.",

    shop_cost = {55,105},

    spr_item = "scroll_magic",

    action_func = function(self, user)
        user.stats.magic = user.stats.magic + 1
    end
}

Data.item_create {
    name = "Magic Map",
    use_message = "You gain knowledge of your surroundings!",
    description = "A magic map that reveals the current level.",

    shop_cost = {25,35},

    spr_item = "spr_scrolls.magic-map",

    action_func = function(self, user)
        MiscSpellAndItemEffects.magic_map_effect(user)
        EventLog.add_all("The map is revealed!", {255,255,255})
    end
}

