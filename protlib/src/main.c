#include "protocol.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Convenience function, constructs a message that can be sent by prot_send
struct prot_msg prot_make_msg(const char* head, const int num_args, ...) {

    struct prot_msg msg;

    // Set the number of arguments
    msg.status = num_args;
    if (msg.status < 0)
        return msg;

    // Copy the head
    strncpy(msg.head, head, PROT_HEAD_SIZE);

    // Copy the arguments
    va_list args;
    va_start(args, num_args);

    for (size_t i = 0; i < (size_t)num_args; i++)
        msg.args[i] = va_arg(args, char*);
    
    va_end(args);

    return msg;
}

// Receive and format a whole message from socket
struct prot_msg prot_recv(TCPsocket socket) {

    struct prot_msg msg;
    char* buf = NULL;

    // Get head
    if (SDLNet_TCP_Recv(socket, msg.head, PROT_HEAD_SIZE) != PROT_HEAD_SIZE)
        goto err;

    // Receive the null-separated list of arguments terminated by an empty argument

    // Intermediate buffer used when reading the arguments
    buf = malloc(PROT_MAX_ARG_SIZE);
    if (!buf) goto err;
    // Status is used to keep track of the number of arguments
    msg.status = 0;
    while (1) {

        // Check if we haven't exceeded the maximum number of arguments
        if (msg.status >= PROT_MAX_ARGS) goto err;

        // Receive the argument
        size_t size = 0;
        do {
            if (size >= PROT_MAX_ARG_SIZE) goto err;
            if (SDLNet_TCP_Recv(socket, &buf[size], 1) != 1) goto err;
        } while (buf[size++] != '\0');

        // An empty argument means the end
        if (buf[0] == '\0') break;
        
        // Strdup reallocates the string and ensures that each argument 
        // is as small as possible (and not PROT_MAX_ARG_SIZE every time)
        if ( NULL == (msg.args[msg.status++] = strdup(buf)) ) goto err;
    }

    free(buf);
    return msg;

    err:

    free(buf);
    msg.status = PROT_ERR_ERR;
    return msg;
}

// Send a message over socket
int prot_send(TCPsocket socket, const struct prot_msg msg) {

    if (msg.status < 0 || msg.status > PROT_MAX_ARGS) return PROT_ERR_ERR;

    // Send the head
    if (SDLNet_TCP_Send(socket, msg.head, PROT_HEAD_SIZE) != PROT_HEAD_SIZE)
        return PROT_ERR_ERR;    

    // Send the arguments
    for (size_t i = 0; i < (size_t)msg.status; i++) {

        size_t size = strlen(msg.args[i])+1;
        if (size > PROT_MAX_ARG_SIZE) return PROT_ERR_ERR;

        if ((size_t)SDLNet_TCP_Send(socket, msg.args[i], size) != size)
            return PROT_ERR_ERR;    
        
    }

    // Send the final null-terminator (aka empty argument)
    if (SDLNet_TCP_Send(socket, &(int){0}, 1) != 1)
        return PROT_ERR_ERR;    

    return PROT_ERR_OK;
}
