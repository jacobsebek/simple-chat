// Check client.h for info and guidance about these functions
// Also make sure to edit the "documentation" that is held there

#include <SDL_net.h>
#include <string.h>

#include "client.h"
#include "protocol.h"

// The global TCP socket, either NULL or connected to a server
static TCPsocket socket;
// This socket set contains only our socket, it is used for non-blocking IO (polling)
static SDLNet_SocketSet sset;

// Send a message to the server
int client_send(const struct client_msg* msg) {

    struct prot_msg raw_msg;

    switch (msg->type) {
        case CLIENT_MSG_MSG:

            if (strlen(msg->u.send.msg.text)+1 > SERV_MAX_MSG_LEN)
                return CLIENT_ERR_INVALID_MSG;

            raw_msg = prot_make_msg("MSG", 1, msg->u.send.msg.text);
        break;
        case CLIENT_MSG_NICK:

            if (strlen(msg->u.send.nick.newnick)+1 > SERV_MAX_NICK_LEN)
                return CLIENT_ERR_INVALID_MSG;

            raw_msg = prot_make_msg("NIC", 1, msg->u.send.nick.newnick);
        break;
        default:
            return CLIENT_ERR_INVALID_MSG;
        break;
    }

    int status = prot_send(socket, raw_msg);
    if (status < 0)
        client_disconnect();

    return status;
}

// Wait for max timeout milliseconds when waiting for data to arrive
//TODO: this function doesn't check for validity of the input other than
// the number of arguments (for example length)
int client_receive(struct client_msg* msg, const unsigned int timeout) {

    // This call is non-blocking, thus if there is no new activity, return immediately after timeout milliseconds
    if (SDLNet_CheckSockets(sset, timeout) <= 0)
        return CLIENT_ERR_NOREC;

    struct prot_msg raw_msg = prot_recv(socket);
    if (raw_msg.status < 0) {
        client_disconnect();
        return raw_msg.status;
    }

    int ret;
    if (!strncmp(raw_msg.head, "MSG", PROT_HEAD_SIZE)) {
        if (raw_msg.status != 2) {
            ret = CLIENT_ERR_ARGCOUNT;
            goto err;
        }

        msg->type = CLIENT_MSG_MSG;
        msg->u.rec.msg.sender = raw_msg.args[0];
        msg->u.rec.msg.text = raw_msg.args[1];
    } else if (!strncmp(raw_msg.head, "NIC", PROT_HEAD_SIZE)) {
        if (raw_msg.status != 1) {
            ret = CLIENT_ERR_ARGCOUNT;
            goto err;
        }

        msg->type = CLIENT_MSG_NICK;
        msg->u.rec.nick.newnick = raw_msg.args[0]; 
    } else if (!strncmp(raw_msg.head, "ACC", PROT_HEAD_SIZE)) {
        if (raw_msg.status != 0) {
            ret = CLIENT_ERR_ARGCOUNT;
            goto err;
        }

        msg->type = CLIENT_MSG_ACCEPTED;
    } else if (!strncmp(raw_msg.head, "REF", PROT_HEAD_SIZE)) {
        if (raw_msg.status != 0) {
            ret = CLIENT_ERR_ARGCOUNT;
            goto err;
        }

        msg->type = CLIENT_MSG_REFUSED;
    }

    return CLIENT_ERR_OK;

    err:

    // When an error occurs, free the arguments
    for (size_t i = 0; i < (size_t)raw_msg.status; i++) 
        free(raw_msg.args[i]);

    client_disconnect();

    return ret;
}

int client_init() {
    // Initialize SDL
    if(SDL_Init(0) < 0)
        return CLIENT_ERR_INIT;

    // Initialize SDL_net
    if(SDLNet_Init() < 0) {
        SDL_Quit();
        return CLIENT_ERR_INIT;
    }

    // Allocate the socket set
    sset = SDLNet_AllocSocketSet(1);
    if (!sset) {
        SDLNet_Quit();
        SDL_Quit();
        return CLIENT_ERR_INIT;
    }

    return CLIENT_ERR_OK;
}

void client_deinit() {
    client_disconnect();
    SDLNet_FreeSocketSet(sset);
    SDLNet_Quit();
    SDL_Quit();
}

// (Re)connect to the specified URL at the Port, and wait for an accept response for timeout milliseconds
int client_connect(const char* url, const unsigned short port, const unsigned int timeout) {

    // If the socket already exists, i.e. we are reconnecting, close the socket
    client_disconnect();

    IPaddress addr;
    if (SDLNet_ResolveHost(&addr, url, port) < 0)
        return CLIENT_ERR_CON_FAILED;

    // Open the actual server socket
    socket = SDLNet_TCP_Open(&addr);
    if (!socket)
        return CLIENT_ERR_CON_FAILED;

    // Add the socket to the set
    if (SDLNet_TCP_AddSocket(sset, socket) < 0) {
        SDLNet_TCP_Close(socket);
        socket = NULL;
        return CLIENT_ERR_CON_FAILED;
    }

    // wait for the first answer (with the specified timeout)
    struct client_msg response;
    int status;
    if ((status = client_receive(&response, timeout)) == CLIENT_ERR_OK) {
        if (response.type != CLIENT_MSG_ACCEPTED) {
            client_disconnect();
            return CLIENT_ERR_CON_REFUSED;
        }
    } else
        client_disconnect();

    return status;

}

// Disconnect from the current server
// Note that this only closes the socket but doesn't cleanup the whole system, use client_deinit for final cleanup 
// (which also calls this function)
void client_disconnect() {
    if (socket == NULL) return;

    SDLNet_TCP_DelSocket(sset, socket);
    SDLNet_TCP_Close(socket);
    socket = NULL;
}
