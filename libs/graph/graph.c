#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "graph.h"

// Initialize the graph
void init_graph(b_graph_t* g, int users, int items) {
    g->num_users = users;
    g->num_items = items;
    
    // Initialize adjacency matrix to 0
    for(int i = 0; i < users; i++) {
        for(int j = 0; j < items; j++) {
            g->user_item[i][j] = 0;
        }
    }
    
    // Initialize PageRank scores
    double initial_pr = 1.0 / (users + items);
    for(int i = 0; i < users + items; i++) {
        g->pr[i] = initial_pr;
        g->pr_new[i] = 0.0;
    }
}

// Add user-item interaction
void add_interaction(b_graph_t* g, int user, int item) {
    if(user < g->num_users && item < g->num_items) {
        g->user_item[user][item] = 1;
    }
}

// Get out-degree for a node
int get_out_degree(b_graph_t* g, int node) {
    int degree = 0;
    
    if(node < g->num_users) { // User node
        for(int j = 0; j < g->num_items; j++) {
            degree += g->user_item[node][j];
        }
    } else { // Item node
        int item_id = node - g->num_users;
        for(int i = 0; i < g->num_users; i++) {
            degree += g->user_item[i][item_id];
        }
    }
    
    return degree;
}

// PageRank iteration
void pagerank_iteration(b_graph_t* g) {
    int total_nodes = g->num_users + g->num_items;
    
    // Reset new PageRank values
    for(int i = 0; i < total_nodes; i++) {
        g->pr_new[i] = (1.0 - DAMPING_FACTOR) / total_nodes;
    }
    
    // Calculate PageRank for each node
    for(int i = 0; i < total_nodes; i++) {
        int out_degree = get_out_degree(g, i);
        
        if(out_degree > 0) {
            double contribution = DAMPING_FACTOR * g->pr[i] / out_degree;
            
            if(i < g->num_users) { // User node - contribute to connected items
                for(int j = 0; j < g->num_items; j++) {
                    if(g->user_item[i][j] == 1) {
                        g->pr_new[g->num_users + j] += contribution;
                    }
                }
            } else { // Item node - contribute to connected users
                int item_id = i - g->num_users;
                for(int j = 0; j < g->num_users; j++) {
                    if(g->user_item[j][item_id] == 1) {
                        g->pr_new[j] += contribution;
                    }
                }
            }
        }
    }
    
    // Update PageRank values
    for(int i = 0; i < total_nodes; i++) {
        g->pr[i] = g->pr_new[i];
    }
}

// Check convergence
int has_converged(b_graph_t* g) {
    int total_nodes = g->num_users + g->num_items;
    
    for(int i = 0; i < total_nodes; i++) {
        if(fabs(g->pr[i] - g->pr_new[i]) > EPSILON) {
            return 0;
        }
    }
    return 1;
}

// Run PageRank algorithm
void run_pagerank(b_graph_t* g) {
    printf("Running PageRank algorithm...\n");
    
    for(int iter = 0; iter < MAX_ITER; iter++) {
        pagerank_iteration(g);
        
        if(iter > 0 && has_converged(g)) {
            printf("Converged after %d iterations\n", iter + 1);
            return;
        }
    }
    
    printf("Maximum iterations reached\n");
}

// Get top-N item recommendations for a user
void get_recommendations(b_graph_t* g, int user_id, int top_n) {
    printf("\nTop-%d recommendations for User %d:\n", top_n, user_id);
    
    // Create array of item scores
    typedef struct {
        int item_id;
        double score;
    } ItemScore;
    
    ItemScore items[MAX_ITEMS];
    int count = 0;
    
    // Collect items not already interacted with
    for(int i = 0; i < g->num_items; i++) {
        if(g->user_item[user_id][i] == 0) { // Not already interacted
            items[count].item_id = i;
            items[count].score = g->pr[g->num_users + i];
            count++;
        }
    }
    
    // Sort items by PageRank score (descending)
    for(int i = 0; i < count - 1; i++) {
        for(int j = i + 1; j < count; j++) {
            if(items[j].score > items[i].score) {
                ItemScore temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }
        }
    }
    
    // Print top-N recommendations
    int recommendations = (top_n < count) ? top_n : count;
    for(int i = 0; i < recommendations; i++) {
        printf("Item %d (score: %.6f)\n", items[i].item_id, items[i].score);
    }
}

// Print adjacency matrix
void print_adjacency_matrix(b_graph_t* g) {
    printf("\nAdjacency Matrix:\n");
    printf("    ");
    for(int j = 0; j < g->num_items; j++) {
        printf("I%d ", j);
    }
    printf("\n");
    
    for(int i = 0; i < g->num_users; i++) {
        printf("U%d  ", i);
        for(int j = 0; j < g->num_items; j++) {
            printf("%d  ", g->user_item[i][j]);
        }
        printf("\n");
    }
}

// Print PageRank scores
void print_pagerank_scores(b_graph_t* g) {
    printf("\nPageRank Scores:\n");
    printf("Users:\n");
    for(int i = 0; i < g->num_users; i++) {
        printf("U%d: %.6f\n", i, g->pr[i]);
    }
    
    printf("Items:\n");
    for(int i = 0; i < g->num_items; i++) {
        printf("I%d: %.6f\n", i, g->pr[g->num_users + i]);
    }
}

int main() {
    b_graph_t graph;
    
    // Initialize graph with 4 users and 5 items (from your document)
    init_graph(&graph, 4, 5);
    
    // Add interactions based on your adjacency matrix
    add_interaction(&graph, 0, 0); // U1-I1
    add_interaction(&graph, 0, 2); // U1-I3
    add_interaction(&graph, 1, 1); // U2-I2
    add_interaction(&graph, 1, 4); // U2-I5
    add_interaction(&graph, 2, 1); // U3-I2
    add_interaction(&graph, 2, 3); // U3-I4
    add_interaction(&graph, 3, 2); // U4-I3
    
    // Print initial state
    print_adjacency_matrix(&graph);
    
    // Run PageRank algorithm
    run_pagerank(&graph);
    
    // Print final PageRank scores
    print_pagerank_scores(&graph);
    
    // Get recommendations for each user
    for(int user = 0; user < graph.num_users; user++) {
        get_graph_recommendations(&graph, user, 3);
    }
    
    return 0;
}