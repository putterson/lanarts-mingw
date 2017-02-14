Data.equipment_create {
    name = "Amulet of the Wall",
    description = "Grants the user the spell 'Wallanthor', and +1 defence.",
    type = "amulet",

    shop_cost = { 120, 140 },

    spr_item = "spr_amulets.blue",
    spells_granted = {"Wallanthor"},

    stat_bonuses = { defence = 1 }
}

-- Data.equipment_create {
--     name = "Amulet of Protection",
--     description = "Grants the user increased defences against magical attacks.",
--     type = "amulet",
-- 
--     shop_cost = { 180, 240 },
-- 
--     stat_bonuses = { willpower = 4 },
--     spr_item = "spr_amulets.protect",
-- }

Data.equipment_create {
    name = "Amulet of Fortification",
    description = "Grants the user Fortification: Melee hits from enemies increase defence temporarily.",
    type = "amulet",

    shop_cost = { 220, 440 },

    effects_granted = {"Fortification"},
    spr_item = "spr_amulets.protect",
}

Data.equipment_create {
    name = "Amulet of Greed",
    description = "Grants the user the ability to wear two additional amulets.",
    type = "amulet",

    shop_cost = { 220, 440 },

    spr_item = "spr_amulets.greed",
    -- Implemented in C++
}


-- name = "Amulet of Festivity",
Data.equipment_create {
    name = "Amulet of Fear",
    description = "Grants +1 strength, and 10% chance to scare enemies on every melee strike.",
    type = "amulet",

    shop_cost = { 120, 140 },

    spr_item = "spr_amulets.fear_strike",
    --spells_granted = {"Fear Strike"},
    effects_granted = {"FearWeapon"},
    stat_bonuses = {strength = 1}
}

Data.equipment_create {
    name = "Amulet of Healing",
    description = "Heals HP every time you kill an enemy.",
    type = "amulet",

    shop_cost = { 120, 140 },

    spr_item = "spr_amulets.healing",
    effects_granted = {"AmuletHealthGainOnKill"},
}


Data.equipment_create {
    name = "Amulet of Trepidation",
    description = "Grants the user the spell 'Trepidize', and +1 magic.",
    type = "amulet",

    shop_cost = { 120, 140},

    spr_item = "spr_amulets.skull",
    spells_granted = {"Trepidize"},
    stat_bonuses = {magic = 1}
}

Data.equipment_create {
    name = "Amulet of Mephitization",
    description = "Grants the user the spell 'Mephitize', and 10% chance of poison on melee attacks.",
    type = "amulet",

    shop_cost = { 120, 140 },

    spr_item = "spr_amulets.cabochon",
    spells_granted = {"Mephitize"},
    effects_granted = {"PoisonedWeapon"}
}

Data.equipment_create {
    name = "Amulet of Regeneration",
--    description = "Grants the user the spell 'Regeneration'.",--, but the user has -25HP.",
    description = "Grants the user extra health regeneration.",--, but the user has -25HP.",
    type = "amulet",

    shop_cost = { 220, 340 },
--    stat_bonuses = { hp = -25},

    spr_item = "spr_amulets.regeneration",
    stat_bonuses = {hpregen = 2 / 60},
    --spells_granted = {"Regeneration"},
}

Data.equipment_create {
    name = "Amulet of Greater Fire",
    description = "Grants the user multidirectional Fire Bolt casts, and makes all projectile spells move faster.",
    type = "amulet",

    shop_cost = { 120, 140 },

    spr_item = "spr_amulets.greaterfireball",
    effects_granted = {"AmuletGreaterFire"},
    --spells_granted = {"Fireball"},
    stat_bonuses = {spell_velocity_multiplier = 1.15}
}

--Data.equipment_create {
--    name = "Amulet of Fire",
--    description = "Grants the user the spell 'Fire Bolt'.",
--    type = "amulet",
--
--    shop_cost = { 70, 100 },
--
--    spr_item = "spr_amulets.fireball",
--    spells_granted = {"Fire Bolt"},
--}

Data.equipment_create {
    name = "Amulet of the Berserker",
    description = "Berserked user regenerates 4HP a second, and has 5% chance of summoning a centaur on kill.",
    type = "amulet",

    shop_cost = { 120, 140 },

    spr_item = "spr_amulets.berserk",
    effects_granted = {"AmuletBerserker"}
}

Data.equipment_create {
    name = "Amulet of Great Pain",
    description = "Grants the user double healing power from necromancy spells.",
    type = "amulet",

    shop_cost = { 120, 140 },

    spr_item = "spr_amulets.greaterpain",
    effects_granted = {"AmuletGreatPain"},
    stat_bonuses = {willpower = 1}
}

Data.equipment_create {
    name = "Amulet of Ice Form",
    description = "Grants the user the spell 'Ice Form'.",
    type = "amulet",

    shop_cost = { 200, 250 },

    spr_item = "spr_amulets.iceform",
    spells_granted = {"Ice Form"}
}

Data.equipment_create {
    name = "Amulet of Light",
    description = "Grants the user the potent crowd control spell 'Luminos'.",
    type = "amulet",

    shop_cost = { 320, 440 },

    spr_item = "spr_amulets.light",
    spells_granted = {"Luminos"}
}

--Data.equipment_create {
--    name = "Amulet of Pain",
--    description = "Grants the user the spell 'Pain' and +1 willpower.",
--    type = "amulet",
--
--    shop_cost = { 120, 140 },
--
--    spr_item = "spr_amulets.pain",
--    spells_granted = {"Pain"},
--    stat_bonuses = {willpower = 1}
--}

Data.equipment_create {
    name = "Amulet of Ringholding",
    description = "Grants user the ability to wear an additional ring.",
    type = "amulet",

    shop_cost = { 120, 140 },

    spr_item = "spr_amulets.urand_bloodlust",
}

