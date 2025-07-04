#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <math.h>

#include "header.h"
#include "server.h"
#include "traitement.h"

#include <ndmath/io.h>
#include <ndmath/helper.h>

#include <mf/mf.h>
#include <knn/knn.h>
#include <graph/graph.h>

// Global variables
client_t clients[MAX_CLIENT];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_running = 1;
recommendation_system_t rec_system;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nShutting down server gracefully...\n");
        server_running = 0;
    }
}

void init_server() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    memset(clients, 0, sizeof(clients));
    pthread_mutex_init(&clients_mutex, NULL);
    init_recommendation_system();
    printf("Server initialized on port %d\n", DEFAULT_PORT);
}

void cleanup_server() {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active) {
            close(clients[i].client_fd);
            clients[i].active = 0;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&rec_system.data_mutex);
    printf("Server cleanup completed\n");
}

int start_reco_server() {
    init_server();
    
    // Load ratings data
    load_ratings_data("server/data/ratings.txt");
    
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Allow address reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return -1;
    }

    // Setup server address
    struct sockaddr_in server_addr = {0};    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENT) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }

    printf("Server listening on port %d...\n", DEFAULT_PORT);

    // Start accepting connections in a separate thread
    pthread_t accept_thread;
    int *server_fd_ptr = malloc(sizeof(int));
    *server_fd_ptr = server_fd;

    if (pthread_create(&accept_thread, NULL, accept_connections, server_fd_ptr) != 0) {
        perror("Failed to create accept thread");
        free(server_fd_ptr);
        close(server_fd);
        return -1;
    }

    // Wait for the accept thread to finish (when server_running becomes 0)
    pthread_join(accept_thread, NULL);
    
    close(server_fd);
    cleanup_server();
    return 0;
}

void *accept_connections(void *arg) {
    int server_fd = *((int*)arg);
    free(arg);

    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            if (!server_running) break;
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENT) {
            client_t *client = &clients[client_count];
            client->client_fd = client_fd;
            client->client_addr = client_addr;
            client->client_id = client_count;
            client->client_addr_len = addr_len;
            client->active = 1;
            client->client_id = client_count;   

            log_message("New client connected: %d from %s\n", client->client_id, inet_ntoa(client_addr.sin_addr));

            if (pthread_create(&client->receive_thread, NULL, handle_client, client) != 0) {
                perror("Failed to create client thread");
                close(client_fd);
                client->active = 0;
            } else {
                client_count++;
                pthread_detach(client->receive_thread);
            }
        } else {
            log_message("Maximum clients reached, rejecting connection\n");
            close(client_fd);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    return NULL;
}


void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[MAX_MESSAGE_LENGTH];
    
    while (server_running && client->active) {
        ssize_t bytes = recv(client->client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes <= 0) {
            if (bytes == 0) {
                log_message("Client %d disconnected\n", client->client_id);
            } else {
                perror("recv failed");
            }
            break;
        }

        buffer[bytes] = '\0';
        
        // Parse recommendation request
        recommendation_request_t req;
        int category_filter = -1;
        int parsed = sscanf(buffer, "%d %d %d %d %d", 
                           &req.user_id, (int*)&req.algorithm, &req.k,
                           &req.num_recommendations, &category_filter);
        
        if (parsed >= 3) {
            req.category_filter = category_filter;
            if (req.num_recommendations > MAX_RECOMMENDATIONS) {
                req.num_recommendations = MAX_RECOMMENDATIONS;
            }
            
            recommendation_result_t results[MAX_RECOMMENDATIONS];
            int num_results = 0;
            
            get_recommendations(&req, results, &num_results);
            
            // Send response
            char response[MAX_MESSAGE_LENGTH];
            format_recommendation_response(&req, results, num_results, response);
            send(client->client_fd, response, strlen(response), 0);
        } else {
            char error_msg[] = "ERROR: Invalid format. Expected: user_id algorithm num_recommendations [category_filter]\n";
            send(client->client_fd, error_msg, strlen(error_msg), 0);
        }
    }

    remove_client(client);
    return NULL;
}


void format_recommendation_response(recommendation_request_t* req, recommendation_result_t* results, 
                                  int num_results, char* response) {
    snprintf(response, MAX_MESSAGE_LENGTH, "RECOMMENDATIONS for user %d:\n", req->user_id);
    
    for (int i = 0; i < num_results; i++) {
        char temp[256];
        snprintf(temp, sizeof(temp), "Item %d (Category %d): Rating %.2f\n",
                results[i].item_id, results[i].category_id, results[i].predicted_rating);
        
        if (strlen(response) + strlen(temp) < MAX_MESSAGE_LENGTH - 1) {
            strcat(response, temp);
        } else {
            break;
        }
    }
    
    if (num_results == 0) {
        strcat(response, "No recommendations available for this user.\n");
    }
}


void remove_client(client_t *client) {
    if (!client) return;
    
    pthread_mutex_lock(&clients_mutex);
    close(client->client_fd);
    client->active = 0;
    
    // Compacter le tableau des clients
    int removed_index = client - clients;
    for (int i = removed_index; i < client_count - 1; i++) {
        clients[i] = clients[i + 1];
        clients[i].client_id = i;  // Réajuster les IDs
    }
    client_count--;
    
    log_message("Client removed, %d clients remaining\n", client_count);
    pthread_mutex_unlock(&clients_mutex);
}

void log_message(const char *format, ...) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    fprintf(stderr, "[%02d:%02d:%02d] ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    fflush(stdout);
}

// Recommendation System logic

void init_recommendation_system() {
    memset(&rec_system, 0, sizeof(rec_system));
    
    // Initialiser la matrice à -1
    for (int i = 0; i < MAX_USERS; i++) {
        for (int j = 0; j < MAX_ITEMS; j++) {
            rec_system.user_item_matrix[i][j] = -1;
        }
    }
    
    pthread_mutex_init(&rec_system.data_mutex, NULL);
    log_message("Recommendation system initialized\n");
}

void load_ratings_data(const char* filename) {
    if (filename == NULL) {
        log_message("Error: Filename cannot be NULL");
        return;
    }
    
    // Charger le fichier avec ndmath
    ndarray_t data = load_ndarray(filename, 10);
    
    if (data.data == NULL) {
        log_message("Error: Failed to load ratings data from %s", filename);
        return;
    }
    
    pthread_mutex_lock(&rec_system.data_mutex);
    
    // Réinitialiser le système
    memset(&rec_system.ratings, 0, sizeof(rec_system.ratings));
    rec_system.num_ratings = 0;
    rec_system.num_users = 0;
    rec_system.num_items = 0;
    
    // Initialiser la matrice à -1
    for (int i = 0; i < MAX_USERS; i++) {
        for (int j = 0; j < MAX_ITEMS; j++) {
            rec_system.user_item_matrix[i][j] = -1;
        }
    }
    
    size_t loaded_count = 0;
    size_t total_rows = data.shape[0];
    
    for (size_t i = 0; i < total_rows && loaded_count < MAX_RATINGS; i++) {
        // Format attendu: user_id, item_id, [category_id], rating , [timestamp]

        int user_id = (int)data.data[i][0];
        int item_id = (int)data.data[i][1];
        int category_id = (int)data.data[i][2];
        float rating = (float)data.data[i][3];
        double timestamp = (double)data.data[i][3];

        // Validation des données
        if (user_id < 0 || user_id >= MAX_USERS || 
            item_id < 0 || item_id >= MAX_ITEMS ||
            rating < 0.0 || rating > 5.0) {
            continue;
        }
        
        // Ajouter le rating
        rating_t *r = &rec_system.ratings[loaded_count];
        r->user_id = user_id;
        r->item_id = item_id;
        r->category_id = category_id;
        r->rating = rating;
        r->timestamp = timestamp;
        
        // Mettre à jour la matrice
        rec_system.user_item_matrix[user_id][item_id] = (int)(rating * 10); // Scale 0-50
        
        // Mettre à jour les compteurs max
        if (user_id >= rec_system.num_users) {
            rec_system.num_users = user_id + 1;
        }
        if (item_id >= rec_system.num_items) {
            rec_system.num_items = item_id + 1;
        }
        
        loaded_count++;
    }
    
    rec_system.num_ratings = loaded_count;
    
    pthread_mutex_unlock(&rec_system.data_mutex);
    
    // Libérer la mémoire du ndarray
    free_array(&data);
    
    log_message("Loaded %zu ratings from %s (%d users, %d items)", 
                loaded_count, filename, rec_system.num_users, rec_system.num_items);
}


int add_rating(int user_id, int item_id, int category_id, float rating) {
    pthread_mutex_lock(&rec_system.data_mutex);
    
    if (rec_system.num_ratings >= MAX_RATINGS) {
        pthread_mutex_unlock(&rec_system.data_mutex);
        return 0;
    }
    
    // Ajouter à la liste des ratings
    rating_t *r = &rec_system.ratings[rec_system.num_ratings];
    r->user_id = user_id;
    r->item_id = item_id;
    r->category_id = category_id;
    r->rating = rating;

    time_t ti = time(NULL);
    r->timestamp = (double)ti;
    
    // Synchroniser avec la matrice
    if (user_id < MAX_USERS && item_id < MAX_ITEMS) {
        rec_system.user_item_matrix[user_id][item_id] = (int)(rating * 10); // Scale 0-50
        
        // Mettre à jour les compteurs
        if (user_id >= rec_system.num_users) {
            rec_system.num_users = user_id + 1;
        }
        if (item_id >= rec_system.num_items) {
            rec_system.num_items = item_id + 1;
        }
    }
    
    rec_system.num_ratings++;
    
    pthread_mutex_unlock(&rec_system.data_mutex);
    return 1;
}

void get_recommendations(recommendation_request_t* request, recommendation_result_t* results, int* num_results) {
    *num_results = 0;
    
    switch (request->algorithm) {
        case ALGO_KNN:
            knn_recommendation(request->user_id, request->k, results, num_results, request->num_recommendations);
            break;
        case ALGO_MF:
            matrix_factorization_recommendation(request->user_id, results, num_results, request->num_recommendations);
            break;
        case ALGO_GRAPH:
            graph_recommendation(request->user_id, results, num_results, request->num_recommendations);
            break;
        default:
            log_message("Unknown recommendation algorithm: %d", request->algorithm);
            break;
    }
}

ndarray_t rating_to_aray(rating_t *ratings, size_t rows, size_t cols)
{
    ndarray_t result = array(rows, cols);
    for(int i = 0; i<rows; i++)
    { 
        set(ratings, i, 0, (double)ratings[i].user_id);
        set(ratings, i, 1, (double)ratings[i].item_id);
        set(ratings, i, 2, (double)ratings[i].category_id);
        set(ratings, i, 3, (double)ratings[i].rating);
        set(ratings, i, 4, (double)ratings[i].timestamp);
    } 
   return result;
}


void knn_recommendation(int user_id, int k, recommendation_result_t* results, int* num_results, int max_results) {
    pthread_mutex_lock(&rec_system.data_mutex);
    
    // Create user-item matrix from rec_system
    ndarray_t rating_matrix = array(rec_system.num_users, rec_system.num_items);
    if (!rating_matrix.data) {
        log_message("Failed to allocate rating matrix for KNN");
        pthread_mutex_unlock(&rec_system.data_mutex);
        *num_results = 0;
        return;
    }

    // Fill the matrix: 0.0 for unrated, actual rating otherwise
    for (int u = 0; u < rec_system.num_users; u++) {
        for (int i = 0; i < rec_system.num_items; i++) {
            if (rec_system.user_item_matrix[u][i] >= 0) {
                // Convert back from integer representation
                set(&rating_matrix, u, i, rec_system.user_item_matrix[u][i] / 10.0);
            } else {
                set(&rating_matrix, u, i, 0.0);
            }
        }
    }

    // Initialize KNN model
    knn_t *model = init_knn(k);
    if (!model) {
        log_message("Failed to initialize KNN model");
        free_array(&rating_matrix);
        pthread_mutex_unlock(&rec_system.data_mutex);
        *num_results = 0;
        return;
    }

    // Fit the model with the rating matrix
    model = fitx(model, rating_matrix);
    if (!model) {
        log_message("Failed to fit KNN model");
        free_knn(model);
        free_array(&rating_matrix);
        pthread_mutex_unlock(&rec_system.data_mutex);
        *num_results = 0;
        return;
    }

    // Predict ratings for unrated items
    *num_results = 0;
    for (int item_id = 0; item_id < rec_system.num_items; item_id++) {
        // Skip if user has already rated this item
        if (rec_system.user_item_matrix[user_id][item_id] >= 0) {
            continue;
        }

        // Only recommend up to max_results
        if (*num_results >= max_results) {
            break;
        }

        double pred = predict_rating(model, user_id, item_id);
        results[*num_results].item_id = item_id;
        results[*num_results].category_id = -1; // Not available in this context
        results[*num_results].predicted_rating = pred;
        (*num_results)++;
    }

    // Clean up
    free_knn(model);
    free_array(&rating_matrix);
    pthread_mutex_unlock(&rec_system.data_mutex);
}

void matrix_factorization_recommendation(int user_id, 
                                         recommendation_result_t* results, 
                                         int* num_results, 
                                         int max_results) {
    pthread_mutex_lock(&rec_system.data_mutex);
    
    // Convert ratings to ndarray format
    ndarray_t ratings = array(rec_system.num_ratings, 5);
    if (!ratings.data) {
        log_message("Failed to allocate ratings array");
        pthread_mutex_unlock(&rec_system.data_mutex);
        *num_results = 0;
        return;
    }
    
    for (int i = 0; i < rec_system.num_ratings; i++) {
        set(&ratings, i, 0, (double)rec_system.ratings[i].user_id);
        set(&ratings, i, 1, (double)rec_system.ratings[i].item_id);
        set(&ratings, i, 2, (double)rec_system.ratings[i].category_id);
        set(&ratings, i, 3, (double)rec_system.ratings[i].rating);
        set(&ratings, i, 4, (double)rec_system.ratings[i].timestamp);
    }
    
    // Save to temp file for MF processing
    save_ndarray(&ratings, "server/data/temp_ratings.txt");
    
    // Train MF model
    ndarray_t full_matrix = MF("server/data/temp_ratings.txt", 64, 10, 0.01, 0.1, 20);
    if (full_matrix.shape[0] == 0) {
        log_message("Matrix factorization failed");
        free_array(&ratings);
        pthread_mutex_unlock(&rec_system.data_mutex);
        *num_results = 0;
        return;
    }
    
    // Get recommendations for this user
    *num_results = 0;
    for (int item_id = 0; item_id < full_matrix.shape[1]; item_id++) {
        // Skip if user has already rated this item
        if (user_id < rec_system.num_users && 
            item_id < rec_system.num_items && 
            rec_system.user_item_matrix[user_id][item_id] >= 0) {
            continue;
        }
        
        // Only recommend up to max_results
        if (*num_results >= max_results) {
            break;
        }
        
        double pred_rating = full_matrix.data[user_id][item_id];
        results[*num_results].item_id = item_id;
        results[*num_results].category_id = -1; // Not available in this context
        results[*num_results].predicted_rating = pred_rating;
        (*num_results)++;
    }
    
    // Clean up
    free_array(&full_matrix);
    free_array(&ratings);
    pthread_mutex_unlock(&rec_system.data_mutex);
}



void graph_recommendation(int user_id, recommendation_result_t* results, int* num_results, int max_results) {
    pthread_mutex_lock(&rec_system.data_mutex);
    
    *num_results = 0;
    
    // Vérifier si l'utilisateur existe
    if (user_id < 0 || user_id >= rec_system.num_users) {
        log_message("Invalid user ID: %d", user_id);
        pthread_mutex_unlock(&rec_system.data_mutex);
        return;
    }

    // Initialiser le graphe bipartite
    b_graph_t graph;
    init_graph(&graph, rec_system.num_users, rec_system.num_items);
    
    // Ajouter les interactions utilisateur-item
    for (int i = 0; i < rec_system.num_ratings; i++) {
        rating_t r = rec_system.ratings[i];
        if (r.rating > 0.0) { // Considérer seulement les ratings positifs
            add_interaction(&graph, r.user_id, r.item_id);
        }
    }

    // Exécuter l'algorithme PageRank
    run_pagerank(&graph);

    // Créer un tableau pour les scores des items
    ItemScore items[MAX_ITEMS];
    int count = 0;
    
    // Collecter les items non notés par l'utilisateur
    for (int item_id = 0; item_id < rec_system.num_items; item_id++) {
        // Vérifier si l'utilisateur n'a pas noté cet item
        if (rec_system.user_item_matrix[user_id][item_id] < 0) {
            items[count].item_id = item_id;
            items[count].score = graph.pr[graph.num_users + item_id];
            count++;
        }
    }

    // Trier les items par score décroissant (tri par sélection)
    for (int i = 0; i < count - 1; i++) {
        int max_idx = i;
        for (int j = i + 1; j < count; j++) {
            if (items[j].score > items[max_idx].score) {
                max_idx = j;
            }
        }
        if (max_idx != i) {
            ItemScore temp = items[i];
            items[i] = items[max_idx];
            items[max_idx] = temp;
        }
    }

    // Sélectionner les top-N recommandations
    int recommendations = (max_results < count) ? max_results : count;
    for (int i = 0; i < recommendations; i++) {
        results[*num_results].item_id = items[i].item_id;
        results[*num_results].category_id = -1; // À remplir si disponible
        results[*num_results].predicted_rating = items[i].score;
        (*num_results)++;
    }

    pthread_mutex_unlock(&rec_system.data_mutex);
}