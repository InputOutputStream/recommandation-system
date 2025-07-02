
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
#define MAX_USERS 1000
#define MAX_ITEMS 1000
#define MAX_RECOMMENDATIONS 20

typedef struct date
{
    size_t day;
    size_t month; 
    size_t year;
}date_t;

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
    int user_id;
    int item_id;
    int category_id;
    float rating;
    time_t timestamp;
} rating_t;

// Recommendation request structure
typedef struct {
    int user_id;
    recommendation_algo_t algorithm;
    int num_recommendations;
    int category_filter;
} recommendation_request_t;

// Recommendation result structure
typedef struct {
    int item_id;
    int category_id;
    float predicted_rating;
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
    int num_ratings;
    int user_item_matrix[MAX_USERS][MAX_ITEMS];
    int num_users;
    int num_items;
    pthread_mutex_t data_mutex;
} recommendation_system_t;

#endif // HEADER_H
