//#include "Server.h"
//
//
//Server::Server(ENetAddress address, ENetHost* server)
//	:m_address(address)
//	,m_server(server)
//{
//
//	m_address.host = ENET_HOST_ANY;
//	m_address.port = 1234;
//
//	m_server = enet_host_create(&m_address, 32, 2, 0, 0);
//}
//
//Server::~Server()
//{
//	if (m_server != nullptr)
//	{
//		enet_host_destroy(m_server);
//	}
//
//}
//
//bool Server::CreateServer()
//{
//	
//	return m_server != nullptr;
//}
//
//int Server::RunServer()
//{
//	/* INITIALIZATION */
//	if (enet_initialize() != 0)
//	{
//		fprintf(stderr, "An error occurrec while initializing ENet. \n");
//		cout << "An error occurred while initializing ENet." << endl;
//		return EXIT_FAILURE; //macro that returns success 0, failure 1
//	}
//	atexit(enet_deinitialize);
//
//	/* CREATING AN ENET SERVER */
//	if (!CreateServer())
//	{
//		fprintf(stderr,
//			"An error occurred while trying to create an ENet server host.\n");
//		exit(EXIT_FAILURE);
//	}
//
//	cout << "Please input a name" << endl;
//	void* host_name;
//	cin >> host_name;
//
//	string user_input = "";
//	while (user_input != "goodbye")
//	{
//		ENetEvent event;
//		cin >> user_input;
//		/* Wait up to 1000 milliseconds for an event. */
//		while (enet_host_service(m_server, &event, 1000) > 0)
//		{
//			switch (event.type)
//			{
//			case ENET_EVENT_TYPE_CONNECT: //client connected to chat
//				cout << event.peer->data << " has connected to chat!" << endl;
//				/* Store any relevant client information here. */
//				event.peer->data = (void*)"Client Name";
//
//				{
//					/* Create a reliable packet of size 7 containing "packet\0" */
//					ENetPacket* packet = enet_packet_create("Hello",
//						strlen("Hello") + 1,
//						ENET_PACKET_FLAG_RELIABLE);
//					/* Extend the packet so and append the string "foo", so it now */
//					/* contains "packetfoo\0"                                      */
//					//enet_packet_resize(packet, strlen("packetfoo") + 1);
//					//strcpy(&packet->data[strlen("packet")], "foo");
//					/* Send the packet to the peer over channel id 0. */
//					/* One could also broadcast the packet by         */
//					//enet_host_broadcast (server, 0, packet);
//					enet_peer_send(event.peer, 0, packet);
//
//					/* One could just use enet_host_service() instead. */
//					enet_host_flush(m_server);
//				}
//
//				break;
//			case ENET_EVENT_TYPE_RECEIVE: //received a message from client
//				cout << event.peer->data << ": "
//					<< event.packet->data
//					<< endl;
//				/* Clean up the packet now that we're done using it. */
//				enet_packet_destroy(event.packet);
//				break;
//
//			case ENET_EVENT_TYPE_DISCONNECT:
//				cout << event.peer->data << " disconnected from chat." << endl;
//				/* Reset the peer's client information. */
//				event.peer->data = NULL;
//			}
//		}
//	}
//	return EXIT_SUCCESS;
//}
//
//
