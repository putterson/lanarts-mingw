/*
 * game_replays.cpp:
 *  Utilities for storing and replaying game replays
 */

#include <deque>

#include <lcommon/strformat.h>

#include "draw/colour_constants.h"

#include "gamestate/GameAction.h"
#include "gamestate/GameState.h"

static FILE* loadfile = NULL;
static FILE* savefile = NULL;
static std::deque<GameAction> loadbuffer;

static std::string find_next_nonexistant(std::string* prev,
		const std::string& name) {
	std::string suffix = ".rep";
	if (prev) {
		*prev = name + suffix;
	}
	for (int tries = 2;; tries++) {
		std::string ret = name + suffix;
		FILE* f = fopen(ret.c_str(), "rb");
		if (!f) {
			return ret;
		}
		fclose(f);
		format(suffix, "%d.rep", tries);
		if (prev) {
			*prev = ret;
		}
	}
	return name;
}
void load_actions(GameState* gs, std::deque<GameAction>& actions) {
	int frame = gs->frame();

	while (true) {
		GameAction buff;
		int nread = fread(&buff, sizeof(GameAction), 1, loadfile);
		if (!nread) {
			fclose(loadfile);
			loadfile = NULL;
            gs->for_screens([&]() {
                gs->game_chat().add_message("*** Replay has finished ***", COL_RED);
            });
			gs->game_settings().loadreplay_file = "";
			break;
		}
		loadbuffer.push_back(buff);
		if (buff.frame > frame) {
			break;
		}
	}
	while (!loadbuffer.empty() && loadbuffer.front().frame == frame) {
		actions.push_back(loadbuffer.front());
		loadbuffer.pop_front();
	}
}
void save_actions(GameState* gs, std::deque<GameAction>& actions) {
	if (!actions.empty()) {
		fwrite(&actions[0], sizeof(GameAction), actions.size(), savefile);
	}
	fflush(savefile);
}

void load_init(GameState* gs, int& seed, class_id& classtype) {
	std::string last_used_name;
	find_next_nonexistant(&last_used_name, gs->game_settings().loadreplay_file);
	loadfile = fopen(last_used_name.c_str(), "rb");
	if (!savefile) {
		printf("Cannot open %s!\n", last_used_name.c_str());
		gs->game_settings().loadreplay_file = "";
	} else {
		fread(&seed, sizeof(unsigned int), 1, loadfile);
		fread(&classtype, sizeof(class_id), 1, loadfile);
	}
}

void save_init(GameState* gs, int seed, class_id classtype) {
	std::string next_free_name = find_next_nonexistant(NULL,
			gs->game_settings().savereplay_file);

	savefile = fopen(next_free_name.c_str(), "wb");
	if (!savefile) {
		printf("Cannot open %s!\n", next_free_name.c_str());
		gs->game_settings().savereplay_file = "";
	} else {
		fwrite(&seed, sizeof(unsigned int), 1, savefile);
		fwrite(&classtype, sizeof(class_id), 1, savefile);
	}
}

bool replay_exists(GameState* gs) {
	FILE* f = fopen(gs->game_settings().loadreplay_file.c_str(), "r");
	if (f != NULL) {
		fclose(f);
		return true;
	}
	return false;
}

