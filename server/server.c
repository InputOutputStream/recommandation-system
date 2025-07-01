#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#include <server.h>

/***
 *The server 
 * 
 */

 // Variables globales avec mutex pour le controle d'accescote serveur
client_t clients[MAX_CLIENT]; // Liste des clients en cours
int client_count; // Nombre de clients connecter

//client mutex
pthread_mutex_t clients_mutex; 
int server_running; 




int start_reco_server()
{
    // 2. Création du socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socketcreation failed");
        return -1;
    }


     // Réutilisation de l'adresse, de la machine pour l'interface 1 je supose
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. Préparation de la structure sockaddr_in pour le serveur
    struct sockaddr_in server_addr = {0};    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);  // Convertit en big-endian
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 4. Bind
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

   
    if (listen(server_fd, MAX_CLIENT) < 0) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }

    //C'est Ici que tout commence, logique du server de chat
    pthread_t accept_thread;
    int *server_fd_ptr = calloc(1, sizeof(int));
    *server_fd_ptr = server_fd;

    pthread_create(&accept_thread, NULL, accept_connections, server_fd_ptr);
    pthread_detach(accept_thread); // terminate automatically on close

    return 0;
}

void *accept_connections(void *arg) {
    int server_fd = *((int*)arg);
    free(arg);

    while(server_running)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) continue; // on n'a pas reussi a le connecter bah on passe

        pthread_mutex_lock(&clients_mutex);
        if(client_count < MAX_CLIENT)
        {
            client_t *client = &clients[client_count];
            client->client_fd = client_fd;
            client->client_addr = client_addr;
            client->client_id = client_count;
            client->client_addr_len = addr_len;
            client->active = 1;
            client->connect_time = time(NULL);
            snprintf(client->client_name, sizeof(client->client_name), "User%d", client_count);
            
            pthread_create(&client->thread, NULL, handle_client, client);
            client_count++;
        }

        pthread_mutex_unlock(&clients_mutex);
    }

    close(server_fd);
    return NULL;
}


void *handle_client(void *arg)
{
    client_t *client =  (client_t *)arg;
    char buffer[MAX_MESSAGE_LENGHT];

    char *result = NULL; // Va changer en la structure de donnees approprier

    while(server_running && client->active)
    {
        size_t bytes = recv(client->client_fd, buffer, sizeof(buffer) -1, 0);

        if(bytes == 0) break;

        buffer[bytes] = '\0';

        /**broadcaster le message a tout les clients
         * 
         * Cette facon de faire va changer si on decide d'utiliser un id comme un numero par exemple
         * 
         * */

        //recuperation de ce que le client veut
        handle_client_message(buffer, result); // traitement de la requette du client
        // traitement stockage du resultat dans la structure adequate

        send(client->client_fd, result, strlen(result), 0);
    }

    client->active = 0;
    close(client->client_fd);
    return NULL;

}


void handle_client_message(char *message, char *result)
{

    return;
}


void broadcast_message(client_t *sender, const char *message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && clients[i].client_fd != sender->client_fd) {
            send(clients[i].client_fd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}



void *handle_client_timeout(void *arg) {
    client_t *client = (client_t*)arg;
    time_t last_activity = time(NULL);
    
    while (client->active) {
        if (time(NULL) - last_activity > CLIENT_TIMEOUT) {
            // Client trop lent, déconnexion
            disconnect_slow_client(client);
            break;
        }
        sleep(1);
    }
    return NULL;
}

void disconnect_slow_client(client_t *client)
{
    return;
}

// Validation côté serveur
int validate_client_message(client_t *client, message_t *msg) {
    
    return 0;
}