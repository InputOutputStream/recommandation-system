#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"
#include "client.h"

int main() {
    printf("Starting Client............\n");

    client_t *client = new_client();
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        return 1;
    }
    
    start_client(client);

    clean(client);
    printf("Client terminated.\n");
    return 0;
}