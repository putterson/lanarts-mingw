/*
 * Map.h:
 *  An abstract map format, where each square has a set of labels and a content number.
 *  16 bits are set out for labels. 8 are predefined, 8 are user defined.
 *  Provides efficient & flexible mechanisms for querying & operating on large areas.
 */

#ifndef LDUNGEON_MAP_H_
#define LDUNGEON_MAP_H_

#include <lcommon/geometry.h>
#include <lcommon/Grid.h>
#include <lcommon/int_types.h>
#include <lcommon/mtwist.h>

class SerializeBuffer;

namespace ldungeon_gen {

	const uint16 FLAG_SOLID = 1 << 0;
	const uint16 FLAG_PERIMETER = 1 << 1;
	const uint16 FLAG_TUNNEL = 1 << 2;
	const uint16 FLAG_HAS_OBJECT = 1 << 3;
	const uint16 FLAG_NEAR_PORTAL = 1 << 4;
	/* For future use */
	const uint16 FLAG_RESERVED1 = 1 << 5;
	const uint16 FLAG_RESERVED2 = 1 << 6;
	const uint16 FLAG_RESERVED3 = 1 << 7;

	/* Identifies squares to act on based on their fields.
	 */
	struct Selector {
		uint16 must_be_on_bits;
		uint16 must_be_off_bits;
		uint16 must_be_content;
		bool use_must_be_value;

		Selector(uint16 must_be_on_bits, uint16 must_be_off_bits,
				uint16 must_be_content) :
						must_be_on_bits(must_be_on_bits),
						must_be_off_bits(must_be_off_bits),
						must_be_content(must_be_content),
						use_must_be_value(true) {
		}

		Selector(uint16 must_be_on_bits = 0, uint16 must_be_off_bits = 0) :
						must_be_on_bits(must_be_on_bits),
						must_be_off_bits(must_be_off_bits),
						must_be_content(0),
						use_must_be_value(false) {
		}

	};

	/* Operates on the labels and content of a square.
	 * The 'most obvious' use case is clearing out an area.
	 */
	struct Operator {
		uint16 turn_on_bits;
		uint16 turn_off_bits;
		uint16 flip_bits;
		uint16 content_value;
		bool use_content_value;

		Operator(uint16 turn_on_bits, uint16 turn_off_bits, uint16 flip_bits,
				uint16 content_value) :
						turn_on_bits(turn_on_bits),
						turn_off_bits(turn_off_bits),
						flip_bits(flip_bits),
						content_value(content_value),
						use_content_value(true) {
		}

		Operator(uint16 turn_on_bits = 0, uint16 turn_off_bits = 0, uint16 flip_bits = 0) :
						turn_on_bits(turn_on_bits),
						turn_off_bits(turn_off_bits),
						flip_bits(flip_bits),
						content_value(0),
						use_content_value(false) {
		}
	};

	struct ConditionalOperator {
		Selector selector;
		Operator oper;
		ConditionalOperator(Selector selector, Operator oper) :
						selector(selector),
						oper(oper) {
		}
	};

	struct Square {
		/* Bits 8 through 16 are used for 'user-defined flags.
		 * These are labeled in the lua-side. */
		uint16 flags;
		/* Content number, based on flags */
		uint16 content;
		/* Group this square belongs to */
		uint16 group;

		Square(uint16 flags = FLAG_SOLID, uint16 content = 0, uint16 group = 0) :
						flags(flags),
						content(content),
						group(group) {
		}
		inline bool matches(Selector selector) const {
			return ((selector.must_be_on_bits & flags)
					== selector.must_be_on_bits)
					&& ((selector.must_be_off_bits & ~flags)
							== selector.must_be_off_bits)
					&& (!selector.use_must_be_value
							|| content == selector.must_be_content);
		}
		inline void apply(Operator oper) {
			flags &= ~oper.turn_off_bits;
			flags |= oper.turn_on_bits;
			flags ^= oper.flip_bits;
			if (oper.use_content_value) {
				content = oper.content_value;
			}
		}
		inline void apply(ConditionalOperator oper) {
			if (matches(oper.selector)) {
				apply(oper.oper);
			}
		}
		bool matches_flags(uint16 flags) const {
			return this->flags & flags;
		}
	};

	struct Group {
		/* Group ID relationship */
		int group_id, parent_group_id;
		std::vector<int> child_group_ids;
		/* Bounding box of area */
		BBox group_area;

		Group(int group_id, int parent_group_id, const BBox& group_area) :
						group_id(group_id),
						parent_group_id(parent_group_id),
						group_area(group_area) {
		}

		void serialize(SerializeBuffer& serializer) const;
		void deserialize(SerializeBuffer& serializer);
	};

	const int ROOT_GROUP_ID = 0;
	typedef int group_t;

	/* Inheritance used for Grid methods */
	class Map: public Grid<Square> {
	public:
		Map(const Size& size = Size(), const Square& fill_value = Square());
		std::vector<Group> groups;
		group_t make_group(const BBox& area, int parent_group_id);

		/* For testing purposes with serialization */
		bool operator==(const Map& map) const;
		bool operator!=(const Map& map) const {
			return !(*this == map);
		}

		void serialize(SerializeBuffer& serializer) const;
		void deserialize(SerializeBuffer& serializer);
	};
}

#endif /* MAP_H_ */