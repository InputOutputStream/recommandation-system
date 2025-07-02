#ifndef SERVER_H
#define SERVER_H

#include "header.h"

// Server function declarations
int start_reco_server();
void *accept_connections(void *arg);
void *handle_client(void *arg);

// Client management functions
void remove_client(client_t *client);

// Utility functions
void init_server();
void cleanup_server();
void log_message(const char *format, ...);
void format_recommendation_response(recommendation_request_t* req, recommendation_result_t* results, 
                                  int num_results, char* response);
// Global recommendation system
extern recommendation_system_t rec_system;

// Recommendation system functions
void init_recommendation_system();
void load_ratings_data(const char* filename);
int add_rating(int user_id, int item_id, int category_id, float rating);
void get_recommendations(recommendation_request_t* request, recommendation_result_t* results, int* num_results);

// Algorithm implementations
void knn_recommendation(int user_id, int k, recommendation_result_t* results, int* num_results, int max_results);
void matrix_factorization_recommendation(int user_id, recommendation_result_t* results, int* num_results, int max_results);
void graph_recommendation(int user_id, recommendation_result_t* results, int* num_results, int max_results);

// Global variables (extern declarations)
extern client_t clients[MAX_CLIENT];
extern int client_count;
extern pthread_mutex_t clients_mutex;
extern int server_running;

#endif // SERVER_H