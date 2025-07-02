#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ndmath/all.h>
#include <math.h>
#include <time.h>

#include "knn.h"

int main() {
    printf("Pearson Correlation KNN Collaborative Filtering Test Suite\n");
    printf("=========================================================\n\n");
    
    // Test 1: Basic functionality with small dataset
    printf("=== Test 1: Basic Functionality ===\n");
    ndarray_t small_ratings = generate_realistic_ratings(8, 6, 42);
    print_rating_matrix(small_ratings, 8, 6);
    printf("Sparsity: %.3f\n\n", calculate_sparsity(small_ratings));
    
    // Initialize model
    knn_t *model = init_knn(3);
    fitx(model, small_ratings); // y not needed for collaborative filtering
    
    // Test individual predictions
    printf("Individual Predictions (with debug):\n");
    for (int user = 0; user < 3; user++) {
        for (int item = 0; item < 3; item++) {
            double original = get(&small_ratings, user, item);
            if (original == 0.0) { // Only predict unrated items
                printf("\n--- Predicting User %d, Item %d ---\n", user, item);
                double predicted = predict_rating(model, user, item);
                printf("Final result: User %d, Item %d: Predicted %.3f\n", user, item, predicted);
            }
        }
    }
    printf("\n");
    
    free_knn(model);
    
    // Test 2: Different k values
    printf("=== Test 2: K-Value Comparison ===\n");
    ndarray_t medium_ratings = generate_realistic_ratings(20, 15, 123);
    ndarray_t original_ratings = copy(&medium_ratings);
    ndarray_t test_ratings = create_test_set(&medium_ratings, 0.2, 456);
    
    printf("Training set sparsity: %.3f\n", calculate_sparsity(medium_ratings));
    
    int k_values[] = {1, 3, 5, 7, 10};
    int n_k = sizeof(k_values) / sizeof(k_values[0]);
    
    printf("K-Value\tMAE\tRMSE\n");
    printf("-------\t---\t----\n");
    
    for (int i = 0; i < n_k; i++) {
        knn_t *test_model = init_knn(k_values[i]);
        fitx(test_model, medium_ratings);
        
        // Predict test ratings
        ndarray_t predictions = copy(&medium_ratings);
        for (size_t u = 0; u < medium_ratings.shape[0]; u++) {
            for (size_t it = 0; it < medium_ratings.shape[1]; it++) {
                if (get(&medium_ratings, u, it) == 0.0 && get(&test_ratings, u, it) != 0.0) {
                    double pred = predict_rating(test_model, u, it);
                    set(&predictions, u, it, pred);
                }
            }
        }
        
        double mae = calculate_mae(test_ratings, predictions);
        double rmse = calculate_rmse(test_ratings, predictions);
        
        printf("%d\t%.3f\t%.3f\n", k_values[i], mae, rmse);
        
        free_knn(test_model);
    }
    printf("\n");
    
    // Test 3: Performance with different dataset sizes
    printf("=== Test 3: Performance Analysis ===\n");
    int sizes[] = {10, 20, 50, 100};
    int items[] = {8, 15, 30, 50};
    int n_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("Users\tItems\tSparsity\tTime(s)\tMAE\n");
    printf("-----\t-----\t--------\t-------\t---\n");
    
    for (int i = 0; i < n_sizes; i++) {
        clock_t start, end;
        
        // Generate dataset
        ndarray_t perf_ratings = generate_realistic_ratings(sizes[i], items[i], 789 + i);
        ndarray_t perf_original = copy(&perf_ratings);
        ndarray_t perf_test = create_test_set(&perf_ratings, 0.15, 987 + i);
        
        double sparsity = calculate_sparsity(perf_ratings);
        
        // Time the prediction process
        start = clock();
        
        knn_t *perf_model = init_knn(5);
        fitx(perf_model, perf_ratings);
        
        // Make predictions for test set
        ndarray_t perf_predictions = copy(&perf_ratings);
        int prediction_count = 0;
        
        for (int u = 0; u < sizes[i]; u++) {
            for (int it = 0; it < items[i]; it++) {
                if (get(&perf_ratings, u, it) == 0.0 && get(&perf_test, u, it) != 0.0) {
                    double pred = predict_rating(perf_model, u, it);
                    set(&perf_predictions, u, it, pred);
                    prediction_count++;
                }
            }
        }
        
        end = clock();
        double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
        
        double mae = calculate_mae(perf_test, perf_predictions);
        
        printf("%d\t%d\t%.3f\t\t%.4f\t%.3f\n", sizes[i], items[i], sparsity, time_taken, mae);
        
        free_knn(perf_model);
    }
    
    // Test 4: Recommendation demonstration
    printf("\n=== Test 4: Recommendation Demo ===\n");
    ndarray_t demo_ratings = generate_realistic_ratings(5, 8, 999);
    
    printf("Original ratings:\n");
    print_rating_matrix(demo_ratings, 5, 8);
    
    knn_t *demo_model = init_knn(3);
    fitx(demo_model, demo_ratings);
    
    // Generate recommendations for user 0
    printf("Recommendations for User 0:\n");
    printf("Item\tOriginal\tPredicted\n");
    printf("----\t--------\t---------\n");
    
    for (size_t item = 0; item < demo_ratings.shape[1]; item++) {
        double original = get(&demo_ratings, 0, item);
        if (original == 0.0) {
            double predicted = predict_rating(demo_model, 0, item);
            printf("%zu\t-\t\t%.3f\n", item, predicted);
        } else {
            printf("%zu\t%.1f\t\t-\n", item, original);
        }
    }
    
    free_knn(demo_model);
    
    printf("\n=== Analysis Complete ===\n");
    return 0;
}