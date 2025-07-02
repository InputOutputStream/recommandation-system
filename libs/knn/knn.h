#ifndef KNN_H
#define KNN_H

#include <ndmath/ndarray.h>

typedef struct KNN{
    size_t k;
    ndarray_t *X;
    ndarray_t *y;
}knn_t;


extern knn_t * init_knn(size_t k);

extern knn_t * fitx(knn_t *model, ndarray_t X);
extern knn_t * fity(knn_t *model, ndarray_t y);

extern knn_t * fit(knn_t *model, ndarray_t X, ndarray_t y);

extern double pearson_correlation(ndarray_t user1, ndarray_t user2);
extern double pearson_correlation_centered(ndarray_t user1, ndarray_t user2);
extern int *arg_bubble_sort_desc(double *arr, int n);
extern double predict_rating(knn_t *model, int user_idx, int item_idx);
extern ndarray_t predict(knn_t *model, ndarray_t user_item_pairs);

extern ndarray_t predict_all_items(knn_t *model, int user_idx);
extern void free_knn(knn_t *model);
extern ndarray_t generate_iris_like_data(int n_samples);
extern ndarray_t generate_labels(int n_samples);
extern void _train_test_split(ndarray_t X, ndarray_t y, double test_size,
                     ndarray_t *X_train, ndarray_t *y_train,
                     ndarray_t *X_test, ndarray_t *y_test);
extern ndarray_t generate_rating_matrix(int n_users, int n_items, double sparsity, int seed);

extern double calculate_accuracy(ndarray_t y_true, ndarray_t y_pred);
extern void print_confusion_matrix(ndarray_t y_true, ndarray_t y_pred, int n_classes);
extern ndarray_t generate_balanced_data(int n_samples, int seed);
extern void shuffle_data(ndarray_t *X, ndarray_t *y);
extern void standardize_features(ndarray_t *X);
extern double cross_validate_knn(ndarray_t X, ndarray_t y, int k, int n_folds);
extern void analyze_class_distribution(ndarray_t y, const char* dataset_name);

extern ndarray_t generate_realistic_ratings(int n_users, int n_items, int seed);
extern double calculate_sparsity(ndarray_t ratings);
extern void print_rating_matrix(ndarray_t ratings, int max_users, int max_items);
extern double calculate_mae(ndarray_t actual, ndarray_t predicted);
extern double calculate_rmse(ndarray_t actual, ndarray_t predicted);
extern ndarray_t create_test_set(ndarray_t *train_ratings, double test_ratio, int seed);
extern ndarray_t generate_rating_matrix(int n_users, int n_items, double sparsity, int seed);

#endif