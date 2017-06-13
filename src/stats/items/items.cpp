/*
 * items.h:
 *  Define item state. These are defined in terms of a base item, and applied properties.
 */

#include <cstring>

#include <lcommon/SerializeBuffer.h>

#include "EquipmentEntry.h"
#include "ItemEntry.h"
#include "ProjectileEntry.h"
#include "WeaponEntry.h"

#include "items.h"

bool ItemProperties::operator ==(const ItemProperties& properties) const {
	if (memcmp(&properties.cooldown_modifiers, &this->cooldown_modifiers,
			sizeof(CooldownModifiers) != 0)) {
		return false;
	}
	if (memcmp(&properties.stat_modifiers, &this->stat_modifiers,
			sizeof(StatModifiers) != 0)) {
		return false;
	}
	if (memcmp(&properties.damage, &this->damage, sizeof(DamageStats) != 0)) {
		return false;
	}
//	if (properties.effect_modifiers.status_effects
//			!= effect_modifiers.status_effects) {
//		return false;
//	}
	if (properties.flags != flags || properties.unknownness != unknownness) {
		return false;
	}
	return true;
}

void ItemProperties::serialize(GameState* gs, SerializeBuffer& serializer) {
	serializer.write(this->cooldown_modifiers);
	serializer.write(this->stat_modifiers);
	serializer.write(this->damage);
	serializer.write_int(this->flags);
	serializer.write(this->unknownness);
    this->effect_modifiers.serialize(gs, serializer);
}

void ItemProperties::deserialize(GameState* gs, SerializeBuffer& serializer) {
	serializer.read(this->cooldown_modifiers);
	serializer.read(this->stat_modifiers);
	serializer.read(this->damage);
	serializer.read_int(this->flags);
	serializer.read(this->unknownness);
	this->effect_modifiers.deserialize(gs, serializer);
}

ItemEntry& Item::item_entry() const {
	return *game_item_data.at(id);
}

EquipmentEntry& Item::equipment_entry() const {
	return get_equipment_entry(id);
}

ProjectileEntry& Item::projectile_entry() const {
	return get_projectile_entry(id);
}

WeaponEntry& Item::weapon_entry() const {
	return get_weapon_entry(id);
}

bool Item::is_normal_item() const {
	ItemEntry* item_entry = game_item_data.at(id);
	return (dynamic_cast<EquipmentEntry*>(item_entry) == NULL);
}

bool Item::is_equipment() const {
	ItemEntry* item_entry = game_item_data.at(id);
	return (dynamic_cast<EquipmentEntry*>(item_entry) != NULL);
}

bool Item::is_projectile() const {
	ItemEntry* item_entry = game_item_data.at(id);
	return (dynamic_cast<ProjectileEntry*>(item_entry) != NULL);
}

bool Item::is_weapon() const {
	ItemEntry* item_entry = game_item_data.at(id);
	return (dynamic_cast<WeaponEntry*>(item_entry) != NULL);
}

bool Item::is_same_item(const Item & item) const {
	return id == item.id && properties == item.properties;
}

void Item::serialize(GameState* gs, SerializeBuffer & serializer) {
	serializer.write_int(id);
	serializer.write_int(amount);
	properties.serialize(gs, serializer);
}

bool Item::operator ==(const Item & item) const {
	return is_same_item(item) && amount == item.amount;
}

void Item::deserialize(GameState* gs, SerializeBuffer & serializer) {
	serializer.read_int(id);
	serializer.read_int(amount);
	properties.deserialize(gs, serializer);
}

