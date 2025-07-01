#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <ncurses.h>
#include <unistd.h>


#include <header.h>
#include <client.h>





client_t *new_client()
{
    return calloc(1, sizeof(client_t));
}

void clean(client_t *client)
{
    free(client);
    return;
}


void start_client(client_t *client)
{

    return;
}


// Reconnexion automatique côté client
int auto_reconnect(client_app_t *app) {
    int attempts = 0;
    const int max_attempts = 5;
    const int retry_delay = 2;  // secondes
    
    while (attempts < max_attempts && !app->running) {
        
        if (connect_to_server(app) == 0) {
            return 0;
        }
        
        attempts++;
        sleep(retry_delay * attempts);  // Backoff exponentiel
    }
    
    return -1;
}


int connect_to_server()
{

    return 0;
}

