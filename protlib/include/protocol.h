// Check the protocol readme to learn about the protocol

#pragma once

#include <SDL_net.h>

// These values are arbitrary but ensure safety of the code
// They can be changed however (but not too ridiculous, keep
// in mind that every call to prot_recv allocates a PROT_MAX_ARG_SIZE buffer for example)
#define PROT_HEAD_SIZE 3
#define PROT_MAX_ARG_SIZE 4096
#define PROT_MAX_ARGS 8

//TODO: more specific errcodes!
enum prot_errcode {
    PROT_ERR_OK = 0,
    PROT_ERR_ERR = -1
};

// A low-lever message structure
// Can be received by prot_recv and sent by prot_send
struct prot_msg {

    // The status indicates the number of arguments in "args", but
    // can also have a negative value, which signifies an error
    int status;
    // The head, used to tell apart types of messages
    char head[PROT_HEAD_SIZE];
    // An array of null-terminated arguments of varying size
    char* args[PROT_MAX_ARGS]; 
};

// A convenience function for elegantly manufacturing messages
struct prot_msg prot_make_msg(const char* head, const int num_args, ...);

// Receive message, the status member of the returned structure
// contains either the number of arguments or a negative enum prot_errcode value
struct prot_msg prot_recv(TCPsocket socket);

// Send message, returns enum prot_errcode values
int prot_send(TCPsocket socket, const struct prot_msg msg);
