/*
 * GameAction.cpp:
 *  Represents an action taken by a player instance.
 *  Formatted for sending over network.
 */

#include "objects/GameInst.h"
#include "GameAction.h"
#include "GameMapState.h"

#include "GameState.h"

GameAction::GameAction() :
		origin(0), act((action_t)0), frame(0), room(0), use_id(0), use_id2(0), action_x(
				0), action_y(0) {
}

GameAction::GameAction(obj_id origin, action_t act, int frame, int level,
		int use_id, float action_x, float action_y, int use_id2) :
		origin(origin), act(act), frame(frame), room(level), use_id(use_id), use_id2(
				use_id2), action_x(action_x), action_y(action_y) {

}
void to_action_file(FILE* f, const GameAction& action) {
	fwrite(&action, sizeof(GameAction), 1, f);
}
GameAction game_action(GameState* gs, GameInst* origin,
		GameAction::action_t action, int use_id, float action_x, float action_y,
		int use_id2) {
	return GameAction(origin->id, action, gs->frame(),
			gs->get_level()->id(), use_id, action_x, action_y, use_id2);
}

GameAction from_action_file(FILE* f) {
	char data[sizeof(GameAction)];
	fread(data, sizeof(GameAction), 1, f);
	return *(GameAction*) data;
}

