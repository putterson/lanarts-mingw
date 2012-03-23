/*
 * dungeon_data.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: 100397561
 */

#include "dungeon_data.h"

//Standard width and height
const int STD_W = 128, STD_H = 128;
ItemGenSettings itemdefault(1 /*more = stronger items*/, 10,
		10 /*10 to 10 items*/);

RoomGenSettings roomdensevaried(1 /*padding*/, 90 /*amount of rooms*/,
		4 /*min size*/, 30 /*max size*/);
RoomGenSettings roommediumhighpad(3 /*padding*/, 30 /*amount of rooms*/,
		4 /*min size*/, 20 /*max size*/);
RoomGenSettings roomsparsesmall(1 /*padding*/, 15 /*amount of rooms*/,
		4 /*min size*/, 9 /*max size*/);

TunnelGenSettings tunneldefault(1, 3 /*1 to 3 width tunnels*/, 1,
		20 /*1 to 20 tunnels per room*/);
TunnelGenSettings tunnelwide(1, 5 /*1 to 5 width tunnels*/, 1,
		20 /*1 to 20 tunnels per room*/);

FeatureGenSettings featuredefault(3, 3 /*3 stairs up, 3 stairs down*/,
		1 /*Default tileset*/);

FeatureGenSettings featurefirstlevel(0, 3 /*0 stairs up, 3 stairs down*/,
		1 /*Default tileset*/);

EnemyGenSettings enemyfew(1, 2, 20);
EnemyGenSettings enemymed(1, 2, 30);
EnemyGenSettings enemymany(1, 2, 40);

#define BRANCH(arr) DungeonBranch(arr, sizeof(arr)/sizeof(LevelGenSettings))

static LevelGenSettings mainbranch[] = { //Attempt
		LevelGenSettings(STD_W * 0.5, STD_H * 0.5, itemdefault, roomdensevaried,
				tunneldefault, featurefirstlevel, enemyfew),
		LevelGenSettings(
				STD_W * 0.5, STD_H * 0.5, itemdefault, roomsparsesmall,
				tunneldefault, featuredefault, enemymed),
		LevelGenSettings(
				STD_W * 0.6, STD_H * 0.6, itemdefault, roommediumhighpad,
				tunneldefault, featuredefault, enemymed),
		LevelGenSettings(
				STD_W * 0.7, STD_H * 0.7, itemdefault, roommediumhighpad,
				tunneldefault, featuredefault, enemymed),
		LevelGenSettings(
				STD_W * 0.7, STD_H * 0.7, itemdefault, roomsparsesmall,
				tunneldefault, featuredefault, enemymany),
		LevelGenSettings(
				STD_W * 1, STD_H * 1, itemdefault, roommediumhighpad,
				tunnelwide, featuredefault, enemymany),
		LevelGenSettings(
				STD_W * 1, STD_H * 1, itemdefault, roommediumhighpad,
				tunnelwide, featuredefault, EnemyGenSettings(1, 2, 50)),
		LevelGenSettings(STD_W * 1, STD_H * 1, itemdefault,
				roommediumhighpad, tunnelwide, featuredefault,
				EnemyGenSettings(1, 2, 50)), };

DungeonBranch game_dungeon_data[] = { BRANCH(mainbranch) };

size_t game_dungeon_n = sizeof(game_dungeon_data) / sizeof(DungeonBranch);
