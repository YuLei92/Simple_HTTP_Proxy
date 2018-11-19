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
#include <unistd.h>

using namespace std;
string findFileName(string);

int main(int argc, char* argv[])
{
	int s;
	char recvbuffer[1048576];
	FILE *fd;
	if (argc != 4)
	{
		printf("Usage: client <proxy_server_ip> <proxy_server_port> <URL to retrieve>\n");
		exit(1);
	}

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Client socket creation error\n");
	}

	struct sockaddr_in serverAddrInfo;
	memset((char*)& serverAddrInfo, '0', sizeof(struct sockaddr_in));
	serverAddrInfo.sin_family = AF_INET;
	serverAddrInfo.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &serverAddrInfo.sin_addr);

	if (connect(s, (struct sockaddr*)&serverAddrInfo, sizeof(serverAddrInfo)) == -1)
	{
		printf("Connection to server error\n");
		exit(1);
	}


	printf("Connected successfully\n");
	string url(argv[3]);

	string message = "GET " + url + " HTTP/1.0\r\n\r\n";
	cout << "Message sent: " << endl << message;
	send(s, message.c_str(), message.length(), 0);
	int bytes = 0;
	bytes = recv(s, recvbuffer, 1048576, 0);
	cout << recvbuffer << endl;

	string filename_s = findFileName(url);
	char* filename = new char[filename_s.length() + 1];
	strcpy(filename, filename_s.c_str());
	printf("\n The file name is: %s\n", filename);
	fd = fopen(filename, "w");
	while (bytes > 0) {
		fwrite(recvbuffer, 1, bytes - 4, fd); // This is the command to write in file.
		bytes = recv(s, recvbuffer, 1048576, 0);
	}
	printf("closed\n");
	fclose(fd);
	close(s);
	return 0;
}

string findFileName(string url) {
	int sign_index, sign_index_2;
	string result;
	sign_index = url.find("/");
	if ((url.at(sign_index + 1)) == '/') {
		sign_index_2 = url.substr(sign_index + 2, url.length() - 1).find('/');
		result = url.substr(sign_index + sign_index_2 + 3, url.length() - 1);
	}
	else {
		result = url.substr(sign_index + 1, url.length() - 1);
	}
	return result;
}
