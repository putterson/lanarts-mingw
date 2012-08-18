/* GameNetConnection.h:
 * 	Utility class for managing communication with other players
 * 	If no network communication is used, the methods of this class
 * 	return trivial results.
 * 	Provides useful behaviour on top of the src_net library.
 */

#ifndef GAMENETCONNECTION_H_
#define GAMENETCONNECTION_H_

#include <vector>

#include "../lanarts_defines.h"
#include "../gamestate/ActionQueue.h"

class GameState;
class GameChat;
class GameStateInitData;
class SerializeBuffer;
class NetConnection;
class PlayerData;
struct ChatMessage;

class GameNetConnection {
public:
	/*Message types*/
	enum message_t {
		PACKET_CLIENT2SERV_CONNECTION_AFFIRM = 0,
		PACKET_SERV2CLIENT_INITIALPLAYERDATA = 1,
		PACKET_ACTION = 2,
		PACKET_CHAT_MESSAGE = 3
	};
	// Initialize with references to structures that are updated by messages
	// Keep parts of the game-state that are updated explicit
	GameNetConnection(GameChat& chat, PlayerData& pd,
			GameStateInitData& init_data);
	~GameNetConnection();

	void initialize_as_client(const char* host, int port);
	void initialize_as_server(int port);

	bool is_connected() {
		return _connection != NULL;
	}

	NetConnection* connection() {
		return _connection;
	}

	SerializeBuffer& grab_buffer(message_t type);

	void set_accepting_connections(bool accept);

	void poll_messages(int timeout = 0);

	void send_packet(SerializeBuffer& serializer, int receiver = -1);
	bool check_integrity(GameState* gs, int value);

	//Do-not-call-directly:
	void _handle_message(int sender, const char* msg, size_t len);

private:
	//Keep back-references so that we can alter world state based on messages received
	GameChat& chat;
	PlayerData& pd;
	GameStateInitData& init_data;

	SerializeBuffer* _message_buffer;
	NetConnection* _connection;
};

void net_send_connection_affirm(GameNetConnection& net, const std::string& name,
		class_id classtype);
void net_send_game_init_data(GameNetConnection& net, PlayerData& pd, int seed);
void net_send_player_actions(GameNetConnection& net, int frame,
		int player_number, const ActionQueue& actions);
void net_send_chatmessage(GameNetConnection& net, ChatMessage& message);

#endif /* GAMENETCONNECTION_H_ */
