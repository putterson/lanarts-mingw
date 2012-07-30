/*
 * GameLevelState.cpp:
 *  Contains game level state information
 */

#include "GameLevelState.h"

GameLevelState::GameLevelState(int levelid, int w, int h, bool wandering_flag,
		bool is_simulation) :
		levelid(levelid), steps_left(0), width(w), height(h), tiles(
				w / TILE_SIZE, h / TILE_SIZE), inst_set(w, h), mc(
				wandering_flag), is_simulation(is_simulation) {
}

GameLevelState::~GameLevelState() {
}

void GameLevelState::copy_to(GameLevelState & level) const {
	level.entrances = this->entrances; //Copy exits&entrances just in case
	level.exits = this->exits; //However we will typically copy_to just to synch
	this->inst_set.copy_to(level.inst_set);
	level.is_simulation = this->is_simulation;
	tiles.copy_to(level.tiles);
	this->mc.partial_copy_to(level.mc);
	level.mc.finish_copy(&level);
	level.is_simulation = this->is_simulation;
	level.steps_left = this->steps_left;
}

int GameLevelState::room_within(const Pos& p) {
	for (int i = 0; i < rooms.size(); i++) {
		int px = p.x / TILE_SIZE, py = p.y / TILE_SIZE;
		const Region & r = rooms[i].room_region;
		if (r.x <= px && r.x + r.w >= px) {
			if (r.y <= py && r.y + r.h >= py) {
				return i;
			}
		}

	}
	return -1;
}

GameLevelState* GameLevelState::clone() const {
	GameLevelState* state = new GameLevelState(levelid, width, height,
			is_simulation);
	copy_to(*state);
	return state;
}

