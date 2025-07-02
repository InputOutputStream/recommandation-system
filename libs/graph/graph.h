#ifndef GRAPH
#define GRAPH


#define MAX_USERS 100
#define MAX_ITEMS 100
#define MAX_ITER 50
#define DAMPING_FACTOR 0.85
#define EPSILON 1e-6

typedef struct BipartiteGraph {
    int num_users;
    int num_items;
    int user_item[MAX_USERS][MAX_ITEMS];  // Adjacency matrix
    double pr[MAX_USERS + MAX_ITEMS];     // PageRank scores
    double pr_new[MAX_USERS + MAX_ITEMS]; 
} b_graph_t;


void init_graph(b_graph_t* g, int users, int items);
void add_interaction(b_graph_t* g, int user, int item);
int get_out_degree(b_graph_t* g, int node);
void pagerank_iteration(b_graph_t* g);
int has_converged(b_graph_t* g);
void run_pagerank(b_graph_t* g);
void get_graph_recommendations(b_graph_t* g, int user_id, int top_n);
void print_adjacency_matrix(b_graph_t* g);
void print_pagerank_scores(b_graph_t* g);


#endif // !GRAPH