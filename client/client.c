#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "header.h"
#include "client.h"

// Global client instance for signal handling
static client_t *global_app = NULL;

// Signal handler for graceful shutdown
void client_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nDisconnecting from recommendation server...\n");
        if (global_app) {
            pthread_mutex_lock(&global_app->client_mutex);
            global_app->running = 0;
            pthread_mutex_unlock(&global_app->client_mutex);
        }
    }
}

client_t *new_client() {
    return calloc(1, sizeof(client_t));
}

void clean(client_t *client) {
    if (client) {
        free(client);
    }
}

void start_client(client_t *client) {
    if (!client) return;
    
    printf("Starting recommendation client...\n");
    
    // Use the passed client instead of creating new one
    if (create_client_app(client) != 0) {
        printf("Failed to initialize client application\n");
        return;
    }
    
    global_app = client;
    signal(SIGINT, client_signal_handler);
    signal(SIGTERM, client_signal_handler);
    
    printf("Connecting to recommendation server...\n");
    
    // Start client application
    if (start_client_app(client) == 0) {
        printf("Successfully connected!\n");
        show_recommendation_help();
        
        // Main client loop
        char input[MAX_MESSAGE_LENGTH];
        while (client->running) {
            printf("Recommendation> ");
            fflush(stdout);
            
            if (fgets(input, sizeof(input), stdin)) {
                // Remove newline and validate input
                size_t len = strlen(input);
                if (len > 0 && input[len-1] == '\n') {
                    input[len-1] = '\0';
                }
                
                if (strlen(input) == 0) continue;
                
                // Handle special commands
                if (strcmp(input, "/quit") == 0 || strcmp(input, "/exit") == 0) {
                    break;
                } else if (strcmp(input, "/help") == 0) {
                    show_recommendation_help();
                    continue;
                } 
                
                // Parse and send recommendation request
                recommendation_request_t req;
                if (parse_recommendation_command(input, &req)) {
                    send_recommendation_request(client, &req);
                } else {
                    printf("Unrecognized command. Type /help for available commands.\n");
                }
            }
        }
    } else {
        printf("Failed to connect to server\n");
    }
    
    stop_client_app(client);
    destroy_client_app(client);
    global_app = NULL;
}

int create_client_app(client_t *app) {
    if (!app) return -1;
    
    app->socket_fd = -1;
    app->running = 0;
    app->active = 0;
    app->receive_thread = 0;
    
    // Initialize mutex
    if (pthread_mutex_init(&app->client_mutex, NULL) != 0) {
        perror("Failed to initialize client mutex");
        return -1;
    }
    
    return 0;
}

void destroy_client_app(client_t *app) {
    if (app) {
        if (app->socket_fd >= 0) {
            close(app->socket_fd);
        }
        pthread_mutex_destroy(&app->client_mutex);
    }
}

int start_client_app(client_t *app) {
    if (!app) return -1;
    
    // Connect to recommendation server
    if (connect_to_server(app) != 0) {
        return -1;
    }
    
    pthread_mutex_lock(&app->client_mutex);
    app->running = 1;
    app->active = 1;
    pthread_mutex_unlock(&app->client_mutex);
    
    // Start receive thread for recommendations
    if (pthread_create(&app->receive_thread, NULL, receive_recommendations, app) != 0) {
        perror("Failed to create receive thread");
        pthread_mutex_lock(&app->client_mutex);
        app->running = 0;
        app->active = 0;
        pthread_mutex_unlock(&app->client_mutex);
        return -1;
    }
    
    return 0;
}

void stop_client_app(client_t *app) {
    if (!app) return;
    
    pthread_mutex_lock(&app->client_mutex);
    if (app->running) {
        app->running = 0;
        app->active = 0;
        pthread_mutex_unlock(&app->client_mutex);
        
        // Wait for receive thread to finish
        if (app->receive_thread != 0) {
            pthread_join(app->receive_thread, NULL);
            app->receive_thread = 0;
        }
    } else {
        pthread_mutex_unlock(&app->client_mutex);
    }
    
    if (app->socket_fd >= 0) {
        close(app->socket_fd);
        app->socket_fd = -1;
    }
}

int connect_to_server(client_t *app) {
    if (!app) return -1;
    
    // Create socket
    app->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (app->socket_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Setup server address
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    
    // Convert IP address
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(app->socket_fd);
        app->socket_fd = -1;
        return -1;
    }
    
    // Connect to server
    if (connect(app->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(app->socket_fd);
        app->socket_fd = -1;
        return -1;
    }
    
    return 0;
}

int auto_reconnect(client_t *app) {
    if (!app) return -1;
    
    int attempts = 0;
    const int max_attempts = 5;
    const int retry_delay = 2;
    
    // Fixed: Check if app is still running, not !running
    while (attempts < max_attempts && app->running) {
        printf("Reconnection attempt %d/%d...\n", attempts + 1, max_attempts);
        
        if (connect_to_server(app) == 0) {
            printf("Reconnected successfully!\n");
            return 0;
        }
        
        attempts++;
        if (attempts < max_attempts) {
            sleep(retry_delay * attempts);
        }
    }
    
    printf("Failed to reconnect after %d attempts\n", max_attempts);
    return -1;
}

void *receive_recommendations(void *arg) {
    client_t *app = (client_t*)arg;
    char buffer[MAX_MESSAGE_LENGTH];
    
    while (1) {
        pthread_mutex_lock(&app->client_mutex);
        int should_run = app->running;
        pthread_mutex_unlock(&app->client_mutex);
        
        if (!should_run) break;
        
        ssize_t bytes = recv(app->socket_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes <= 0) {
            if (bytes == 0) {
                printf("\nRecommendation server disconnected\n");
            } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("recv failed");
            }
            
            // Try to reconnect
            if (auto_reconnect(app) != 0) {
                pthread_mutex_lock(&app->client_mutex);
                app->running = 0;
                pthread_mutex_unlock(&app->client_mutex);
            }
            break;
        }
        
        buffer[bytes] = '\0';
        printf("\n%s\nRecommendation> ", buffer);
        fflush(stdout);
    }
    
    return NULL;
}

int send_recommendation_request(client_t *app, recommendation_request_t *req) {
    if (!app || !req || app->socket_fd < 0) return -1;
    
    char buffer[MAX_MESSAGE_LENGTH];
    int len = snprintf(buffer, sizeof(buffer), "%d %d %d %d %d", 
                       req->user_id, req->algorithm, req->k, req->num_recommendations, req->category_filter);
    
    if (len >= sizeof(buffer)) {
        printf("Request too long\n");
        return -1;
    }
    
    ssize_t bytes_sent = send(app->socket_fd, buffer, len, 0);
    if (bytes_sent < 0) {
        perror("Send failed");
        return -1;
    }
    
    return 0;
}

int parse_recommendation_command(const char *input, recommendation_request_t *req) {
    if (!input || !req) return 0;
    
    memset(req, 0, sizeof(recommendation_request_t));
    req->category_filter = -1; // Default: no filter
    
    // Parse /recommend user_id algorithm count [category]
    if (strncmp(input, "/recommend ", 11) == 0) {
        char algo_str[32];
        int count = 5; // default
        int category = -1; // default
        int temp;
        req->k == 0;
        
        int parsed = sscanf(input + 11, "%d %s %d %d %d", &req->user_id, algo_str, &req->k, &count, &category);
        
        if (parsed >= 2) {
            // Parse algorithm
            if (strcasecmp(algo_str, "knn") == 0) {
                if(req->k == 0 || category == -1){
                    fprintf(stderr, "k value null or not received, switching to default k value 3\n", req->k);
                    temp = count;
                    count = req->k;
                    category = temp;
                    req->k = DEFAULT_K;
                }
                req->algorithm = ALGO_KNN;
            } else if (strcasecmp(algo_str, "mf") == 0) {
                req->algorithm = ALGO_MF;
            } else if (strcasecmp(algo_str, "graph") == 0) {
                req->algorithm = ALGO_GRAPH;
            } else {
                printf("Invalid algorithm '%s'. Using KNN as default.\n", algo_str);
                printf("Using KNN as default with k = %d.\n", DEFAULT_K);
                req->algorithm = ALGO_KNN;
            }
            
            // Set count and category
            req->num_recommendations = (count > 0 && count <= MAX_RECOMMENDATIONS) ? count : 5;
            req->category_filter = category;
            
            return 1;
        }
    }
    
    return 0;
}

void show_recommendation_help() {
    printf("\n=== Recommendation Client Help ===\n");
    printf("Available commands:\n");
    printf("  /help                        - Show this help\n");
    printf("  /recommend <uid> <algo> [k] [n] [cat] - Get recommendations\n");
    printf("      uid: User ID (0-%d)\n", MAX_USERS-1);
    printf("      algo: knn, mf, graph\n");
    printf("      k value: set to 0 if algo is not knn\n");
    printf("      n: Number of recommendations (1-%d, default: 5)\n", MAX_RECOMMENDATIONS);
    printf("      cat: Category filter (-1 for none, default: -1)\n");
    printf("  /quit, /exit                 - Quit the application\n");
    printf("\nExample: /recommend 123 knn 3 10 2\n");
    printf("===============================\n\n");
}

