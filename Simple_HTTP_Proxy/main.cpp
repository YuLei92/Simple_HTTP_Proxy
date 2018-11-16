#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <stdarg.h>

int main(int argc, char *argv[]) {
	int local_port;
	pid_t pid;

	bind_addr = NULL;
	int server_sock, client_sock, remote_sock, remote_port = 0;
	int connections_processed = 0;
	char *bind_addr, *remote_host, *cmd_in, *cmd_out;

	if (argc < 3) {
		printf("Syntax:  [local_port][remote_host] -p [remote_port] \n", argv[0]);
		return local_port;
	}
	/* Create server socket */
	int create_socket(int port) {
		int server_sock, optval = 1;
		struct sockaddr_in server_addr;

		if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			return SERVER_SOCKET_ERROR;
		}

		if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
			return SERVER_SETSOCKOPT_ERROR;
		}


	}
