/*
 * StoreInst.cpp:
 *  Represents a store NPC or store tile
 */
#include <typeinfo>

#include "draw/colour_constants.h"

#include "draw/SpriteEntry.h"
#include "gamestate/GameState.h"

#include <lcommon/SerializeBuffer.h>

#include "stats/items/ItemEntry.h"
#include <lcommon/math_util.h>
#include "objects/collision_filters.h"

#include "StoreInst.h"

const int MAX_PLAYERS_IN_STORE = 25;

void StoreInst::step(GameState* gs) {
	if (gs->object_visible_test(this)) {
		last_seen_spr = spriteid;
	}
	GameInst* players[MAX_PLAYERS_IN_STORE];

	int nplayers = gs->object_radius_test(this, players, MAX_PLAYERS_IN_STORE,
			player_colfilter, x, y, 24);

    gs->for_screens([&]() {
        for (int i = 0; i < nplayers; i++ ) {
            if ((PlayerInst*)players[i] == gs->local_player()) {
                gs->game_hud().override_sidebar_contents(&sidebar_display);
            }
        }
    });
}

void StoreInst::draw(GameState* gs) {
	sidebar_display.init(this, gs->game_hud().sidebar_content_area());
	Colour drawcolour;
	if (gs->object_radius_test(this, NULL, 0, player_colfilter, x, y, 24)) {
		drawcolour = Colour(255, 255, 100, 255);
	}

	if (last_seen_spr > -1) {
		draw_sprite_centered(gs->view(), last_seen_spr, x, y, 0,0, spr_frame, drawcolour);
	}
}

void StoreInst::copy_to(GameInst* inst) const {
	LANARTS_ASSERT(typeid(*this) == typeid(*inst));
	*(StoreInst*)inst = *this;
}

void StoreInst::init(GameState* gs) {
	GameInst::init(gs);
	sidebar_display = StoreContent();
}

StoreInst* StoreInst::clone() const {
	return new StoreInst(*this);
}

void StoreInst::serialize(GameState* gs, SerializeBuffer& serializer) {
	GameInst::serialize(gs, serializer);
	inv.serialize(serializer);
	serializer.write(last_seen_spr);
	serializer.write(spriteid);
	serializer.write(spr_frame);
}

void StoreInst::deserialize(GameState* gs, SerializeBuffer& serializer) {
	GameInst::deserialize(gs, serializer);
	inv.deserialize(serializer);
	serializer.read(last_seen_spr);
	serializer.read(spriteid);
	serializer.read(spr_frame);
	sidebar_display = StoreContent();
}

