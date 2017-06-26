Randarts = require "items.Randarts"

M = {} -- Submodule

_filter = (group) -> 
    -- Remove objects dummied out with 'true':
    table.remove_occurrences(group, true)
    for i=1,#group
        _filter(group[i])
    return group

-- Rings
rings = (chance, args) -> _filter{
    :chance
    args.ignore_medium or { item: "Magician's Ring",    chance: 8                      }
    args.ignore_medium or { item: "Moonstone Ring",     chance: 8                      }
    args.ignore_medium or { item: "Ogre's Ring",        chance: 8                      }
    args.ignore_medium or { item: "Hydra Ring",         chance: 8                      }
    args.ignore_medium or{  item: "Gallanthor's Ring",  chance: 4                      }
    args.ignore_strong or{  item: "Ring of Ethereal Armour",  chance: 1                      }
    args.ignore_strong or{  item: "Ring of Vampirism",  chance: 1                      }
    args.ignore_strong or{  item: "Abolishment Ring",  chance: 1                      }
    args.ignore_strong or{  item: "Wizard's Ring",  chance: 4                      }
    { item: "Ring of Spells",     chance: 8                      }
    { item: "Ring of Vitality",   chance: 8                      }
    { item: "Ring of Stone",      chance: 4                      }
--    { item: RANDART("Ring"),      chance: 4                      }
}

-- Amulets
amulets = (chance, args) -> { 
    :chance
    { item: "Amulet of the Wall", chance: 4 }
    { item: "Amulet of Fear", chance: 4 }
    { item: "Amulet of Ringholding", chance: 4 }
    { item: "Amulet of Trepidation", chance: 4 }
    { item: "Amulet of Healing", chance: 4 }
    { item: "Amulet of Mephitization", chance: 4 }
    { item: "Amulet of Greed", chance: 1 }
    --{ item: "Amulet of Fire", chance: 4 }
    -- { item: "Amulet of Greater Fire", chance: 1 }
    { item: "Amulet of Fortification", chance: 4 }
--    { item: "Amulet of the Berserker", chance: 4 }
--    { item: "Amulet of Pain", chance: 4 }
    { item: "Amulet of Great Pain", chance: 2 }
    { item: "Amulet of Ringholding", chance: 4 }
--    { item: "Amulet of Ice Form", chance: 4 } -- For now, a guaranteed drop only
    args.ignore_strong or { item: "Amulet of Regeneration", chance: 1 }
    args.ignore_strong or { item: "Amulet of Light", chance: 1 }
--    { item: RANDART("Amulet"),      chance: 1 }
}

-- Consumables
consumables = (chance, args) -> _filter { 
    :chance
    {
        chance: 99
        { item: "Arrow",              chance: 5,  amount: {5,10}  }
        { item: "Silver Arrow",       chance: 4,  amount: {2,6}   }
        -- Scrolls
        { item: "Haste Scroll",       chance: 2                      }
        -- Potions
        { item: "Health Potion",      chance: 8                     }
        { item: "Mana Potion",        chance: 8                     }
        { item: "Stone",              chance: 4,  amount: {3,15}  }
        { item: "Magic Map",         chance: 2                      }
        { item: "Scroll of Fear",         chance: 2                      }
    }

    -- Permanent enchantments
    args.ignore_strong or { 
        chance: 1
        { item: "Strength Scroll",    chance: 1                      }
        { item: "Magic Scroll",       chance: 1                      }
        { item: "Defence Scroll",     chance: 1                      }
        { item: "Will Scroll",        chance: 1                      }
        -- Make the above very, very rare in comparison:
        { item: "Scroll of Experience",        chance: 96             }
    }
}

-- Headgear
helmets = (chance, args) -> _filter{ 
    :chance
    -- Helmets
    args.ignore_weak or { item: "Iron Helmet",        chance: 5                      }
    args.ignore_weak or { item: "Wizard's Hat",       chance: 5                      }
    { item: "Runed Helmet",       chance: 1                      }
    { item: "Crown of Mars",      chance: 1                      }
    { item: "Crown of Orcus",     chance: 1                      }
    { item: "Diana's Diadem",     chance: 1                      }
}

-- Belts
belts = (chance, args) -> _filter { 
    :chance
    { item: "Belt of Protection",      chance: 20                      }
    { item: "Belt of Slaying",        chance: 20                      }
    { item: "Lifesteal Belt",        chance: 10                      }
    args.ignore_strong or { item: "Warped Belt",        chance: 1                      }
    args.ignore_strong or { item: "Wishful Belt",        chance: 1                      }
    args.ignore_strong or { item: "Spiked Belt",        chance: 1                      }
    args.ignore_medium or { item: "Dank Belt",        chance: 2                      }
}

-- Legwear
legwear  = (chance) -> { 
    :chance
    { item: "Platelegs",      chance: 50                      }
    { item: "Magic Skirt",        chance: 30                      }
    { item: "Gallanthor's Skirt",        chance: 1                      }
}

bows  = (chance, args) -> _filter {
    :chance 
    {
        chance: 10
        args.ignore_weak or {
            chance: 100
            { item: "Short Bow",          chance: 2                      }
        } 
        args.ignore_medium or {
            chance: 20
            -- Bows
            { item: "Long Bow",           chance: 2                      }
        }
        args.ignore_strong or {
            chance: 5
            -- Bows
            { item: "Mars Bow",           chance: 90                      }
            { item: "The Piercer",           chance: 1                      }
        }
    }
--    args.ignore_randarts or { item: RANDART("Bow"), chance: 1}
}

armour = (chance, args) -> _filter {
    :chance,
    -- Body Armour
    args.ignore_weak or { item: "Leather Armour",     chance: 8                      }
    args.ignore_weak or { item: "Robe",               chance: 6                      }
    args.ignore_weak or { item: "Green Robe",               chance: 1         }
    args.ignore_weak or { item: "White Robe",               chance: 1         }
    args.ignore_weak or { item: "Red Robe",               chance: 1         }

    args.ignore_medium or{ item: "Chainmail",          chance: 4                      }
    args.ignore_medium or { item: "Magician's Robe",    chance: 2                      }
    args.ignore_medium or { item: "Robe of Mana",       chance: 4                      }
    args.ignore_medium or { item: "Robe of Vitality",       chance: 2                      }

    args.ignore_strong or { item: "Platemail",          chance: 2                      }
    args.ignore_strong or { item: "Crystalline Plate",          chance: 1                      }
    args.ignore_strong or { item: "Runed Robe",         chance: 1                      }
}

boots = (chance, args) -> _filter {
    :chance 
    {
        chance: 10
        args.ignore_weak or {
            chance: 100
            { item: "Leather Boots",      chance: 1                      }
        } 
        --args.ignore_medium or {
        --    chance: 20
        --    { item: "Iron Boots",           chance: 2                      }
        --}
        args.ignore_strong or {
            chance: 5
            { item: "Troll Boots",           chance: 90                      }
            { item: "Stripe Boots",           chance: 90                      }
            { item: "Jack Boots",           chance: 90                      }
        }
    }
--    args.ignore_randarts or { item: RANDART("Boots"), chance: 1}
}

gloves = (chance, args) -> _filter {
    :chance 
    {
        chance: 10
        args.ignore_weak or {
            chance: 100
            { item: "Leather Gloves",      chance: 1                      }
        } 
        args.ignore_strong or {
            chance: 5
            { item: "Steel Gloves",           chance: 200                      }
            { item: "Runed Gloves",           chance: 90                      }
            { item: "Gloves of Fear",           chance: 10                      }
            { item: "Gloves of Confusion",           chance: 10                      }
        }
    }
--    args.ignore_randarts or { item: RANDART("Gloves"), chance: 1}
}

weapons = (chance, args) -> _filter {
    :chance 
    {
        chance: 10
        args.ignore_weak or {
            chance: 100
            { item: "Dagger",      chance: 1                      }
            { item: "Short Sword",      chance: 1                      }
--            { item: "Standard Sword",      chance: 1                      }
        } 
        args.ignore_medium or {
            chance: 20
            { item: "Wizard's Staff",                 chance: 1           }
            { item: "Mace",                          chance: 1           }
        }
        args.ignore_strong or {
            chance: 5
            { item: "Iron Broadsword",                          chance: 4           }
            { item: "Gallanthor's Crutch",                 chance: 4     }
            { item: "Serated Sword",      chance: 4                      }
            { item: "Magic Sword",        chance: 4                     }
--            { item: "Triple Sword",        chance: 4                      }
--            { item: "Battle Axe",        chance: 4                      }
            { item: "Iron Scimitar",        chance: 1                      }
            { item: "Orc Axe",        chance: 4                      }
        }
    }
--    args.ignore_randarts or { item: RANDART("Weapon"), chance: 1}
}

M.store_items = {
    consumables(60, ignore_weak: false)
    rings(8, ignore_weak: true)
    helmets(3, ignore_weak: true)
    armour(5, ignore_weak: true)
    belts(2, ignore_weak: true)
    legwear(1, ignore_weak: true)
    boots(1, ignore_weak: true)
    amulets(2, ignore_weak: true)
    bows(5, ignore_weak: true)
    gloves(5, ignore_weak: true)
    weapons(8, {ignore_weak: true, ignore_medium: true})
}

M.enchanted_items = {
    chance: 10 -- For basic_items entry
    { item: "Gold",                 chance: 85, amount: {15,45}  }
    consumables(22, ignore_weak: true, ignore_strong: true)
    rings(8, ignore_weak: true)
    helmets(3, ignore_weak: true)
    armour(5, ignore_weak: true)
    belts(1, ignore_weak: true)
    amulets(2, ignore_weak: true)
    legwear(2, ignore_weak: true)
    boots(1, ignore_weak: true)
    bows(5, ignore_weak: true)
    gloves(3, ignore_weak: true)
    weapons(12, ignore_weak: true, ignore_medium: true)
}

M.basic_items = {
    { item: "Gold",                 chance: 100, amount: {5,15}  }
    consumables(40, ignore_strong: true)
    rings(2, ignore_strong: true)
    helmets(3, ignore_strong: true)
    armour(12, ignore_strong: true)
    amulets(2, ignore_weak: true)
    belts(1, ignore_strong: true)
    legwear(1, ignore_strong: true)
    boots(1, ignore_strong: true)
    bows(5, ignore_strong: true)
    gloves(5, ignore_strong: true)
    weapons(12, ignore_strong: true)
    -- M.enchanted_items -- Chance of being moved up a category. See enchanted_items for the weight.
}

M.randart_items = require("items.Randarts").RANDARTS

M.epic_store_items = {
    consumables(40, ignore_weak: true)
    table.merge M.randart_items[1], {chance: 40}
    table.merge M.randart_items[2], {chance: 20}
    table.merge M.randart_items[3], {chance: 10}
}

return M
