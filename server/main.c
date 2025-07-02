#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"
#include "server.h"

int main() {
    printf("Starting Reconnaissance Server...\n");
    
    int result = start_reco_server();
    
    if (result != 0) {
        fprintf(stderr, "Server failed to start\n");
        return 1;
    }
    
    printf("Server shutdown complete.\n");
    return 0;
}