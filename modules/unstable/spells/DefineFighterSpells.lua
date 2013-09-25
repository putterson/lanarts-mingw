local spell_define = (import ".SpellDefineUtils").spell_define
local StatusType = import "@StatusType"

local SpellTraits = import ".SpellTraits"

spell_define {
	name = "Berserk",
	description = "Allows you to strike powerful blows for a limited duration, afterwards you are slower and vulnerable.",
	mp_cost = 40,

    

    on_use = function(self, caster)
        local B = caster.base
        StatusType.update_hook(B.hooks, "Berserk", caster, 150 + math.min(4, B.level)  * 20)
    end,

    cooldown_self = 1000,
    cooldown = 50
}
