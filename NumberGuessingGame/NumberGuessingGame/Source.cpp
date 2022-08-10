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
bool end_game = false;

enum PacketHeaderTypes
{
	PHT_Invalid = 0,
	PHT_IsDead,
	PHT_Guess,
	PHT_Count,
	PHT_GameFinished,
	PHT_Play,
	PHT_Wait
};

struct GamePacket
{
	GamePacket() {}
	PacketHeaderTypes Type = PHT_Invalid;
};

struct IsDeadPacket : public GamePacket
{
	IsDeadPacket()
	{
		Type = PHT_IsDead;
	}

	int playerId = 0;
	bool IsDead = false;
};

struct GuessPacket : public GamePacket
{
	GuessPacket()
	{
		Type = PHT_Guess;
	}

	int player_guess = 0;
};

struct PlayGamePacket : public GamePacket
{
	PlayGamePacket()
	{
		Type = PHT_Play;
	}
};

struct WaitingRoomPacket : public GamePacket
{
	WaitingRoomPacket()
	{
		Type = PHT_Wait;
	}
};

struct EndGamePacket : public GamePacket
{
	EndGamePacket()
	{
		Type = PHT_GameFinished;
	}
	int playerId = 0;
	string end_game_message = "The winner is " + playerId;
};
//can pass in a peer connection if wanting to limit
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
		1 /* only allow 1 outgoing connection */,
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

		if (RecGamePacket->Type == PHT_IsDead)
		{
			cout << "u dead?" << endl;
			IsDeadPacket* DeadGamePacket = (IsDeadPacket*)(event.packet->data);
			if (DeadGamePacket)
			{
				string response = (DeadGamePacket->IsDead ? "yeah" : "no");
				cout << response << endl;
			}
		}
		else if (RecGamePacket->Type == PHT_Guess)
		{
			cout << "Player blahblah guessed " << RecGamePacket << endl;
			GuessPacket* PlayerGuess = (GuessPacket*)(event.packet->data);
			if (PlayerGuess)
			{
				if (PlayerGuess->player_guess == correct_answer)
				{
					cout << "Correct!" << endl;
					end_game = true;
				}
				
			}
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

void BroadcastCanEnterGame()
{
	if (client_connections == 2)
	{ //play
		PlayGamePacket* PlayPacket = new PlayGamePacket();
		string message = "Let's play Guess the Number!\n Guess the number:";
		ENetPacket* packet = enet_packet_create(message.c_str(),
			sizeof(message.c_str()),
			ENET_PACKET_FLAG_RELIABLE);

		enet_host_broadcast(NetHost, 0, packet);
		enet_host_flush(NetHost);
	}
	else
	{ //wait
		WaitingRoomPacket* WaitPacket = new WaitingRoomPacket();
		string message = "Waiting for more players...";
		ENetPacket* packet = enet_packet_create(message.c_str(),
			sizeof(message.c_str()),
			ENET_PACKET_FLAG_RELIABLE);

		enet_host_broadcast(NetHost, 0, packet);
		enet_host_flush(NetHost);
	}
}

//TODO: add player name/id into message
void BroadcastEndGame()
{
	
	EndGamePacket* EndPacket = new EndGamePacket();
	EndPacket->playerId = 0; //this should be the winner's id
	string end_game_message = "The winner is " + EndPacket->playerId;
	ENetPacket* packet = enet_packet_create(end_game_message.c_str(),
		sizeof(end_game_message.c_str()),
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
				BroadcastCanEnterGame();
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				HandleReceivePacket(event);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				cout << (char*)event.peer->data << "disconnected." << endl;
				/* Reset the peer's client information. */
				event.peer->data = NULL;
				//notify remaining player that the game is done due to player leaving
				BroadcastEndGame();
			}
		}
	}
}

void ClientProcessPackets()
{
	while (1)
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
	//server setup
	//server should create a random number, ask the client to try and guess
	//when it received a respnse, tell the player if they are correct or not
	//shut down after guess correctly (notify client that the game is over)
	// game should not start until two clients have connected
	// 
	//client will send the guess over to the server
	//client shouldn't be able to guess until the server responds "correct" or "incorrect"

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
		cout << "Please enter number of players: ";
		int players = 0;
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

		PacketThread = new thread(ClientProcessPackets);

		//handle possible connection failure
		{
			//enet_peer_reset(Peer);
			//cout << "Connection to 127.0.0.1:1234 failed." << endl;
		}
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