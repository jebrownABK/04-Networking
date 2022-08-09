#pragma once
#include <enet/enet.h>
#include <iostream>

using namespace std;

#pragma comment(lib, "Ws2_32.lib") //including library files using the pragma comment
#pragma comment(lib, "Winmm.lib")

class Server
{
	ENetAddress m_address; //simple struct to keep track of host IP address and port
	ENetHost* m_server = nullptr;
public:
	Server(ENetAddress address, ENetHost* server);
	~Server();
	int RunServer();
private:
	bool CreateServer();
};