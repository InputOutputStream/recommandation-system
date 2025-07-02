#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ndmath/all.h>
#include <math.h>
#include <time.h>

#include "knn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ndmath/all.h>
#include <math.h>
#include <time.h>

#include "knn.h"

// Generate synthetic user-item rating matrix
ndarray_t generate_rating_matrix(int n_users, int n_items, double sparsity, int seed) {
    srand(seed);
    ndarray_t ratings = array(n_users, n_items);
    
    // Initialize all ratings to 0 (unrated)
    for (int i = 0; i < n_users; i++) {
        for (int j = 0; j < n_items; j++) {
            set(&ratings, i, j, 0.0);
        }
    }
    
    // Fill with ratings based on sparsity (percentage of rated items)
    int total_ratings = (int)(n_users * n_items * sparsity);
    
    for (int r = 0; r < total_ratings; r++) {
        int user = rand() % n_users;
        int item = rand() % n_items;
        
        // Generate rating between 1-5
        double rating = 1.0 + (rand() % 5);
        set(&ratings, user, item, rating);
    }
    
    return ratings;
}

// Generate more realistic rating matrix with user preferences
ndarray_t generate_realistic_ratings(int n_users, int n_items, int seed) {
    srand(seed);
    ndarray_t ratings = array(n_users, n_items);
    
    // Initialize to 0
    for (int i = 0; i < n_users; i++) {
        for (int j = 0; j < n_items; j++) {
            set(&ratings, i, j, 0.0);
        }
    }
    
    // Create user groups with similar preferences and more overlap
    for (int user = 0; user < n_users; user++) {
        int user_type = user % 3; // 3 different user types
        double base_rating = 2.0 + user_type; // Base preference
        
        // Each user rates 50-70% of items (increased overlap)
        int min_ratings = (int)(n_items * 0.5);
        int max_ratings = (int)(n_items * 0.7);
        int n_ratings = min_ratings + (rand() % (max_ratings - min_ratings + 1));
        
        // Create a list of items to rate
        int *items_to_rate = malloc(n_items * sizeof(int));
        for (int i = 0; i < n_items; i++) {
            items_to_rate[i] = i;
        }
        
        // Shuffle the items
        for (int i = n_items - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int temp = items_to_rate[i];
            items_to_rate[i] = items_to_rate[j];
            items_to_rate[j] = temp;
        }
        
        // Rate the first n_ratings items
        for (int r = 0; r < n_ratings && r < n_items; r++) {
            int item = items_to_rate[r];
            
            // Generate rating with some noise but based on user type
            double noise = (rand() % 21 - 10) / 20.0; // -0.5 to +0.5
            double rating = base_rating + noise;
            
            // Add some item-specific bias
            if (item % 3 == user_type) {
                rating += 0.5; // Users prefer certain types of items
            }
            
            // Clamp to 1-5 range
            if (rating < 1.0) rating = 1.0;
            if (rating > 5.0) rating = 5.0;
            
            set(&ratings, user, item, rating);
        }
        
        free(items_to_rate);
    }
    
    return ratings;
}

// Calculate sparsity of rating matrix
double calculate_sparsity(ndarray_t ratings) {
    int total = ratings.shape[0] * ratings.shape[1];
    int rated = 0;
    
    for (size_t i = 0; i < ratings.shape[0]; i++) {
        for (size_t j = 0; j < ratings.shape[1]; j++) {
            if (get(&ratings, i, j) != 0.0) {
                rated++;
            }
        }
    }
    
    return (double)rated / total;
}

// Print rating matrix (first few users/items)
void print_rating_matrix(ndarray_t ratings, int max_users, int max_items) {
    printf("Rating Matrix (first %dx%d):\n", max_users, max_items);
    printf("User\\Item ");
    
    for (int j = 0; j < max_items && j < ratings.shape[1]; j++) {
        printf("%8d", j);
    }
    printf("\n");
    
    for (size_t i = 0; i < max_users && i < ratings.shape[0]; i++) {
        printf("%8zu ", i);
        for (size_t j = 0; j < max_items && j < ratings.shape[1]; j++) {
            double rating = get(&ratings, i, j);
            if (rating == 0.0) {
                printf("%8s", "-");
            } else {
                printf("%8.1f", rating);
            }
        }
        printf("\n");
    }
    printf("\n");
}

// Calculate Mean Absolute Error
double calculate_mae(ndarray_t actual, ndarray_t predicted) {
    double sum_error = 0.0;
    int count = 0;
    
    for (size_t i = 0; i < actual.shape[0]; i++) {
        for (size_t j = 0; j < actual.shape[1]; j++) {
            double actual_val = get(&actual, i, j);
            double pred_val = get(&predicted, i, j);
            
            // Only calculate error for rated items
            if (actual_val != 0.0 && pred_val != 0.0) {
                sum_error += fabs(actual_val - pred_val);
                count++;
            }
        }
    }
    
    return count > 0 ? sum_error / count : 0.0;
}

// Calculate Root Mean Square Error
double calculate_rmse(ndarray_t actual, ndarray_t predicted) {
    double sum_squared_error = 0.0;
    int count = 0;
    
    for (size_t i = 0; i < actual.shape[0]; i++) {
        for (size_t j = 0; j < actual.shape[1]; j++) {
            double actual_val = get(&actual, i, j);
            double pred_val = get(&predicted, i, j);
            
            if (actual_val != 0.0 && pred_val != 0.0) {
                double error = actual_val - pred_val;
                sum_squared_error += error * error;
                count++;
            }
        }
    }
    
    return count > 0 ? sqrt(sum_squared_error / count) : 0.0;
}

// Create test set by hiding some ratings
ndarray_t create_test_set(ndarray_t *train_ratings, double test_ratio, int seed) {
    srand(seed);
    ndarray_t test_data = copy(train_ratings);
    
    int n_users = train_ratings->shape[0];
    int n_items = train_ratings->shape[1];
    
    // Count total rated items
    int total_rated = 0;
    for (int i = 0; i < n_users; i++) {
        for (int j = 0; j < n_items; j++) {
            if (get(train_ratings, i, j) != 0.0) {
                total_rated++;
            }
        }
    }
    
    // Hide test_ratio of ratings
    int n_test = (int)(total_rated * test_ratio);
    int hidden = 0;
    
    while (hidden < n_test) {
        int user = rand() % n_users;
        int item = rand() % n_items;
        
        if (get(train_ratings, user, item) != 0.0) {
            set(train_ratings, user, item, 0.0); // Hide from training
            hidden++;
        }
    }
    
    return test_data; // Original ratings for testing
}


ndarray_t generate_balanced_data(int n_samples, int seed) {
    srand(seed);
    ndarray_t data = array(n_samples, 4);
    
    int samples_per_class = n_samples / 3;
    
    for(int i = 0; i < n_samples; i++) {
        int class_id = i / samples_per_class;
        if (class_id > 2) class_id = 2; // Handle remainder
        
        // Generate more realistic feature ranges
        switch(class_id) {
            case 0: // Class 0 - smaller values
                set(&data, i, 0, 4.0 + (rand() % 100) / 200.0);  // 4.0-4.5
                set(&data, i, 1, 3.0 + (rand() % 100) / 200.0);  // 3.0-3.5
                set(&data, i, 2, 1.0 + (rand() % 100) / 200.0);  // 1.0-1.5
                set(&data, i, 3, 0.1 + (rand() % 50) / 500.0);   // 0.1-0.2
                break;
            case 1: // Class 1 - medium values
                set(&data, i, 0, 5.5 + (rand() % 100) / 200.0);  // 5.5-6.0
                set(&data, i, 1, 2.5 + (rand() % 100) / 200.0);  // 2.5-3.0
                set(&data, i, 2, 3.5 + (rand() % 100) / 200.0);  // 3.5-4.0
                set(&data, i, 3, 1.0 + (rand() % 100) / 200.0);  // 1.0-1.5
                break;
            case 2: // Class 2 - larger values
                set(&data, i, 0, 6.0 + (rand() % 100) / 200.0);  // 6.0-6.5
                set(&data, i, 1, 2.8 + (rand() % 100) / 200.0);  // 2.8-3.3
                set(&data, i, 2, 5.0 + (rand() % 150) / 200.0);  // 5.0-5.75
                set(&data, i, 3, 1.8 + (rand() % 100) / 200.0);  // 1.8-2.3
                break;
        }
    }
    
    return data;
}

// Shuffle data for better train/test split
void shuffle_data(ndarray_t *X, ndarray_t *y) {
    int n = X->shape[0];
    int m = X->shape[1];
    
    for(int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        
        // Swap rows in X
        for(int k = 0; k < m; k++) {
            double temp = get(X, i, k);
            set(X, i, k, get(X, j, k));
            set(X, j, k, temp);
        }
        
        // Swap values in y
        double temp_y = get(y, i, 0);
        set(y, i, 0, get(y, j, 0));
        set(y, j, 0, temp_y);
    }
}

// Feature scaling (standardization)
void standardize_features(ndarray_t *X) {
    int n_samples = X->shape[0];
    int n_features = X->shape[1];
    
    for(int j = 0; j < n_features; j++) {
        // Calculate mean
        double mean = 0.0;
        for(int i = 0; i < n_samples; i++) {
            mean += get(X, i, j);
        }
        mean /= n_samples;
        
        // Calculate standard deviation
        double variance = 0.0;
        for(int i = 0; i < n_samples; i++) {
            double diff = get(X, i, j) - mean;
            variance += diff * diff;
        }
        double std_dev = sqrt(variance / n_samples);
        
        // Standardize
        if(std_dev > 1e-8) { // Avoid division by zero
            for(int i = 0; i < n_samples; i++) {
                double standardized = (get(X, i, j) - mean) / std_dev;
                set(X, i, j, standardized);
            }
        }
    }
}

// Cross-validation for better performance estimation
double cross_validate_knn(ndarray_t X, ndarray_t y, int k, int n_folds) {
    int n_samples = X.shape[0];
    int fold_size = n_samples / n_folds;
    double total_accuracy = 0.0;
    
    for(int fold = 0; fold < n_folds; fold++) {
        int test_start = fold * fold_size;
        int test_end = (fold == n_folds - 1) ? n_samples : (fold + 1) * fold_size;
        int test_size = test_end - test_start;
        int train_size = n_samples - test_size;
        
        // Create train and test sets
        ndarray_t X_train = array(train_size, X.shape[1]);
        ndarray_t y_train = array(train_size, 1);
        ndarray_t X_test = array(test_size, X.shape[1]);
        ndarray_t y_test = array(test_size, 1);
        
        // Fill training set
        int train_idx = 0;
        for(int i = 0; i < n_samples; i++) {
            if(i < test_start || i >= test_end) {
                for(size_t j = 0; j < X.shape[1]; j++) {
                    set(&X_train, train_idx, j, get(&X, i, j));
                }
                set(&y_train, train_idx, 0, get(&y, i, 0));
                train_idx++;
            }
        }
        
        // Fill test set
        int test_idx = 0;
        for(int i = test_start; i < test_end; i++) {
            for(size_t j = 0; j < X.shape[1]; j++) {
                set(&X_test, test_idx, j, get(&X, i, j));
            }
            set(&y_test, test_idx, 0, get(&y, i, 0));
            test_idx++;
        }
        
        // Train and test
        knn_t *model = init_knn(k);
        fit(model, X_train, y_train);
        
        ndarray_t y_pred = predict(model, X_test);
        
        // Calculate accuracy for this fold
        int correct = 0;
        for(int i = 0; i < test_size; i++) {
            if(get(&y_test, i, 0) == get(&y_pred, i, 0)) {
                correct++;
            }
        }
        
        double fold_accuracy = (double)correct / test_size;
        total_accuracy += fold_accuracy;
        
        printf("Fold %d: %.4f accuracy\n", fold + 1, fold_accuracy);
    }
    
    return total_accuracy / n_folds;
}

// Analyze class distribution
void analyze_class_distribution(ndarray_t y, const char* dataset_name) {
    printf("\n=== %s Class Distribution ===\n", dataset_name);
    
    int n_samples = y.shape[0];
    int class_counts[10] = {0}; // Assume max 10 classes
    int max_class = 0;
    
    for(int i = 0; i < n_samples; i++) {
        int class_val = (int)get(&y, i, 0);
        class_counts[class_val]++;
        if(class_val > max_class) max_class = class_val;
    }
    
    for(int i = 0; i <= max_class; i++) {
        printf("Class %d: %d samples (%.1f%%)\n", 
               i, class_counts[i], 100.0 * class_counts[i] / n_samples);
    }
}


// Test data generation functions
ndarray_t generate_iris_like_data(int n_samples) {
    srand(time(NULL));
    ndarray_t data = array(n_samples, 4);
    
    for(int i = 0; i < n_samples; i++) {
        // Generate features that somewhat mimic Iris dataset
        if(i < n_samples/3) {
            // Class 0 (Setosa-like)
            set(&data, i, 0, 4.5 + (rand() % 100) / 100.0);  // sepal_length
            set(&data, i, 1, 3.0 + (rand() % 100) / 100.0);  // sepal_width  
            set(&data, i, 2, 1.3 + (rand() % 50) / 100.0);   // petal_length
            set(&data, i, 3, 0.2 + (rand() % 30) / 100.0);   // petal_width
        }
        else if(i < 2*n_samples/3) {
            // Class 1 (Versicolor-like)
            set(&data, i, 0, 5.5 + (rand() % 100) / 100.0);
            set(&data, i, 1, 2.5 + (rand() % 100) / 100.0);
            set(&data, i, 2, 3.5 + (rand() % 100) / 100.0);
            set(&data, i, 3, 1.0 + (rand() % 80) / 100.0);
        }
        else {
            // Class 2 (Virginica-like)
            set(&data, i, 0, 6.0 + (rand() % 100) / 100.0);
            set(&data, i, 1, 2.8 + (rand() % 100) / 100.0);
            set(&data, i, 2, 5.0 + (rand() % 150) / 100.0);
            set(&data, i, 3, 1.8 + (rand() % 100) / 100.0);
        }
    }
    
    return data;
}

ndarray_t generate_labels(int n_samples) {
    ndarray_t labels = array(n_samples, 1);
    
    for(int i = 0; i < n_samples; i++) {
        if(i < n_samples/3) {
            set(&labels, i, 0, 0);  // Class 0
        }
        else if(i < 2*n_samples/3) {
            set(&labels, i, 0, 1);  // Class 1
        }
        else {
            set(&labels, i, 0, 2);  // Class 2
        }
    }
    
    return labels;
}

double calculate_accuracy(ndarray_t y_true, ndarray_t y_pred) {
    int correct = 0;
    int total = y_true.shape[0];
    
    for(int i = 0; i < total; i++) {
        if(get(&y_true, i, 0) == get(&y_pred, i, 0)) {
            correct++;
        }
    }
    
    return (double)correct / total;
}

void print_confusion_matrix(ndarray_t y_true, ndarray_t y_pred, int n_classes) {
    int confusion[n_classes][n_classes];
    
    // Initialize confusion matrix
    for(int i = 0; i < n_classes; i++) {
        for(int j = 0; j < n_classes; j++) {
            confusion[i][j] = 0;
        }
    }
    
    // Fill confusion matrix
    for(size_t i = 0; i < y_true.shape[0]; i++) {
        int true_label = (int)get(&y_true, i, 0);
        int pred_label = (int)get(&y_pred, i, 0);
        confusion[true_label][pred_label]++;
    }
    
    printf("\nConfusion Matrix:\n");
    printf("     ");
    for(int i = 0; i < n_classes; i++) {
        printf("%4d", i);
    }
    printf("\n");
    
    for(int i = 0; i < n_classes; i++) {
        printf("%4d ", i);
        for(int j = 0; j < n_classes; j++) {
            printf("%4d", confusion[i][j]);
        }
        printf("\n");
    }
}

