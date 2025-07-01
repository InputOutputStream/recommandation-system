#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <header.h>
#include <client.h>

// Ce menu est sense rester comme ca on va voir comment tout implmenter ailleur
int main() {

    fprintf(stdout, "Startting Client............\n");

    client_t *client = new_client();
    start_client(client);

    // Add a kind of menu to permit operation choice 


    clean(client);
    return 0;
}