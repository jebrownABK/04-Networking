/*#pragma once
#include <enet/enet.h>
#include <iostream>

using namespace std;

#pragma comment(lib, "Ws2_32.lib") //including library files using the pragma comment
#pragma comment(lib, "Winmm.lib")

class Client
{
	ENetAddress m_address; //simple struct to keep track of host IP address and port
	ENetHost* m_client = nullptr;
public:
	Client(ENetAddress address, ENetHost* server);
	~Client();
	int RunClient();
private:
	bool CreateClient();
};*/