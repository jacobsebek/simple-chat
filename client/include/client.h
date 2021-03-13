#pragma once

// Including this so frontends have access to SERV_MAX_MSG_LEN etcetera
#include "server.h"

enum client_errcode {
    CLIENT_ERR_OK, // No error occured
    CLIENT_ERR_INIT, // An initialisation error occured
    CLIENT_ERR_CON_FAILED, // Coudln't connect to server
    CLIENT_ERR_CON_REFUSED, // Server refused connection
    CLIENT_ERR_INVALID_MSG, // Attempt to send an invalid message (maybe exceeding the size limit etc.)
    CLIENT_ERR_ARGCOUNT, // The client has received more arguments in a message than expected
    CLIENT_ERR_NOREC // Nothing to receive in a non-blocking receive function
};

enum client_msg_type {
    // Sent by server after connection
    CLIENT_MSG_ACCEPTED,
    CLIENT_MSG_REFUSED, 
    // A simple message
    CLIENT_MSG_MSG, 
    // A request to change one's nick
    // Note that this doesn't guarantee that the nick will be changed
    // The actual nick that the server will use is always sent back to the client
    // Although usually, the server just sends back the same nick as a confirmation
    CLIENT_MSG_NICK
} type;

// A "generic" message, either sent or receceived from the server
struct client_msg {

    enum client_msg_type type;

    // Note that some of these types can have varying arguments depending on
    // who sends them, either the client or the server
    union {
        // Data received from the server
        union {
            // Corresponds to CLIENT_MSG_MSG
            struct {
                char* sender;
                char* text;
            } msg;

            // CLIENT_MSG_NICK
            struct {
                char* newnick;
            } nick;
        } rec;

        // Data sent to the server
        union {
            // CLIENT_MSG_MSG   
            struct {
                char* text;
            } msg;
            
            // CLIENT_MSG_NICK   
            struct {
                char* newnick;
            } nick;
        } send;
    } u;
};

// Send a message to the server
// Can return protlib error codes as well as client error codes
// Only disconnects the client when returning protlib errors (aka values < 0)
// It does all the necessary checks for message validity before sending
int client_send(const struct client_msg* msg);

// Receive a message from the server (waiting at max for timeout milliseconds)
// Can return protlib error codes as well as client error codes
// Disconnects whenever the return value isn't CLIENT_ERR_OK
// Checks for valid argument count but not for the argument length
int client_receive(struct client_msg* msg, const unsigned int timeout);

// Initialise the backend, call this before using any other functions
// Returns either OK or CLIENT_ERR_INIT
int client_init();

// Disconnects and cleans up the backend, use at the exit of the app
// Always succeeds
void client_deinit();

// Connect to the server at URL at Port, wait for CLIENT_MSG_ACCEPTED for at max timeout milliseconds
// If it doesn't arrive in that time or anything else than CLIENT_ERR_OK is returned, the connection is closed
// Returns what client_receive returned
int client_connect(const char* url, const unsigned short port, const unsigned int timeout);

// Disconnect from the current server
// Always succeeds
void client_disconnect();
