#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
const int BUFFER_SIZE = 4096;
const int SERVER_PORT = 9091;
 
int main()
{
	int client_socket;
	const char *server_ip = "127.0.0.1";
	char buffSend[BUFFER_SIZE];
 
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	assert(client_socket != -1);
 
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
 
	assert(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != -1);
	
	while(1)
	{
		fgets(buffSend, BUFFER_SIZE, stdin);
		assert(send(client_socket, buffSend, strlen(buffSend), 0) != -1);
	}
	close(client_socket);
 
	return 0;
}