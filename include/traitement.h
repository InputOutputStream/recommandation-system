#ifndef TRAITEMENT_H
#define TRAITEMENT_H

#include "header.h"
#include <ndmath/ndarray.h>

extern date_t format_seconds_to_date(double seconds);
extern bool is_less(date_t d1, date_t d2);
extern bool is_greater(date_t d1, date_t d2);
extern bool is_equal(date_t d1, date_t d2);
extern bool is_less_equal(date_t d1, date_t d2);
extern bool is_greater_equal(date_t d1, date_t d2);
extern bool is_valid_date(date_t date);
extern ndarray_t load_timed_data(char *path, date_t start, date_t stop);
extern size_t get_count_safe(ndarray_t array, double value, size_t index, bool is_row);
extern ndarray_t nettoyer_transaction(ndarray_t transactions, size_t minU, size_t minI);
extern void train_test_split(ndarray_t this, float share, int random_state);
extern ndarray_t get_timed_data_array(ndarray_t data, date_t start, date_t stop);
extern void train_test_split_with_files(ndarray_t this, float share, int random_state, 
                                 char *train_file, char *test_file);
extern void clean_files(char *test_file, char *train_file, char *result_file_path);
void all_treatment(date_t *d1, date_t *d2, char *data_file, size_t batch_size, 
                      size_t minU, size_t minI, float split, int random_state, char *train, char* test);
#endif