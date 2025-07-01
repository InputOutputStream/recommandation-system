
#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#include <header.h>


// MACROS




// client.h
typedef struct CLIENT_APP {
    // Connexion réseau
    int server_fd;
    struct sockaddr_in server_addr;
    
    // État local
    char username[64];
    char current_channel[64];
    int client_number;
    int message_count;
    
    // Threads
    pthread_t network_thread;    // Réception messages
    pthread_t ui_thread;         // Interface utilisateur
    pthread_t heartbeat_thread;  // Keep-alive
    
    // Synchronisation
    pthread_mutex_t message_mutex;
    int running;
} client_app_t;



client_t *new_client();
int connect_to_server();
void start_client(client_t *client);
void clean(client_t *client);

#endif // !CLIENT_H