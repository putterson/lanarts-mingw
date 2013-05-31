/*
 * tunnelgen.cpp:
 *  Tunnel generation algorithms
 */

#include <cassert>
#include <vector>
#include <cstring>

#include <lcommon/mtwist.h>
#include "tunnelgen.h"

namespace ldungeon_gen {

	struct TunnelSliceContext;

	/*Internal implementation class to connect a specified room to either another tunnel
	 * (accept_tunnel_entry = true) or to another room (always). */
	class TunnelGenImpl {
	public:
		enum {
			NO_TURN = 0, TURN_PERIMETER = 1, TURN_START = 2
		};
		TunnelGenImpl(Map& map, MTwist& mt, Selector is_invalid,
				Selector is_finished, ConditionalOperator fill_oper,
				ConditionalOperator perimeter_oper, int start_room, int padding,
				int width, int depth, int change_odds,
				bool accept_tunnel_entry = false) :
						map(map),
						mt(mt),
						is_invalid(is_invalid),
						is_finished(is_finished),
						fill_oper(fill_oper),
						perimeter_oper(perimeter_oper),
						start_room(start_room),
						end_room(0),
						padding(padding),
						accept_tunnel_entry(accept_tunnel_entry),
						avoid_groupid(-1),
						width(width),
						maxdepth(depth),
						change_odds(change_odds) {
		}

	public:
		//Tunnel generation function, generates as described above
		bool generate(Pos p, int dx, int dy, std::vector<Square>& btbuff,
				std::vector<TunnelSliceContext>& tsbuff);

	private:
		Pos next_turn_position(TunnelSliceContext* cntxt, int& ndx, int& ndy);
		Pos next_tunnel_position(TunnelSliceContext* cntxt);

		void initialize_slice(TunnelSliceContext* cntxt, int turn_state, int dx,
				int dy, const Pos& pos);
		void backtrack_slice(Square* prev_content, TunnelSliceContext* cntxt,
				int dep);
		void tunnel_turning_slice(TunnelSliceContext* curr,
				TunnelSliceContext* next);
		void tunnel_straight_slice(TunnelSliceContext* cntxt,
				TunnelSliceContext* next);

		//all return true if valid
		bool validate_slice(Square* prev_content, TunnelSliceContext* cntxt,
				int dep);

	private:
		Map& map;
		MTwist& mt;
		Selector is_invalid, is_finished;
		ConditionalOperator perimeter_oper, fill_oper;
		int start_room;
		int end_room;
		int padding;
		bool accept_tunnel_entry;
		int avoid_groupid;
		int width;
		int maxdepth;
		int change_odds;
	};

	static Square* backtrack_entry(Square* backtracking, int entry_size,
			int entryn) {
		return &backtracking[entry_size * entryn];
	}

	struct TunnelSliceContext {
		//provided
		Pos p;
		int dx, dy;
		char turn_state;
		int attempt_number;

		//calculated
		bool tunneled;
		Pos ip, newpos;
	};

	void TunnelGenImpl::initialize_slice(TunnelSliceContext* cntxt,
			int turn_state, int dx, int dy, const Pos& pos) {
		cntxt->turn_state = turn_state;
		cntxt->dx = dx, cntxt->dy = dy;
		cntxt->p = pos;
		cntxt->ip = Pos(pos.x - (dy != 0 ? 1 : 0), pos.y - (dx != 0 ? 1 : 0));
		cntxt->attempt_number = 0;
	}

	void TunnelGenImpl::backtrack_slice(Square* prev_content,
			TunnelSliceContext* cntxt, int dep) {
		int dx = cntxt->dx, dy = cntxt->dy;

		for (int i = 0; i < width + padding * 2; i++) {
			int xcomp = (dy == 0 ? 0 : i);
			int ycomp = (dx == 0 ? 0 : i);
			map[Pos(cntxt->ip.x + xcomp, cntxt->ip.y + ycomp)] =
					prev_content[i];
		}

	}
	bool TunnelGenImpl::validate_slice(Square* prev_content,
			TunnelSliceContext* cntxt, int dep) {
		int dx = cntxt->dx, dy = cntxt->dy;
		if (cntxt->p.x <= 2
				|| cntxt->p.x
						>= map.width() - width - (dy == 0 ? 0 : padding * 2))
			return false;
		if (cntxt->p.y <= 2
				|| cntxt->p.y
						>= map.height() - width - (dx == 0 ? 0 : padding * 2))
			return false;

		/* Start with the assumption that we're done */
		cntxt->tunneled = true;

		for (int i = 0; i < width; i++) {
			int xcomp = (dy == 0 ? 0 : i);
			int ycomp = (dx == 0 ? 0 : i);
			Square& sqr = map[Pos(cntxt->p.x + xcomp, cntxt->p.y + ycomp)];

			/* Look for reasons we would not be done */
//			if (sqr.matches_flags(FLAG_SOLID) || sqr.group == 0//|| (!accept_tunnel_entry && sqr.group == 0)
			if (!sqr.matches(is_finished)) {
				cntxt->tunneled = false;
				break;
			}
			if (sqr.group == avoid_groupid)
				return false;
			if (sqr.group == start_room)
				return false;
		}

		if (cntxt->tunneled) {
			/* Do not finish tunnelling in the middle of a turn */
			if (cntxt->turn_state != NO_TURN)
				return false;
			return true;
		}

		for (int i = 0; i < width + padding * 2; i++) {
			int xcomp = (dy == 0 ? 0 : i);
			int ycomp = (dx == 0 ? 0 : i);
			Square& sqr = map[Pos(cntxt->ip.x + xcomp, cntxt->ip.y + ycomp)];
//			if (!sqr.matches_flags(FLAG_SOLID)
//					|| (!accept_tunnel_entry && cntxt->turn_state == NO_TURN
//							&& sqr.matches_flags(FLAG_PERIMETER)
//							&& sqr.group == 0))
			if (sqr.matches(is_invalid)) {
				return false;
			}
			memcpy(prev_content + i, &sqr, sizeof(Square));
		}
		return true;
	}

	Pos TunnelGenImpl::next_turn_position(TunnelSliceContext* cntxt, int& ndx,
			int& ndy) {
		Pos newpos;
		int dx = cntxt->dx, dy = cntxt->dy;
		bool positive = mt.rand(2);

		if (dx == 0) {
			if (positive)
				newpos.x = cntxt->p.x + width;
			else
				newpos.x = cntxt->p.x - 1;
			ndx = positive ? +1 : -1;
		} else {
			ndx = 0;
			newpos.x = dx > 0 ? cntxt->p.x - width : cntxt->p.x + 1;
		}

		if (dy == 0) {
			if (positive)
				newpos.y = cntxt->p.y + width;
			else
				newpos.y = cntxt->p.y - 1;
			ndy = positive ? +1 : -1;
		} else {
			ndy = 0;
			newpos.y = dy > 0 ? cntxt->p.y - width : cntxt->p.y + 1;
		}
		return newpos;
	}

	Pos TunnelGenImpl::next_tunnel_position(TunnelSliceContext* cntxt) {
		Pos newpos;
		int dx = cntxt->dx, dy = cntxt->dy;

		if (dy == 0)
			newpos.y = cntxt->p.y;
		else if (dy > 0)
			newpos.y = cntxt->p.y + 1;
		else
			newpos.y = cntxt->p.y - 1;

		if (dx == 0)
			newpos.x = cntxt->p.x;
		else if (dx > 0)
			newpos.x = cntxt->p.x + 1;
		else
			newpos.x = cntxt->p.x - 1;

		return newpos;
	}

	void TunnelGenImpl::tunnel_straight_slice(TunnelSliceContext* cntxt,
			TunnelSliceContext* next) {
		int dx = cntxt->dx, dy = cntxt->dy;

		map[cntxt->ip].apply(perimeter_oper);
		for (int i = 0; i < width; i++) {
			int xcomp = (dy == 0 ? 0 : i);
			int ycomp = (dx == 0 ? 0 : i);

			Square& sqr = map[Pos(cntxt->p.x + xcomp, cntxt->p.y + ycomp)];
			sqr.apply(fill_oper);
		}

		map[Pos(cntxt->ip.x + (dy == 0 ? 0 : width + 1),
				cntxt->ip.y + (dx == 0 ? 0 : width + 1))].apply(perimeter_oper);

		Pos newpos = next_tunnel_position(cntxt);

		bool start_turn = cntxt->turn_state == NO_TURN
				&& mt.rand(change_odds) == 0;
		initialize_slice(next, start_turn ? TURN_START : NO_TURN, dx, dy,
				newpos);
	}
	void TunnelGenImpl::tunnel_turning_slice(TunnelSliceContext* cntxt,
			TunnelSliceContext* next) {
		int dx = cntxt->dx, dy = cntxt->dy;
		int ndx, ndy;
		Pos newpos = next_turn_position(cntxt, ndx, ndy);

		for (int i = 0; i < width + padding * 2; i++) {
			int xcomp = (dy == 0 ? 0 : i);
			int ycomp = (dx == 0 ? 0 : i);
			Square& sqr = map[Pos(cntxt->ip.x + xcomp, cntxt->ip.y + ycomp)];
			sqr.apply(perimeter_oper);
		}
		initialize_slice(next, TURN_PERIMETER, ndx, ndy, newpos);
	}

	template<class T>
	void __resizebuff(T& t, size_t size) {
		if (t.size() <= size / 2)
			t.resize(size);
		else if (t.size() < size)
			t.resize(t.size() * 2);
	}

	bool TunnelGenOperator::apply(Map& map,
			group_t parent_group_id, const BBox& rect) {

		Pos p;
		bool axis, positive;

		std::vector<Square> btbuff;
		std::vector<TunnelSliceContext> tsbuff;

		std::vector<int> genpaths(map.groups.size(), 0);
		std::vector<int> totalpaths(map.groups.size(), 0);
		for (int i = 0; i < map.groups.size(); i++) {
			totalpaths[i] = randomizer.rand(num_tunnels);
		}
		int nogen_tries = 0;
		while (nogen_tries < 200) {
			nogen_tries++;

			for (int i = 0; i < map.groups.size(); i++) {
				if (genpaths[i] >= totalpaths[i])
					continue;
				bool generated = false;
				int genwidth = randomizer.rand(size);
				for (; genwidth >= 1 && !generated; genwidth--) {
					int path_len = 5;
					for (int attempts = 0; attempts < 16 && !generated;
							attempts++) {
						TunnelGenImpl tg(map, randomizer, is_invalid,
								is_finished, fill_oper, perimeter_oper, i + 1,
								padding, genwidth, path_len, 20,
								padding > 0
										&& (genpaths[i] > 0 || nogen_tries > 100));

						generate_entrance(map.groups[i].group_area, randomizer,
								std::min(genwidth, 2), p, axis, positive);

						int val = positive ? +1 : -1;
						int dx = axis ? 0 : val, dy = axis ? val : 0;

						if (tg.generate(p, dx, dy, btbuff, tsbuff)) {
							genpaths[i]++;
							nogen_tries = 0;
							path_len = 5;
							generated = true;
						}
						if (attempts >= 4) {
							path_len += 5;
						}
					}
				}
			}
		}
	}
	bool TunnelGenImpl::generate(Pos p, int dx, int dy,
			std::vector<Square>& btbuff,
			std::vector<TunnelSliceContext>& tsbuff) {

		int entry_size = width + padding * 2;

		__resizebuff(btbuff, entry_size * maxdepth);
		__resizebuff(tsbuff, maxdepth);

		Square* backtracking = &btbuff[0];
		TunnelSliceContext* tsc = &tsbuff[0];

		Square* prev_content;
		TunnelSliceContext* cntxt;

		bool complete_tunnel = false;
		int tunnel_depth = 0;

//By setting TURN_PERIMETER we avoid trying a turn on the first tunnel slice
		initialize_slice(&tsc[tunnel_depth], TURN_PERIMETER, dx, dy, p);
		while (true) {
			prev_content = backtrack_entry(backtracking, entry_size,
					tunnel_depth);
			cntxt = &tsc[tunnel_depth];

			//We must leave room to initialize the next tunnel depth
			bool valid =
					tunnel_depth < maxdepth - 1
							&& (tsc[tunnel_depth].attempt_number > 0
									|| validate_slice(prev_content, cntxt,
											tunnel_depth));
			if (valid && cntxt->attempt_number <= 0) {

				if (cntxt->tunneled) {
					end_room = map[p].group;
					complete_tunnel = true;
					break;
				}

				if (cntxt->turn_state == TURN_START) {
					this->tunnel_turning_slice(cntxt, &tsc[tunnel_depth + 1]);
				} else {
					this->tunnel_straight_slice(cntxt, &tsc[tunnel_depth + 1]);
				}

				cntxt->attempt_number++;
				tunnel_depth++;
			} else {
				tunnel_depth--;
				if (tunnel_depth < 0)
					break;

				//set values to those of previous depth
				prev_content = backtrack_entry(backtracking, entry_size,
						tunnel_depth);
				cntxt = &tsc[tunnel_depth];

				backtrack_slice(prev_content, cntxt, tunnel_depth);
			}
		}

		return complete_tunnel;
	}

	void generate_entrance(const BBox& bbox, MTwist& mt, int len, Pos& p,
			bool& axis, bool& positive) {
		int ind;
		axis = mt.rand(2), positive = mt.rand(2);
		if (axis) {
			int rmx = bbox.x2 - len;
			if (rmx == bbox.x1 + 1)
				ind = rmx;
			else
				ind = mt.rand(bbox.x1 + 1, rmx);
			p.y = positive ? bbox.y2 : bbox.y1 - 1;
			p.x = ind;
		} else {
			int rmy = bbox.y2 - len;
			if (rmy == bbox.y1 + 1)
				ind = rmy;
			else
				ind = mt.rand(bbox.y1 + 1, rmy);
			p.x = positive ? bbox.x2 : bbox.x1 - 1;
			p.y = ind;
		}
	}

}