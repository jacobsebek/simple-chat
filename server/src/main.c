#include <SDL_net.h>

#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "server.h"

// This struct defines a connected client, one open socket
// The only cached info needed is the nick
static struct client {
    char nick[SERV_MAX_NICK_LEN];
    TCPsocket socket;
} clients[SERV_MAX_CLIENTS];

// The set of all connected clients, this allows simple non-blocking IO
// Without unnecessary multithreading 
// (altought that would be needed for large scale applications obviously, or perhaps a combination)
static SDLNet_SocketSet socks;
// The listening socket, also contained in socks, listens for incomming connections
static TCPsocket server_socket;

// Broadcasts a message sent by client to all other clients
// This is used internally and with care because it doesn't do any sort of checks
// e.g. validity of the message
static int broadcast_message(struct client* client, const char* msg) {

    // Args: nick, message
    struct prot_msg msg_pack = prot_make_msg("MSG", 2, client->nick, msg);

    // Send this message to everyone (except the client that sent it)
	for (size_t i = 0; i < SERV_MAX_CLIENTS; i++) {
		if (clients[i].socket == NULL || clients[i].socket == client->socket) continue;

        // Not really necessary to error check
        // If one of the clients disconnect, we will disconnect them anyway in the main loop asap
        prot_send(clients[i].socket, msg_pack);
	}

    return 0;
}

// Disconnects a client, this includes closing the socket, removing it from the
// socket set and letting everyone know, this will appear as the client sending
// the message "Disconnected"
static void disconnect_client(struct client* client) {

    fprintf(stdout, "Client %s disconnected.\n", client->nick);
    broadcast_message(client, "Disconnected");

    SDLNet_TCP_DelSocket(socks, client->socket);
    SDLNet_TCP_Close(client->socket);
    client->socket = NULL;

}

// Handle any sort of incoming data from a client
// Blocking, however it is used with the polling mechanism of the socket set
// for it not to be..
static int handle_message(struct client* client) {

    // Receive the data, protlib packages it up nicely
    struct prot_msg msg = prot_recv(client->socket);
    if (msg.status < 0) {
        disconnect_client(client); 
        return -1;
    }

    int ret = 0;
    // Handle the message based on the head
    if (!strncmp(msg.head, "MSG", PROT_HEAD_SIZE)) {

        // Checks for the correct number of arguments and the argument length
        if (msg.status != 1) {
            ret = -1;
            goto err;
        }

        if (strlen(msg.args[0])+1 > SERV_MAX_MSG_LEN) {
            ret = -1;
            goto err;
        }
        
        fprintf(stdout, "<%s> : %s\n", client->nick, msg.args[0]);
        broadcast_message(client, msg.args[0]);
        
    } else
    if (!strncmp(msg.head, "NIC", PROT_HEAD_SIZE)) {

        if (msg.status != 1) {
            ret = -1;
            goto err;
        }

        if (strlen(msg.args[0])+1 > SERV_MAX_NICK_LEN) {
            ret = -1;
            goto err;
        }

        fprintf(stdout, "The client %s changed his nickname to %s\n", client->nick, msg.args[0]);          

        // Let others know too
        char buf[SERV_MAX_MSG_LEN]; // Be safe!
        snprintf(buf, sizeof(buf), "Changed nickname to <%s>", msg.args[0]);
        broadcast_message(client, buf);

        // Update the nick
        strcpy(client->nick, msg.args[0]);

        // Send a confirmation back to the client
        // This message exists in order to potentially filter nicknames, 
        // bad characters, and also for sending the initial nick at the beginning
        prot_send(client->socket, prot_make_msg("NIC", 1, client->nick));
    }

    err:

    if (ret != 0)
        disconnect_client(client);

    // Free the dynamically allocated arguments
    for (size_t i = 0; i < (size_t)msg.status; i++) 
        free(msg.args[i]);

    return ret;
}

// Handles a new incomming connection
static int handle_connection() {
    fprintf(stdout, "Handling connection\n");

    TCPsocket connection = SDLNet_TCP_Accept(server_socket);
    if (!connection) {
        fprintf(stderr, "Failed to accept incomming connection\n"); 
        return -1;
    }

    // The clients are stored in sort of a clumsy way but it's sufficient
    // Find the first empty client slot
    int i = 0;
    for (; i < SERV_MAX_CLIENTS; i++)
        if (clients[i].socket == NULL) {
            break;
        }

    // Either send an ACCapted or a REFused message
    if (i == SERV_MAX_CLIENTS) {
        fprintf(stderr, "Cannot accept client, max number of clients reached\n");
        prot_send(connection, prot_make_msg("REF", 0));
        return -1;
    } else 
        prot_send(connection, prot_make_msg("ACC", 0));


    // Send a request to the client to change his local nickname
    snprintf(clients[i].nick, sizeof(clients[i].nick), "Anonymous");
    if (prot_send(connection, prot_make_msg("NIC", 1, clients[i].nick)) < 0) {
        fprintf(stdout, "Incoming connection lost\n");
        return -1;
    }    

    // Register the client
    clients[i].socket = connection;
    SDLNet_TCP_AddSocket(socks, connection);

    fprintf(stdout, "Client %s connected.\n", clients[i].nick);
    broadcast_message(&clients[i], "Connected");

    return 0;
}

int main(int argc, char *argv[]) {

    (void)argc; (void)argv;
	
	// Initialize SDL
	if(SDL_Init(0) < 0) {
		printf("SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}

	// Initialize SDL_net
	if(SDLNet_Init() < 0) {
		printf("SDLNet_Init: %s\n", SDLNet_GetError());
		exit(1);
	}

    // Initialise the socket set
    // +1 for the server socket
    socks = SDLNet_AllocSocketSet(SERV_MAX_CLIENTS + 1);
    if (!socks) {
        fprintf(stderr, "SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
        exit(1);
    }

    // Resolve the port for opening the listening socket
	IPaddress addr;
	if (SDLNet_ResolveHost(&addr, NULL, SERV_PORT) < 0) {
		printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
		exit(1);
	}	
	
	// Open the actual server socket
	server_socket = SDLNet_TCP_Open(&addr);
	if (!server_socket) {
		printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
		exit(1);
	}

    // Add the server socket to the set
    SDLNet_TCP_AddSocket(socks, server_socket);

    while (1) {
        
        // If there is socket activity...
        // -1 = wait for as long as possible, in this case about 49 days
        // I can do this because there is literally no other work to do
        if (SDLNet_CheckSockets(socks, -1) > 0) {

            // ..Is it an incomming connection?
            if (SDLNet_SocketReady(server_socket))
                handle_connection();
            else {
                // else it is a message from one of the clients
                for (size_t i = 0; i < SERV_MAX_CLIENTS; i++) {
                    if (clients[i].socket == NULL) continue;

                    if (SDLNet_SocketReady(clients[i].socket))
                        handle_message(&clients[i]);
                }

            }

        }

    }    

    // Note that this code is currently unreachable, although it's nice to have it here

    // Close all the client sockets
    for (size_t i = 0; i < SERV_MAX_CLIENTS; i++)
        SDLNet_TCP_Close(clients[i].socket);

    // Close the server socket
    SDLNet_TCP_Close(server_socket);

    SDLNet_FreeSocketSet(socks);
    SDLNet_Quit();
    SDL_Quit();
}
