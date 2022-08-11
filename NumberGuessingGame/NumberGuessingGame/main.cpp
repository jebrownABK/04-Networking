

#include <enet/enet.h>
#include <string>
#include <iostream>
#include <thread>

using namespace std;

ENetHost* NetHost = nullptr;
ENetPeer* Peer = nullptr;
bool IsServer = false;
thread* PacketThread = nullptr;

int correct_answer;
int client_connections = 0;
int players = 0;
bool end_game = false;
void BroadcastEndGame(void* player_id);
void BroadcastGuessNumber(int guess, void* player_id);

enum PacketHeaderTypes
{
	PHT_Invalid = 0,
	PHT_Guess,
	PHT_Count,
	PHT_GameFinished,
	PHT_Play,
	PHT_Wait,
	PHT_Quit
};

struct GamePacket
{
	GamePacket() {}
	PacketHeaderTypes Type = PHT_Invalid;
};
struct GuessPacket : public GamePacket
{
	GuessPacket()
	{
		Type = PHT_Guess;
	}

	void* player_id;
	int player_guess = 0;
};
struct PlayGamePacket : public GamePacket
{
	PlayGamePacket()
	{
		Type = PHT_Play;
	}
	string message = "";
};
struct WaitingRoomPacket : public GamePacket
{
	WaitingRoomPacket()
	{
		Type = PHT_Wait;
	}
	string message = "";
};
struct EndGamePacket : public GamePacket
{
	EndGamePacket()
	{
		Type = PHT_GameFinished;
	}
	void* playerId;
	string end_game_message = "";
};
struct QuitPacket : public GamePacket
{
	QuitPacket()
	{
		Type = PHT_Quit;
	}


	void* player_id;
};

bool CreateServer(int num_players)
{
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = 1234;
	NetHost = enet_host_create(&address /* the address to bind the server host to */,
		num_players      /* allow up to 32 clients and/or outgoing connections */,
		2      /* allow up to 2 channels to be used, 0 and 1 */,
		0      /* assume any amount of incoming bandwidth */,
		0      /* assume any amount of outgoing bandwidth */);

	return NetHost != nullptr;
}

bool CreateClient()
{
	NetHost = enet_host_create(NULL /* create a client host */,
		2 /* only allow 2 outgoing connection */,
		2 /* allow up 2 channels to be used, 0 and 1 */,
		0 /* assume any amount of incoming bandwidth */,
		0 /* assume any amount of outgoing bandwidth */);

	return NetHost != nullptr;
}

bool AttemptConnectToServer()
{
	ENetAddress address;
	/* Connect to some.server.net:1234. */
	enet_address_set_host(&address, "127.0.0.1");
	address.port = 1234;
	/* Initiate the connection, allocating the two channels 0 and 1. */
	Peer = enet_host_connect(NetHost, &address, 2, 0);
	return Peer != nullptr;
}

void HandleReceivePacket(const ENetEvent& event)
{
	GamePacket* RecGamePacket = (GamePacket*)(event.packet->data);
	if (RecGamePacket)
	{
		cout << "Received Game Packet " << endl;

		if (RecGamePacket->Type == PHT_Guess)
		{
			
			GuessPacket* PlayerGuess = (GuessPacket*)(event.packet->data);
			cout << "Player " << PlayerGuess->player_id << " guessed " << PlayerGuess->player_guess;
			if (PlayerGuess->player_guess == correct_answer)
			{
				BroadcastEndGame(event.peer->data);
			}
			else
			{
				cout << "Wrong!" << endl;
				cout << "Guess the number: ";
				int number_guess;
				cin >> number_guess; //read in the client's guess
				BroadcastGuessNumber(number_guess, event.peer->data); //check guess
			}

		}
		else if (RecGamePacket->Type == PHT_Play)
		{
			PlayGamePacket* PlayGame = (PlayGamePacket*)(event.packet->data); //create packet
			cout << PlayGame->message; //display message to clients
			int number_guess;
			cin >> number_guess; //read in the client's guess
			BroadcastGuessNumber(number_guess, event.peer->data); //check guess
		}
		else if (RecGamePacket->Type == PHT_GameFinished)
		{
			EndGamePacket* EndGame = (EndGamePacket*)(event.packet->data);
			cout << EndGame->end_game_message;
			end_game = true;
		}
		else if (RecGamePacket->Type == PHT_Wait)
		{
			WaitingRoomPacket* Waiting = (WaitingRoomPacket*)(event.packet->data);
			cout << Waiting->message;
		}
	}
	else
	{
		cout << "Invalid Packet " << endl;
	}

	/* Clean up the packet now that we're done using it. */
	enet_packet_destroy(event.packet);
	{
		enet_host_flush(NetHost);
	}
}

void BroadcastCanEnterGame(int current_connections, int number_players)
{
	if (current_connections == number_players)
	{ //play
		cout << "Starting Guessing Game...";

		PlayGamePacket* ReadyPacket = new PlayGamePacket();
		ReadyPacket->message = "Let's play Guess the Number!\n Guess the number:";

		ENetPacket* packet = enet_packet_create(ReadyPacket,
			sizeof(*ReadyPacket),
			ENET_PACKET_FLAG_RELIABLE);

		/* One could also broadcast the packet by         */
		enet_host_broadcast(NetHost, 0, packet);
		//enet_peer_send(event.peer, 0, packet);

		/* One could just use enet_host_service() instead. */
		//enet_host_service();
		enet_host_flush(NetHost);
		delete ReadyPacket;
	}
	else
	{ //wait
		cout << "Waiting for more players...";

		WaitingRoomPacket* WaitingPacket = new WaitingRoomPacket();
		WaitingPacket->message = "Waiting for more players...";

		ENetPacket* packet = enet_packet_create(WaitingPacket,
			sizeof(*WaitingPacket),
			ENET_PACKET_FLAG_RELIABLE);

		/* One could also broadcast the packet by         */
		enet_host_broadcast(NetHost, 0, packet);
		//enet_peer_send(event.peer, 0, packet);

		/* One could just use enet_host_service() instead. */
		//enet_host_service();
		enet_host_flush(NetHost);
		delete WaitingPacket;
	}
}
void BroadcastEndGame(void* player_id)
{
	EndGamePacket* EndPacket = new EndGamePacket();
	EndPacket->playerId = player_id; //this should be the winner's id
	EndPacket->end_game_message = "The winner is " + (char)EndPacket->playerId;
	ENetPacket* packet = enet_packet_create(EndPacket,
		sizeof(*EndPacket),
		ENET_PACKET_FLAG_RELIABLE);

	/* One could also broadcast the packet by         */
	enet_host_broadcast(NetHost, 0, packet);
	//enet_peer_send(event.peer, 0, packet);

	/* One could just use enet_host_service() instead. */
	//enet_host_service();
	enet_host_flush(NetHost);
	delete EndPacket;

	end_game = true;
}
void BroadcastGuessNumber(int guess, void* player_id)
{
	GuessPacket* NumberGuessPacket = new GuessPacket();
	NumberGuessPacket->player_id = player_id;
	NumberGuessPacket->player_guess = guess;

	ENetPacket* packet = enet_packet_create(NumberGuessPacket,
		sizeof(*NumberGuessPacket),
		ENET_PACKET_FLAG_RELIABLE);

	/* One could also broadcast the packet by         */
	enet_host_broadcast(NetHost, 0, packet);
	//enet_peer_send(event.peer, 0, packet);

	/* One could just use enet_host_service() instead. */
	//enet_host_service();
	enet_host_flush(NetHost);
	delete NumberGuessPacket;

	end_game = true;
}
void BroadcastQuitGame()
{
	QuitPacket* Quitter = new QuitPacket();
	string end_game_message = "Sorry! Player disconnected from server.";
	ENetPacket* packet = enet_packet_create(end_game_message.c_str(),
		sizeof(end_game_message.c_str()), //sizeof(*struct) w/string inside of struct
		ENET_PACKET_FLAG_RELIABLE);

	/* One could also broadcast the packet by         */
	enet_host_broadcast(NetHost, 0, packet);
	//enet_peer_send(event.peer, 0, packet);

	/* One could just use enet_host_service() instead. */
	//enet_host_service();
	enet_host_flush(NetHost);
	delete Quitter;

	end_game = true;
}

void ServerProcessPackets()
{
	while (!end_game)
	{
		ENetEvent event;
		while (enet_host_service(NetHost, &event, 1000) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				client_connections++;
				/* Store any relevant client information here. */
				event.peer->data = (void*)("Client information");
				cout << client_connections << " clients connected" << endl;
				BroadcastCanEnterGame(client_connections, players);
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				HandleReceivePacket(event);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				cout << (char*)event.peer->data << "disconnected." << endl;
				/* Reset the peer's client information. */
				event.peer->data = NULL;
				//notify remaining player that the game is done due to player leaving
				BroadcastQuitGame();
			}
		}
	}
}

void ClientListenForEvents()
{
	while (!end_game)
	{
		ENetEvent event;
		/* Wait up to 1000 milliseconds for an event. */
		while (enet_host_service(NetHost, &event, 1000) > 0)
		{
			switch (event.type)
			{
			case  ENET_EVENT_TYPE_CONNECT:
				cout << "Connection succeeded " << endl;
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				HandleReceivePacket(event);
				break;
			}
		}
	}
}

int main(int argc, char** argv)
{
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet.\n");
		cout << "An error occurred while initializing ENet." << endl;
		return EXIT_FAILURE;
	}
	atexit(enet_deinitialize);

	cout << "1) Create Server " << endl;
	cout << "2) Create Client " << endl;
	int UserInput;
	cin >> UserInput;
	if (UserInput == 1)
	{

		correct_answer = 1 + (rand() % 10);
		cout << "Please enter number of players: ";

		cin >> players;

		if (!CreateServer(players))
		{
			fprintf(stderr,
				"An error occurred while trying to create an ENet server.\n");
			exit(EXIT_FAILURE);
		}

		IsServer = true;
		cout << "waiting for players to join..." << endl;
		PacketThread = new thread(ServerProcessPackets);

	}
	else if (UserInput == 2)
	{
		if (!CreateClient())
		{
			fprintf(stderr,
				"An error occurred while trying to create an ENet client host.\n");
			exit(EXIT_FAILURE);
		}

		ENetEvent event;
		if (!AttemptConnectToServer())
		{
			fprintf(stderr,
				"No available peers for initiating an ENet connection.\n");
			exit(EXIT_FAILURE);
		}

		string user_name;
		cout << "Enter your first name: ";
		cin >> user_name;

		PacketThread = new thread(ClientListenForEvents);
	}
	else
	{
		cout << "Invalid Input" << endl;
	}


	if (PacketThread)
	{
		PacketThread->join();
	}
	delete PacketThread;
	if (NetHost != nullptr)
	{
		enet_host_destroy(NetHost);
	}

	return EXIT_SUCCESS;
}