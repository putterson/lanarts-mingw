/*
 * InventoryContent.cpp:
 *  Represents an interactive view of the inventory for the side bar
 */

#include <ldraw/draw.h>

#include "draw/colour_constants.h"
#include "draw/draw_sprite.h"

#include "draw/SpriteEntry.h"

#include "data/lua_util.h"

#include "gamestate/GameState.h"
#include "objects/PlayerInst.h"

#include "stats/items/ItemEntry.h"

#include "../console_description_draw.h"

#include "InventoryContent.h"

//Draw in bottom-right of slot
static void draw_slot_cost(GameState* gs, money_t cost, int x, int y) {
    using namespace ldraw;
    gs->font().drawf(DrawOptions(COL_PALE_YELLOW).origin(CENTER_BOTTOM),
            Pos(x + TILE_SIZE / 2, y + TILE_SIZE - 2), "+%d", cost);
}


static void draw_player_inventory_slot(GameState* gs, LuaValue handler, ItemSlot& itemslot, int x,
		int y) {
	if (itemslot.amount() > 0) {
		ItemEntry& ientry = itemslot.item_entry();
		ldraw::DrawOptions options;
        if (gs->io_controller().ctrl_held()) {
            options.draw_colour.a *= 0.5; // 50% opacity
        }
		ientry.item_image().draw(options, Pos(x,y));

        if (ientry.stackable) {
			gs->font().drawf(COL_WHITE, Pos(x+1, y+1), "%d", itemslot.amount());
		}

        if (gs->io_controller().ctrl_held()) {
            if (itemslot.item_entry().sellable) {
                draw_slot_cost(gs, itemslot.item_entry().sell_cost(), x, y);
            }
        }
	}
}

static void draw_player_inventory(GameState* gs, LuaValue handler, Inventory& inv,
		const BBox& bbox, int min_slot, int max_slot, int slot_selected) {
	int mx = gs->mouse_x(), my = gs->mouse_y();
	int slot = min_slot;
    int slot_highlighted = lmethod_call<int>(handler, "slot_highlighted");
    int handler_slot_selected = lmethod_call<int>(handler, "slot_selected");
	for (int y = bbox.y1; y < bbox.y2; y += TILE_SIZE) {
		for (int x = bbox.x1; x < bbox.x2; x += TILE_SIZE) {
			if (slot >= max_slot || slot >= inv.max_size())
				break;

			ItemSlot& itemslot = inv.get(slot);

			BBox slotbox(x, y, x + TILE_SIZE, y + TILE_SIZE);
			Colour outline_col(COL_UNFILLED_OUTLINE);
			if (itemslot.amount() > 0 && slot != slot_selected) {
				if (itemslot.is_equipped()) {
					ldraw::draw_rectangle(Colour(25, 50, 10), slotbox);
				}
				outline_col = COL_FILLED_OUTLINE;

				if (slotbox.contains(mx, my)) {
					outline_col = COL_PALE_YELLOW;
					draw_console_item_description(gs, itemslot.item,
							itemslot.item_entry());
				} else if (slot == handler_slot_selected) {
                    outline_col = COL_GREEN;
                }
			}
            if (slot == slot_highlighted) {
                outline_col = COL_PALE_GREEN;
                if (!itemslot.empty())
                draw_console_item_description(gs, itemslot.item,
                                              itemslot.item_entry());
            }
            if (slot == handler_slot_selected) {
                outline_col = COL_GREEN;
            }
			if (slot != slot_selected)
				draw_player_inventory_slot(gs, handler, itemslot, x, y);
			//draw rectangle over item edges
			ldraw::draw_rectangle_outline(outline_col, slotbox);

			slot++;
		}
	}

	if (slot_selected != -1) {
		draw_player_inventory_slot(gs, handler, inv.get(slot_selected),
				gs->mouse_x() - TILE_SIZE / 2, gs->mouse_y() - TILE_SIZE / 2);
	}
}

static int get_itemslotn(Inventory& inv, const BBox& bbox, int mx, int my) {
	if (!bbox.contains(mx, my)) {
		return -1;
	}

	int posx = (mx - bbox.x1) / TILE_SIZE;
	int posy = (my - bbox.y1) / TILE_SIZE;
	int slot = 5 * posy + posx;

	if (slot < 0 || slot >= inv.max_size())
		return -1;

	return slot;
}

const int ITEMS_PER_PAGE = 40;

void InventoryContent::draw(GameState* gs) const {
	ensure_init(gs);
	PlayerInst* p = gs->local_player();

	Inventory& inv = p->inventory();
	int min_item = ITEMS_PER_PAGE * page_number, max_item = min_item
			+ ITEMS_PER_PAGE;
	draw_player_inventory(gs, handler, inv, bbox, min_item, max_item, slot_selected);
}

int InventoryContent::amount_of_pages(GameState* gs) {
	PlayerInst* p = gs->local_player();

	size_t items_n = p->inventory().last_filled_slot();
	/* Add ITEMS_PER_PAGE - 1 so that 0 spells need 0 pages, 1 spell needs 1 page, etc*/
	size_t item_pages = (items_n + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;

	return (int)item_pages;
}

bool InventoryContent::handle_io(GameState* gs, ActionQueue& queued_actions) {
	ensure_init(gs);
	PlayerInst* p = gs->local_player();
	Inventory& inv = p->inventory();
	int mx = gs->mouse_x(), my = gs->mouse_y();
	bool within_inventory = bbox.contains(mx, my);

	/* Use an item */
	if (gs->mouse_left_click() && within_inventory) {
		int slot = get_itemslotn(inv, bbox, mx, my);
		auto action_type = gs->io_controller().ctrl_held() ? GameAction::SELL_ITEM : GameAction::USE_ITEM;
		if (slot >= 0 && slot < INVENTORY_SIZE && inv.get(slot).amount() > 0) {
			queued_actions.push_back(
					game_action(gs, p, action_type, slot, p->x, p->y));
			return true;
		}
	}

	/* Start dragging an item */
	if (gs->mouse_right_click() && within_inventory) {
		int slot = get_itemslotn(inv, bbox, mx, my);
		if (slot != -1 && inv.slot_filled(slot)) {
			slot_selected = slot;
			return true;
		}
	}
    auto drop_item_slot = [&] (int slot, bool drop_half) {
	if (slot < 0 || slot >= inv.max_size())
            return;
        if (inv.get(slot).empty()) {
            return;
        }
        queued_actions.push_back(
                game_action(gs, p, GameAction::DROP_ITEM, slot, 0,0, drop_half));
    };
    auto reposition_item = [&] (int slot1, int slot2) {
	if (slot1 < 0 || slot1 >= inv.max_size())
            return;
	if (slot2 < 0 || slot2 >= inv.max_size())
            return;
        queued_actions.push_back(
                game_action(gs, p, GameAction::REPOSITION_ITEM, slot1, 0, 0, slot2));
    };
    install_closure(drop_item_slot);
    install_closure(reposition_item);
    lmethod_call<void>(handler, "handle_inventory", drop_item_slot, reposition_item);

	/* Drop a dragged item */
	if (slot_selected > -1 && gs->mouse_right_release()) {
		int slot = get_itemslotn(inv, bbox, mx, my);
		if (slot == -1 || slot == slot_selected) {
            drop_item_slot(slot_selected, gs->io_controller().shift_held());
		} else {
            reposition_item(slot_selected, slot);
		}
		return true;
	}

	return false;
}

void InventoryContent::ensure_init(GameState* gs) const {
	if (handler.empty()) {
		handler = gs->local_player()->input_source().value;//.raw_call("inventory_content");
	}
}

