#include <enet/enet.h>
#include <string>
#include <iostream>
#include <thread>

using namespace std;

// global variables
ENetHost* NetHost = nullptr;
ENetPeer* Peer = nullptr;
thread* PacketThread = nullptr;

int correct_answer;
int players = 0;
bool end_game = false;

// function definitions
bool CreateServer(int num_players);
bool CreateClient();
bool AttemptConnectToServer();

void ServerListenForEvents();

void HandleReceivePacket(const ENetEvent& event);
void ClientListenForEvents();

void BraodcastWaitingMessage();
void BraodcastPlayGameMessage();
void BroadcastGuessNumber();
void BroadcastEndGame(void* player_id);
void BroadcastQuitGame();

enum PacketHeaderTypes
{
	PHT_Invalid = 0,
	PHT_Wait,
	PHT_Play,
	PHT_Guess,
	PHT_GameFinished,
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
	string message = "";
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

	string message;
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
		if (RecGamePacket->Type == PHT_Wait)
		{
			WaitingRoomPacket* Waiting = (WaitingRoomPacket*)(event.packet->data);
			cout << Waiting->message;
		}
		else if (RecGamePacket->Type == PHT_Play)
		{
			PlayGamePacket* PlayGame = (PlayGamePacket*)(event.packet->data);
			cout << PlayGame->message << endl;

			BroadcastGuessNumber();
		}
		else if (RecGamePacket->Type == PHT_Guess)
		{

			GuessPacket* PlayerGuess = (GuessPacket*)(event.packet->data);
			cout << PlayerGuess->message;
			cin >> PlayerGuess->player_guess;

			if (PlayerGuess->player_guess == correct_answer)
			{
				BroadcastEndGame(event.peer->data);
			}
			else
			{
				BroadcastGuessNumber();
			}

		}
		else if (RecGamePacket->Type == PHT_GameFinished)
		{
			EndGamePacket* EndGame = (EndGamePacket*)(event.packet->data);
			cout << EndGame->end_game_message;
			end_game = true;
		}
		else if (RecGamePacket->Type == PHT_Quit)
		{
			QuitPacket* QuitGame = (QuitPacket*)(event.packet->data);
			cout << QuitGame->message;
			end_game = true;
		}
		
	}
	else
	{
		cout << "Invalid Packet " << endl;
	}

	enet_packet_destroy(event.packet);
	{
		enet_host_flush(NetHost);
	}
}

void BraodcastWaitingMessage()
{

	WaitingRoomPacket* WaitingPacket = new WaitingRoomPacket();
	WaitingPacket->message = "Waiting for more players...";

	ENetPacket* packet = enet_packet_create(WaitingPacket,
		sizeof(*WaitingPacket),
		ENET_PACKET_FLAG_RELIABLE);

	enet_host_broadcast(NetHost, 0, packet);

	enet_host_flush(NetHost);
	delete WaitingPacket;
}

void BraodcastPlayGameMessage()
{
	cout << "Starting Guessing Game...";

	PlayGamePacket* ReadyPacket = new PlayGamePacket();
	ReadyPacket->message = "Let's play Guess the Number!";

	ENetPacket* packet = enet_packet_create(ReadyPacket,
		sizeof(*ReadyPacket),
		ENET_PACKET_FLAG_RELIABLE);

	enet_host_broadcast(NetHost, 0, packet);

	enet_host_flush(NetHost);
	delete ReadyPacket;
}

void BroadcastGuessNumber()
{
	GuessPacket* NumberGuessPacket = new GuessPacket();
	NumberGuessPacket->message = "Guess the number: ";

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

void BroadcastEndGame(void* player_id)
{
	EndGamePacket* EndPacket = new EndGamePacket();
	EndPacket->playerId = player_id;
	EndPacket->end_game_message = "The winner is " + (char)EndPacket->playerId;
	ENetPacket* packet = enet_packet_create(EndPacket,
		sizeof(*EndPacket),
		ENET_PACKET_FLAG_RELIABLE);

	enet_host_broadcast(NetHost, 0, packet);

	enet_host_flush(NetHost);
	delete EndPacket;

	end_game = true;
}

void BroadcastQuitGame()
{
	QuitPacket* Quitter = new QuitPacket();
	Quitter->message = "Sorry! Player disconnected from server. Reconnect to play again.";
	ENetPacket* packet = enet_packet_create(Quitter,
		sizeof(*Quitter), //sizeof(*struct) w/string inside of struct
		ENET_PACKET_FLAG_RELIABLE);

	/* One could also broadcast the packet by         */
	enet_host_broadcast(NetHost, 0, packet);
	//enet_peer_send(event.peer, 0, packet);

	/* One could just use enet_host_service() instead. */
	//enet_host_service();
	enet_host_flush(NetHost);
	delete Quitter;
}

void ServerListenForEvents()
{
	int client_connections = 0;
	correct_answer = 1 + (rand() % 10);
	while (!end_game) //listening for client events until the game is over
	{
		ENetEvent event;
		while (enet_host_service(NetHost, &event, 1000) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				client_connections++; //number of clients that have connected

				event.peer->data = (void*)("Client information");
				cout << client_connections << "A new player has connected. Client connections: " << client_connections << endl;

				if (client_connections == players) //ready to play
				{
					BraodcastPlayGameMessage();
				}
				else if (client_connections < players) //braodcast waiting message
				{
					BraodcastWaitingMessage();
				}
				else //too many connections, don't add new player to game
				{

				}

				break;
			case ENET_EVENT_TYPE_RECEIVE:
				HandleReceivePacket(event);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				cout << (char*)event.peer->data << "disconnected." << endl;
				event.peer->data = NULL;
				BroadcastQuitGame();
			}
		}
	}
}

void ClientListenForEvents()
{
	while (!end_game) //listening to server until game is over
	{
		ENetEvent event;
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
	if (UserInput == 1) //server creation
	{
		
		//number of players is declared globally, now initialize it by server input
		cout << "Please enter number of players: ";
		cin >> players;

		//create server
		if (!CreateServer(players))
		{
			fprintf(stderr,
				"An error occurred while trying to create an ENet server.\n");
			exit(EXIT_FAILURE);
		}

		//start listening for events
		cout << "waiting for players to join..." << endl;
		PacketThread = new thread(ServerListenForEvents);

	}
	else if (UserInput == 2) //client creation
	{
		//create a client
		if (!CreateClient())
		{
			fprintf(stderr,
				"An error occurred while trying to create an ENet client host.\n");
			exit(EXIT_FAILURE);
		}

		//try to connect to a server
		ENetEvent event;
		if (!AttemptConnectToServer())
		{
			fprintf(stderr,
				"No available peers for initiating an ENet connection.\n");
			exit(EXIT_FAILURE);
		}

		//start listening for events
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