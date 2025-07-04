
#ifndef HEADER_H
#define HEADER_H

#include <pthread.h>
#include <netinet/in.h>
#include <time.h>

// Configuration constants
#define DEFAULT_PORT 8080
#define SERVER_IP "127.0.0.1"  // Add missing SERVER_IP
#define MAX_CLIENT 10
#define MAX_MESSAGE_LENGTH 1024
#define CLIENT_TIMEOUT 300
#define MAX_RATINGS 10000
#define MAX_USERS 9999
#define MAX_ITEMS 9999
#define MAX_RECOMMENDATIONS 20

#define DEFAULT_K 3 

typedef struct date
{
    int day;
    int month; 
    int year;
}date_t;

// Définition temporaire de la structure ItemScore
typedef struct {
    int item_id;
    double score;
} ItemScore;

// Message types
typedef enum {
    MSG_RECOMMENDATION,  
} message_type_t;

// Recommendation algorithms
typedef enum {
    ALGO_KNN = 1,
    ALGO_MF,
    ALGO_GRAPH
} recommendation_algo_t;

// Rating structure
typedef struct {
    long user_id;
    long item_id;
    long category_id;
    double rating;
    double timestamp;
} rating_t;

// Recommendation request structure
typedef struct {
    long user_id;
    recommendation_algo_t algorithm;
    long num_recommendations;
    long category_filter;
    int k;
} recommendation_request_t;

// Recommendation result structure
typedef struct {
    long item_id;
    long category_id;
    double predicted_rating;
} recommendation_result_t;

// Message structure
typedef struct {
    message_type_t type;
    char content[MAX_MESSAGE_LENGTH];
    time_t timestamp;
    recommendation_request_t rec_request;
} message_t;

// Fixed Client structure - unified fields
typedef struct {
    int socket_fd;                    // Unified socket field
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_id;
    int active;
    int running;
    pthread_t receive_thread;         // Unified thread field
    pthread_mutex_t client_mutex;     // Add mutex for thread safety
} client_t;

// Recommendation system data structures
typedef struct {
    rating_t ratings[MAX_RATINGS];
    long num_ratings;
    long user_item_matrix[MAX_USERS][MAX_ITEMS];
    long num_users;
    long num_items;
    pthread_mutex_t data_mutex;
} recommendation_system_t;

#endif // HEADER_H
