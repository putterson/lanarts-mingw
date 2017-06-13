/*
 * adv_net_tests.cpp:
 *  Tests sending lanarts specific messages
 */

#include <cstring>
#include <cstdio>

#include <vector>
#include <string>

#include <SDL_thread.h>

#include <lua.hpp>

#include <net-lib/lanarts_net.h>

#include <lcommon/unittest.h>

#include "gamestate/GameState.h"

#include "gamestate/PlayerData.h"
#include "net/GameNetConnection.h"

struct LanartsNetInitHelper {
	LanartsNetInitHelper() {
		lanarts_net_init(true);
	}
	~LanartsNetInitHelper() {
		lanarts_net_quit();
	}
};

const int TEST_POLL_TIMEOUT = 50;

SUITE(adv_net_unit_tests) {

//	struct NetUpdatedState {
//		GameNetConnection conn;
//		GameChat chat;
//		GameStateInitData init;
//		PlayerData pd;
//		NetUpdatedState() :
//						conn(chat, pd, init) {
//		}
//	};
//
//	static void test_state_helper(NetUpdatedState& netstate,
//			const char* servername, const char* clientname1,
//			const char* clientname2) {
//		std::vector<PlayerDataEntry>& pdes = netstate.pd.all_players();
//
//		CHECK_EQUAL(3, pdes.size());
//		for (int i = 0; i < pdes.size(); i++) {
//			CHECK(pdes.at(i).player_inst.get() == NULL);
//			CHECK(pdes.at(i).net_id == i);
//		}
//		/* test expected names */
//		CHECK(pdes.at(0).player_name == servername);
//		CHECK(pdes.at(1).player_name == clientname1);
//		CHECK(pdes.at(2).player_name == clientname2);
//	}
//	static void test_initstate_helper(GameStateInitData& client_init,
//			GameStateInitData& server_init) {
//		CHECK(client_init.received_init_data);
//		CHECK(client_init.seed == server_init.seed);
//		CHECK(client_init.network_debug_mode == server_init.network_debug_mode);
//		CHECK(client_init.regen_on_death == server_init.regen_on_death);
//		CHECK(client_init.time_per_step == server_init.time_per_step);
//	}
//
//	static void init_connections(NetUpdatedState& serverstate,
//			NetUpdatedState& client1state, NetUpdatedState& client2state) {
//
//		const int TEST_PORT = 6112;
//
//		const char* servername = "server";
//		const char* clientname1 = "client1";
//		const char* clientname2 = "client2";
//
//		auto callback = [](){return true;};
//
//		serverstate.conn.initialize_as_server(callback, TEST_PORT);
//		serverstate.pd.register_player("server", NULL, 0, 0);
//
//		client1state.conn.initialize_as_client(callback, "localhost", TEST_PORT);
//		client2state.conn.initialize_as_client(callback, "localhost", TEST_PORT);
//
//		serverstate.conn.poll_messages(TEST_POLL_TIMEOUT);
//		client1state.conn.poll_messages(TEST_POLL_TIMEOUT);
//		client2state.conn.poll_messages(TEST_POLL_TIMEOUT);
//
//		net_send_connection_affirm(client1state.conn, clientname1, 1);
//
//		serverstate.conn.poll_messages(TEST_POLL_TIMEOUT);
//		client1state.conn.poll_messages(TEST_POLL_TIMEOUT);
//		client2state.conn.poll_messages(TEST_POLL_TIMEOUT);
//
//		net_send_connection_affirm(client2state.conn, clientname2, 2);
//
//		serverstate.conn.poll_messages(TEST_POLL_TIMEOUT);
//		client1state.conn.poll_messages(TEST_POLL_TIMEOUT);
//		client2state.conn.poll_messages(TEST_POLL_TIMEOUT);
//
//		GameStateInitData server_init;
//		server_init.seed = 0x1;
//		server_init.network_debug_mode = true;
//		server_init.regen_on_death = true;
//		server_init.time_per_step = 1.1f;
//
//		net_send_game_init_data(serverstate.conn, serverstate.pd, server_init);
//
//		client1state.conn.poll_messages(TEST_POLL_TIMEOUT);
//		client2state.conn.poll_messages(TEST_POLL_TIMEOUT);
//
//		test_state_helper(serverstate, servername, clientname1, clientname2);
//
//		test_state_helper(client1state, servername, clientname1, clientname2);
//		test_initstate_helper(client1state.init, server_init);
//
//		test_state_helper(client2state, servername, clientname1, clientname2);
//		test_initstate_helper(client2state.init, server_init);
//
//	}
//	TEST(test_net_connect_affirm) {
//		LanartsNetInitHelper lanarts_net_init_helper;
//
//		NetUpdatedState serverstate, client1state, client2state;
//		init_connections(serverstate, client1state, client2state);
//	}
//
//	static void assert_game_actions_equals(GameAction& action1,
//			GameAction& action2) {
//		CHECK(action1.origin == action2.origin);
//		CHECK(action1.act == action2.act);
//		CHECK(action1.frame == action2.frame);
//		CHECK(action1.room == action2.room);
//	}
//
//	static void send_action(GameAction& action, int frame, int player,
//			NetUpdatedState& state) {
//		ActionQueue queue;
//		queue.push_back(action);
//		state.pd.all_players()[player].action_queue.queue_actions_for_frame(
//				queue, frame);
//		net_send_player_actions(state.conn, frame, player, queue);
//	}
//
//	static void test_net_actionqueue_assert(PlayerDataEntry& pde,
//			GameAction& act, int frame) {
//		ActionQueue action;
//		pde.action_queue.extract_actions_for_frame(action, frame);
//		CHECK(action.size() == 1);
//		assert_game_actions_equals(action.at(0), act);
//	}
//	static void test_net_action_assert(NetUpdatedState& state, GameAction& a1,
//			GameAction& a2, GameAction& a3, int frame) {
//		test_net_actionqueue_assert(state.pd.all_players().at(0), a1, frame);
//		test_net_actionqueue_assert(state.pd.all_players().at(1), a2, frame);
//		test_net_actionqueue_assert(state.pd.all_players().at(2), a3, frame);
//	}
//
//	static void poll_n(GameNetConnection& connection, int times) {
//		int polled = 0;
//		while (polled < times) {
//			int got = connection.poll_messages(TEST_POLL_TIMEOUT);
//			if (got == 0) {
//				return; // Failure, but go on
//			}
//			polled += got;
//		}
//	}
//	TEST(test_net_action_sending1) {
//		LanartsNetInitHelper lanarts_net_init_helper;
//
//		NetUpdatedState serverstate, client1state, client2state;
//		init_connections(serverstate, client1state, client2state);
//
//		for (int frame = 0; frame < 3; frame++) {
//			GameAction serv_action(0, GameAction::action_t(0), frame, 0);
//			GameAction client1_action(1, GameAction::action_t(1), frame, 1);
//			GameAction client2_action(2, GameAction::action_t(2), frame, 2);
//
//			send_action(serv_action, frame, 0, serverstate);
//			send_action(client1_action, frame, 1, client1state);
//			send_action(client2_action, frame, 2, client2state);
//
//			/* We expect an action from each player */
//			poll_n(serverstate.conn, 2);
//			poll_n(client1state.conn, 2);
//			poll_n(client2state.conn, 2);
//
//			test_net_action_assert(serverstate, serv_action, client1_action,
//					client2_action, frame);
//			test_net_action_assert(client1state, serv_action, client1_action,
//					client2_action, frame);
//			test_net_action_assert(client2state, serv_action, client1_action,
//					client2_action, frame);
//		}
//	}
//
//	TEST(test_net_action_sending2) {
//		LanartsNetInitHelper lanarts_net_init_helper;
//
//		NetUpdatedState serverstate, client1state, client2state;
//		init_connections(serverstate, client1state, client2state);
//
//		for (int frame = 0; frame < 3; frame++) {
//			GameAction serv_action(0, GameAction::action_t(0), frame, 0);
//			GameAction client1_action(1, GameAction::action_t(1), frame, 1);
//			GameAction client2_action(2, GameAction::action_t(2), frame, 2);
//
//			send_action(serv_action, frame, 0, serverstate);
//			send_action(client1_action, frame, 1, client1state);
//			send_action(client2_action, frame, 2, client2state);
//		}
//		/* We expect an action from each player, 3 times, so 6 */
//		poll_n(serverstate.conn, 6);
//		poll_n(client1state.conn, 6);
//		poll_n(client2state.conn, 6);
//
//		for (int frame = 0; frame < 3; frame++) {
//			GameAction serv_action(0, GameAction::action_t(0), frame, 0);
//			GameAction client1_action(1, GameAction::action_t(1), frame, 1);
//			GameAction client2_action(2, GameAction::action_t(2), frame, 2);
//
//			test_net_action_assert(serverstate, serv_action, client1_action,
//					client2_action, frame);
//			test_net_action_assert(client1state, serv_action, client1_action,
//					client2_action, frame);
//			test_net_action_assert(client2state, serv_action, client1_action,
//					client2_action, frame);
//		}
//	}

}
