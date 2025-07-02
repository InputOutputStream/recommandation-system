#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ndmath/all.h>

#include <math.h>
#include "knn.h"

knn_t* init_knn(size_t k)
{
    knn_t *KNN = calloc(1, sizeof(knn_t));
    if (KNN == NULL) {
        return NULL;
    }
    KNN->k = k;  
    KNN->X = NULL;
    KNN->y = NULL;
    return KNN; 
}


knn_t * fitx(knn_t *model, ndarray_t X)
{
    if (model == NULL) {
        return NULL;
    }
    
    model->y = malloc(sizeof(ndarray_t));
    
    if (model->y == NULL) {
        return NULL;
    }
    
    *model->y = copy(&X);
    
    return model;
}

knn_t * fity(knn_t *model, ndarray_t y)
{
    if (model == NULL) {
        return NULL;
    }
    
    model->y = malloc(sizeof(ndarray_t));
    
    if (model->y == NULL) {
        return NULL;
    }
    
    *model->y = copy(&y);
    
    return model;
}

knn_t * fit(knn_t *model, ndarray_t X, ndarray_t y)
{
    model = fitx(model, X);
    model = fitx(model, y);
    return model;
}


// Calculate Pearson correlation coefficient between two users/items
double pearson_correlation(ndarray_t user1, ndarray_t user2)
{
    int n = user1.shape[1]; // number of items
    double sum_x = 0.0, sum_y = 0.0;
    double sum_x2 = 0.0, sum_y2 = 0.0;
    double sum_xy = 0.0;
    int count = 0;
    
    // Calculate sums for items that both users have rated (non-zero values)
    for (int i = 0; i < n; i++) {
        double x = get(&user1, 0, i);
        double y = get(&user2, 0, i);
        
        // Only consider items that both users have rated (assuming 0 means not rated)
        if (x > 0.001 && y > 0.001) { // Use small epsilon instead of exact 0.0
            sum_x += x;
            sum_y += y;
            sum_x2 += x * x;
            sum_y2 += y * y;
            sum_xy += x * y;
            count++;
        }
    }
    
    // Need at least 1 common rating to calculate correlation (relaxed requirement)
    if (count < 1) {
        return 0.0; // No correlation if no common ratings
    }
    
    if (count == 1) {
        return 0.1;
    }
    
    // Calculate means
    double mean_x = sum_x / count;
    double mean_y = sum_y / count;
    
    // Calculate Pearson correlation coefficient
    double numerator = sum_xy - count * mean_x * mean_y;
    double denominator_x = sum_x2 - count * mean_x * mean_x;
    double denominator_y = sum_y2 - count * mean_y * mean_y;
    
    double denominator = sqrt(denominator_x * denominator_y);
    
    if (denominator < 0.001) { 
        return 0.1; 
    }
    
    double correlation = numerator / denominator;
    
    // Clamp correlation between -1 and 1
    if (correlation > 1.0) correlation = 1.0;
    if (correlation < -1.0) correlation = -1.0;
    
    return correlation;
}

double pearson_correlation_centered(ndarray_t user1, ndarray_t user2)
{
    int n = user1.shape[1];
    double sum_numerator = 0.0;
    double sum_denom_x = 0.0;
    double sum_denom_y = 0.0;
    int count = 0;
    
    // First pass: calculate means for commonly rated items
    double sum_x = 0.0, sum_y = 0.0;
    for (int i = 0; i < n; i++) {
        double x = get(&user1, 0, i);
        double y = get(&user2, 0, i);
        
        if (x > 0.001 && y > 0.001) { // Use epsilon comparison
            sum_x += x;
            sum_y += y;
            count++;
        }
    }
    
    if (count < 1) {
        return 0.0;
    }
    
    // If only one common rating, return small positive correlation
    if (count == 1) {
        return 0.2;
    }
    
    double mean_x = sum_x / count;
    double mean_y = sum_y / count;
    
    // Second pass: calculate correlation using mean-centered values
    for (int i = 0; i < n; i++) {
        double x = get(&user1, 0, i);
        double y = get(&user2, 0, i);
        
        if (x > 0.001 && y > 0.001) {
            double centered_x = x - mean_x;
            double centered_y = y - mean_y;
            
            sum_numerator += centered_x * centered_y;
            sum_denom_x += centered_x * centered_x;
            sum_denom_y += centered_y * centered_y;
        }
    }
    
    double denominator = sqrt(sum_denom_x * sum_denom_y);
    
    if (denominator < 0.001) {
        return 0.2; // Return small positive correlation for edge cases
    }
    
    double correlation = sum_numerator / denominator;
    
    // Clamp correlation
    if (correlation > 1.0) correlation = 1.0;
    if (correlation < -1.0) correlation = -1.0;
    
    return correlation;
}

void swap_int(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void swap_double(double *a, double *b)
{
    double temp = *a;
    *a = *b;
    *b = temp;
}

int *arg_bubble_sort_desc(double *arr, int n)
{
    int *arg = calloc(n, sizeof(int));
    if (arg == NULL) {
        return NULL;
    }
    
    for(int i = 0; i < n; i++) {
        arg[i] = i;
    }

    for(int i = 0; i < n - 1; i++) {
        for(int j = 0; j < n - 1 - i; j++) {
            if(arr[j] < arr[j + 1]) { // Changed to descending order
                swap_int(&arg[j], &arg[j + 1]);
                swap_double(&arr[j], &arr[j + 1]);
            }
        }    
    }

    return arg;
}

// Predict rating for a specific item using collaborative filtering
double predict_rating(knn_t *model, int user_idx, int item_idx)
{
    if (model == NULL || model->X == NULL) {
        return 0.0;
    }
    
    int n_users = model->X->shape[0];
    ndarray_t target_user = row_index(model->X, user_idx);
    
    // Calculate correlations with all other users
    double *correlations = malloc(n_users * sizeof(double));
    if (correlations == NULL) {
        return 0.0;
    }
    
    for (int i = 0; i < n_users; i++) {
        if (i == user_idx) {
            correlations[i] = -2.0; // Exclude self with very low value
        } else {
            ndarray_t other_user = row_index(model->X, i);
            correlations[i] = pearson_correlation_centered(target_user, other_user);
        }
    }
    
    // Get k most similar users (highest correlation)
    int *similar_users = arg_bubble_sort_desc(correlations, n_users);
    if (similar_users == NULL) {
        free(correlations);
        return 0.0;
    }
    
    // Calculate weighted average rating
    double weighted_sum = 0.0;
    double correlation_sum = 0.0;
    int count = 0;
    
    // Try to use users with positive correlation first
    for (size_t i = 0; i < model->k && i < n_users; i++) {
        int similar_user_idx = similar_users[i];
        
        // Skip if it's the same user
        if (similar_user_idx == user_idx) {
            continue;
        }
        
        double correlation = correlations[similar_user_idx];
        
        // Get the rating of this similar user for the target item
        double rating = get(model->X, similar_user_idx, item_idx);
        
        // Only consider users who have rated this item
        if (rating > 0.001) {
            // Use absolute correlation to weight ratings
            double abs_correlation = fabs(correlation);
            if (abs_correlation > 0.001) { // Even small correlations can be useful
                weighted_sum += abs_correlation * rating;
                correlation_sum += abs_correlation;
                count++;
            }
        }
    }
    
    // If no similar users found with positive correlation, try global average
    if (correlation_sum < 0.001 || count == 0) {
        // Calculate global average for this item
        double item_sum = 0.0;
        int item_count = 0;
        
        for (int u = 0; u < n_users; u++) {
            double rating = get(model->X, u, item_idx);
            if (rating > 0.001) {
                item_sum += rating;
                item_count++;
            }
        }
        
        if (item_count > 0) {
            double item_avg = item_sum / item_count;
            free(correlations);
            free(similar_users);
            return item_avg;
        }
        
        // Last resort: use user's average rating
        double user_sum = 0.0;
        int user_count = 0;
        
        for (size_t it = 0; it < model->X->shape[1]; it++) {
            double rating = get(model->X, user_idx, it);
            if (rating > 0.001) {
                user_sum += rating;
                user_count++;
            }
        }
        
        free(correlations);
        free(similar_users);
        return user_count > 0 ? user_sum / user_count : 2.5; // Default to middle rating
    }
    
    free(correlations);
    free(similar_users);
    
    return weighted_sum / correlation_sum;
}

// Modified predict function for collaborative filtering
ndarray_t predict(knn_t *model, ndarray_t user_item_pairs)
{
    int n_predictions = user_item_pairs.shape[0];
    ndarray_t predictions = array(n_predictions, 1);
    
    for (int i = 0; i < n_predictions; i++) {
        int user_idx = (int)get(&user_item_pairs, i, 0);
        int item_idx = (int)get(&user_item_pairs, i, 1);
        
        double predicted_rating = predict_rating(model, user_idx, item_idx);
        set(&predictions, i, 0, predicted_rating);
    }
    
    return predictions;
}

// Predict ratings for all unrated items for a specific user
ndarray_t predict_all_items(knn_t *model, int user_idx)
{
    if (model == NULL || model->X == NULL) {
        ndarray_t empty = {0};
        return empty;
    }
    
    int n_items = model->X->shape[1];
    ndarray_t predictions = array(1, n_items);
    
    for (int item_idx = 0; item_idx < n_items; item_idx++) {
        double existing_rating = get(model->X, user_idx, item_idx);
        
        if (existing_rating == 0.0) {
            double predicted_rating = predict_rating(model, user_idx, item_idx);
            set(&predictions, 0, item_idx, predicted_rating);
        } else {
            set(&predictions, 0, item_idx, existing_rating);
        }
    }
    
    return predictions;
}

void free_knn(knn_t *model) {
    if (model) {
        if (model->X) {
            free_array(model->X);
        }
        if (model->y) {
            free_array(model->y);
        }
        free(model);
    }
}