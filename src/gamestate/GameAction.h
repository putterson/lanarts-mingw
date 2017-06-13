/*
 * GameAction.h:
 *  Represents an action taken by a player instance.
 *  Formatted for sending over network.
 */

#ifndef GAMEACTION_H_
#define GAMEACTION_H_
#include <cstdio>

#include "lanarts_defines.h"

class GameInst;
class GameState;

struct GameAction {
	enum action_t {
		MOVE,
		USE_SPELL,
		USE_REST,
		USE_PORTAL,
		USE_ITEM,
		SELL_ITEM,
		USE_WEAPON,
		PICKUP_ITEM,
		DROP_ITEM,
		//TODO: Ensure this doesn't duplicate purpose of use-item <equipped>
		DEEQUIP_ITEM,
		REPOSITION_ITEM,
		CHOSE_SPELL,
		PURCHASE_FROM_STORE
	};
	GameAction();
	GameAction(obj_id origin, action_t act, int frame, int level,
			int use_id = 0, float action_x = 0, float action_y = 0,
			int use_id2 = 0);
	obj_id origin;
	action_t act;
	//TODO: Remove frame from GameAction informaiton
	int frame, room;
	int use_id, use_id2;
	float action_x, action_y;
};

GameAction game_action(GameState* gs, GameInst* origin,
		GameAction::action_t action, int use_id = 0, float action_x = 0,
		float action_y = 0, int use_id2 = 0);

void to_action_file(FILE* f, const GameAction& action);
GameAction from_action_file(FILE* f);

#endif /* GAMEACTION_H_ */
