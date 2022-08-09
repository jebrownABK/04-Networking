//following along http://enet.bespin.org/Tutorial.html
#include <enet/enet.h>
#include <iostream>
#include "Server.h"
#include "Client.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib") //including library files using the pragma comment
#pragma comment(lib, "Winmm.lib")

ENetAddress address;
ENetHost* server = nullptr;
ENetHost* client = nullptr;

int main(int argc, char** argv)
{
	Server my_server(address, server);
	Client my_client(address, client);

	cout << "1) Create a server " << endl;
	cout << "2) Create a client " << endl;

	int user_input;
	cin >> user_input;

	switch (user_input)
	{
	case 1:
		my_server.RunServer();
		break;
	case 2:
		my_client.RunClient();
		break;
	default:
		cout << "invalid input." << endl;
		break;
	}

}

