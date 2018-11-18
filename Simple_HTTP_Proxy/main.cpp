#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <vector>
#include <time.h>

using namespace std;

int main(int argc, char* argv[])
{
	int sd;
	char recvbuffer[1048576];

	if (argc != 4)
	{
		printf("Usage: client <proxy_server_ip> <proxy_server_port> <URL to retrieve>\n");
		exit(1);
	}

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Client socket creation error\n");
	}

	struct sockaddr_in serverAddrInfo;
	memset((char*)& serverAddrInfo, '0', sizeof(struct sockaddr_in));
	serverAddrInfo.sin_family = AF_INET;
	serverAddrInfo.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &serverAddrInfo.sin_addr);

	if (connect(sd, (struct sockaddr*)&serverAddrInfo, sizeof(serverAddrInfo)) == -1)
	{
		printf("Connection to server error\n");
		exit(1);
	}

	printf("Connected successfully\n");
	string url(argv[3]);
	string message = "GET " + url + " HTTP/1.0\r\n\r\n";
	cout << "Message sent: " << endl << message;
	send(sd, message.c_str(), message.length(), 0);
	int bytes = 0;
	bytes = recv(sd, recvbuffer, 1048576, 0);
	cout << recvbuffer << endl;
	
	return 0;
}
