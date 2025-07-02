#ifndef MF_H
#define MF_H

#include <ndmath/array.h>

// Structure pour les transactions (compatible avec le format de traitement.c)
typedef struct {
    size_t user_id;
    size_t item_id;
    double rating;
    double timestamp;
} Transaction;

// Fonction principale de factorisation matricielle
extern ndarray_t MF(const char* train_data, size_t batch_size, size_t k, double alpha, double lambda, size_t epochs);

// Fonction de prédiction pour toutes les données de test
extern ndarray_t Predict_all_MF(ndarray_t full_matrix, size_t batch_size, const char* test_data);

// Fonction utilitaire pour convertir ndarray en transactions
extern Transaction* ndarray_to_transactions(ndarray_t data, size_t* num_transactions);

#endif