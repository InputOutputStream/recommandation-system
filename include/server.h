#ifndef SERVER_H
#define SERVER_H

#include "header.h"

/***
 *The server 
 * header file 
 */

extern int start_reco_server();

void *accept_connections(void *arg);

extern void display_messages(const char *user_name, const char *messages);
extern void update_client_list(client_t *list);
extern void update_status(client_t*client_list, const char *status);

extern void *handle_client(void *arg);
extern void broadcast_message(client_t *sender, const char *message);

#endif // !