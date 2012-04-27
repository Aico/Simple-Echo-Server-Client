/**
 * Echo Server.
 *
 * Usage: server <Server Port>
 *
 * Author: Kenneth Chik 2012.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "ErrorHandle.h"

static const int BUFFER_SIZE = 1; //size of receive buffer
static const int BACKLOG = 5; //number of pending connections.

void printBuffer(char * prefix, char * buffer, int length) {
	printf("%s: ",prefix);
	int i;
	for (i=0;i<length;++i) {
		printf("%d ",(int)buffer[i]);
	}
	printf("\n");
}

void echoData(int client_socket) {
    char buffer[BUFFER_SIZE]; // Buffer for echo string

    // Receive message from client
    ssize_t receive_data_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (receive_data_size < 0) {
        systemErrorMsg("First call to recv() failed");
    } else if (receive_data_size > 0) {
    	printBuffer("Received",buffer,receive_data_size);
    }

    // echo data and receive more until end of stream
    while (receive_data_size > 0) { // 0 indicates end of stream
        ssize_t sent_data_size = send(client_socket, buffer, receive_data_size, 0);
        if (sent_data_size < 0) {
            //systemErrorMsg("send() failed");
        } else if (sent_data_size != receive_data_size) {
            userErrorExit("echo data size is not equal to received data size.");
        }

        if (sent_data_size > 0) {
        	printBuffer("Sent",buffer,sent_data_size);
        }

        receive_data_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (receive_data_size < 0) {
            systemErrorMsg("recv() failed");
        } else if (receive_data_size > 0) {
        	printBuffer("Received",buffer,receive_data_size);
        }
    }

    close(client_socket);
}

int main(int argc, char *argv[]) {

    //Check for correct usage.
    if (argc != 2 || (argc==2 && atoi(argv[1])==0)) {
        userErrorExit("Usage: server <Server Port>");
    }

    //Prevent server from receiving SIGPIPE when write to socket that is closed.
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        systemErrorExit("signal");
    }
  
    in_port_t server_port = atoi(argv[1]); // First argument:  local server port

    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket < 0) {
        systemErrorExit("Failed to create socket.");
    }

    struct sockaddr_in server_address;                  // Server address structure used in bind function.
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;                // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Any interface
    server_address.sin_port = htons(server_port);          // Local port to bind to.

    // Bind to the local address
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
        systemErrorExit("Failed to bind to socket");
    }

    // Listen on socket.
    if (listen(server_socket, BACKLOG) < 0) {
        systemErrorExit("Failed to listen on socket");
    }

    for (;;) {
        struct sockaddr_in client_address; // Client address
    	socklen_t client_address_length = sizeof(client_address);
    	// Wait for client connect
        int client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_address_length);
        if (client_socket < 0) {
            systemErrorExit("accept connection failed: most often caused by a bad socket.");
        }
    
        char client_name[INET_ADDRSTRLEN]; // Stores converted client address name.
        if (inet_ntop(AF_INET, &client_address.sin_addr.s_addr, client_name,sizeof(client_name)) != NULL) {
            printf("Handling client %s/%d\n", client_name, ntohs(client_address.sin_port));
        } else {
            userErrorMsg("Unable to get client address");
        }

        echoData(client_socket); //echo the data back to client.
    }
    return 1; //not reached.
}
