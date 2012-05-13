/*
 * itemgen.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: 100397561
 */

#include "itemgen.h"
#include "../world/GameState.h"
#include "../world/GameTiles.h"
#include "../world/objects/ItemInst.h"
#include "../data/item_data.h"


void generate_items(const ItemGenSettings& is, MTwist& mt, GeneratedLevel& level, GameState* gs){
	GameTiles& tiles = gs->tile_grid();
	int start_x = (tiles.tile_width()-level.width())/2;
	int start_y = (tiles.tile_height()-level.height())/2;

	int amount = mt.rand(is.min_items, is.max_items+1);

	int total_chance = 0;

	for (int i = 0; i < is.item_chances.size(); i++){
		total_chance += is.item_chances[i].genchance;
	}
	if (is.item_chances.empty()) return;

	//generate gold
	for (int i = 0; i < amount; i++) {
		Pos ipos = generate_location(mt, level);
		int ix = (ipos.x+start_x) * TILE_SIZE + TILE_SIZE/2;
		int iy = (ipos.y+start_y) * TILE_SIZE + TILE_SIZE/2;
		
		int item_roll = mt.rand(total_chance);
		int item_type;
		int itemn;
		for (itemn = 0; itemn < is.item_chances.size(); itemn++){
			item_roll -= is.item_chances[itemn].genchance;
			item_type = is.item_chances[itemn].itemtype;
			if (item_roll < 0)
				break;
		}
		gs->add_instance(new ItemInst(item_type, ix,iy));
		level.at(ipos).has_instance = true;
	}
}
