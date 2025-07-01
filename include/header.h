#ifndef HEADER_H
#define HEADER_H

#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>


#define MAX_CLIENT 250
#define MAX_MESSAGE_LENGHT 1024 // increase later or not
#define DEFAULT_PORT 8008
#define NAME_LENGHT 128
#define CLIENT_TIMEOUT 10000


/**
 * From the oldest message to the newest
 */

typedef struct MESSAGES
{
    char *payload[MAX_MESSAGE_LENGHT];
    uint16_t sender_user_id;
    time_t timestamp;
}message_t;


typedef struct CLIENT
{
    struct sockaddr_in client_addr; 
    unsigned int client_addr_len;
    size_t client_id;
    char client_name[NAME_LENGHT];
    time_t connect_time;
    size_t client_number;
    int client_fd;
    pthread_t thread; // Client execution thread
    int active;
}client_t;


typedef struct RESULT
{
    // Properties of the chat
    u_int16_t user_id;
    message_t *message;
}result_t;



typedef enum {
    MSG_SEND,           // Envoyer message
    MSG_HEARTBEAT,      // Keep-alive
    MSG_DISCONNECT      // Déconnexion propre
} client_message_type_t;


typedef enum {
    MSG_BROADCAST,      // Message diffusé
    MSG_USER_JOIN,      // Utilisateur rejoint
    MSG_USER_LEAVE,     // Utilisateur part
    MSG_USER_LIST,      // Liste des utilisateurs
    MSG_ERROR,          // Erreur serveur
    MSG_PING            // Ping du serveur
} server_message_type_t;

// Variables globales avec mutex pour le controle d'accescote serveur
extern client_t clients[MAX_CLIENT]; // Liste des clients en cours
extern int client_count; // Nombre de clients connecter


typedef struct DATE{
    int day, month, year;
}date_t;

//client mutex
extern pthread_mutex_t clients_mutex; 
extern int server_running; 


#endif // !HEADER_H