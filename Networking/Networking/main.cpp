//following along http://enet.bespin.org/Tutorial.html
#include <enet/enet.h>
#include <iostream>
#include "Server.h"
#include "Client.h"
#include <string>
#include <thread>
#include <chrono>
//message.front()->sending string with packet
//listen for events
//in while loop, sendUserMessage
using namespace std;

#pragma comment(lib, "Ws2_32.lib") //including library files using the pragma comment
#pragma comment(lib, "Winmm.lib")

/* global variable declarations */
ENetAddress address;
ENetHost* server = nullptr;
ENetHost* client = nullptr;

bool exit_chat = false;
bool client_connected = false;


/* function declarations */
void ProcessPacket(ENetPeer* peer, string message, ENetHost* host);
void SendChat(string user_name, ENetHost* host);
void ListenForEvents(ENetHost* host, string user_name);
bool CreateServer();
bool CreateClient();
void ConnectToServer();

void SendChat(string user_name, ENetHost* host)
{
	string user_message = "";
	getline(cin, user_message);

	string full_message = user_name + ": " + user_message;
	ProcessPacket(&(host->peers[0]), full_message, host);


}

void ListenForEvents(ENetHost* host, string user_name)
{
	while (!exit_chat)
	{
		ENetEvent event;
		while (enet_host_service(host, &event, 1000) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT: //client has connected
				
				event.peer->data = (void*)("Host information");
				cout << endl;
				cout << event.peer->data << " has connected to the chat!" << endl;
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				cout << event.packet->data << endl;
				/* Clean up the packet now that we're done using it. */
				enet_packet_destroy(event.packet);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				cout << (char*)event.peer->data << "disconnected from chat." << endl;
				/* Reset the peer's client information. */
				event.peer->data = NULL;
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
	//Server my_server(address, server);
	//Client my_client(address, client);

	cout << "1) Create a server " << endl;
	cout << "2) Create a client " << endl;

	int user_input;
	cin >> user_input;

	if (user_input == 1)
	{
		//my_server.RunServer();
		if (!CreateServer())
		{
			fprintf(stderr,
				"An error occurred while trying to create an ENet server host.\n");
			exit(EXIT_FAILURE);
		}

		cout << "Please enter your name: ";
		string host_name = "";
		cin >> host_name;

		cout << endl;
		cout << "Waiting for client connection..." << endl;

		cin.ignore();
		thread event_listener = thread(ListenForEvents, server, host_name);
		while (!exit_chat)
		{
			SendChat(host_name, server);
		}

	}

	else if (user_input == 2)
	{
		if (!CreateClient())
		{
			fprintf(stderr,
				"An error occurred while trying to create an ENet client host.\n");
			exit(EXIT_FAILURE);
		}

		ConnectToServer();

		cout << "Please enter your name: ";
		string client_name = "";
		cin >> client_name;

		cout << endl;
		cin.ignore();

		thread event_listener = thread(ListenForEvents, client, client_name);

		while (!exit_chat)
		{
			SendChat(client_name, client);
		}
	}
	else {
		cout << "invalid input." << endl;
	}

	if (server != nullptr)
	{
		enet_host_destroy(server);
	}

	if (client != nullptr)
	{
		enet_host_destroy(client);
	}

	return EXIT_SUCCESS;

}

void ProcessPacket(ENetPeer* peer, string message, ENetHost* host)
{
	/* Create a reliable packet of size 7 containing "packet\0" */
	ENetPacket* packet = enet_packet_create(message.c_str(),
		strlen(message.c_str()) + 1,
		ENET_PACKET_FLAG_RELIABLE);
	/* Extend the packet so and append the string "foo", so it now */
	/* contains "packetfoo\0"                                      */
	//enet_packet_resize(packet, strlen("packetfoo") + 1);
	//strcpy(&packet->data[strlen("packet")], "foo");
	/* Send the packet to the peer over channel id 0. */
	/* One could also broadcast the packet by         */
	enet_host_broadcast(host, 0, packet);
	//enet_peer_send(event.peer, 0, packet);

	/* One could just use enet_host_service() instead. */
	//enet_host_service();
	enet_host_flush(host);
}

void ConnectToServer()
{
	ENetAddress address;
	ENetEvent event;
	ENetPeer* peer;
	/* Connect to some.server.net:1234. */
	enet_address_set_host(&address, "127.0.0.1");
	address.port = 1234;
	/* Initiate the connection, allocating the two channels 0 and 1. */
	peer = enet_host_connect(client, &address, 2, 0);
	if (peer == NULL)
	{
		fprintf(stderr,
			"No available peers for initiating an ENet connection.\n");
		exit(EXIT_FAILURE);
	}
	/* Wait up to 5 seconds for the connection attempt to succeed. */
	if (enet_host_service(client, &event, 5000) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT)
	{
		cout << "Connection to 127.0.0.1:1234 succeeded." << endl;
	}
	else
	{
		/* Either the 5 seconds are up or a disconnect event was */
		/* received. Reset the peer in the event the 5 seconds   */
		/* had run out without any significant event.            */
		enet_peer_reset(peer);
		cout << "Connection to 127.0.0.1:1234 failed." << endl;
	}
}

bool CreateServer()
{
	/* Bind the server to the default localhost.     */
	/* A specific host address can be specified by   */
	/* enet_address_set_host (& address, "x.x.x.x"); */
	address.host = ENET_HOST_ANY;
	/* Bind the server to port 1234. */
	address.port = 1234;
	server = enet_host_create(&address /* the address to bind the server host to */,
		32      /* allow up to 32 clients and/or outgoing connections */,
		2      /* allow up to 2 channels to be used, 0 and 1 */,
		0      /* assume any amount of incoming bandwidth */,
		0      /* assume any amount of outgoing bandwidth */);

	return server != nullptr;
}

bool CreateClient()
{
	client = enet_host_create(NULL /* create a client host */,
		1 /* only allow 1 outgoing connection */,
		2 /* allow up 2 channels to be used, 0 and 1 */,
		0 /* assume any amount of incoming bandwidth */,
		0 /* assume any amount of outgoing bandwidth */);

	return client != nullptr;
}

