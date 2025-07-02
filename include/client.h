#ifndef CLIENT_H
#define CLIENT_H

#include "header.h"

// Client function declarations
client_t *new_client();
void clean(client_t *client);
void start_client(client_t *client);
int connect_to_server(client_t *app);
int auto_reconnect(client_t *app);

// Client application functions
int create_client_app(client_t *app);  // Modified signature
void destroy_client_app(client_t *app);
int start_client_app(client_t *app);
void stop_client_app(client_t *app);

// Recommendation handling functions
void *receive_recommendations(void *arg);
int send_recommendation_request(client_t *app, recommendation_request_t *req);

// Request parsing function
int parse_recommendation_command(const char *input, recommendation_request_t *req);

// Utility function
void show_recommendation_help();

// Signal handling
void client_signal_handler(int sig);

#endif // CLIENT_H
