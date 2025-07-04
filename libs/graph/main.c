
#include <stdio.h>
#include "graph.h"

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