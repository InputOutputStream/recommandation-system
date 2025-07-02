#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ndmath/array.h>
#include <ndmath/helper.h>
#include <ndmath/operations.h>
#include <ndmath/io.h>
#include <math.h>

#include "mf.h"

// Fonction pour convertir ndarray en tableau de transactions
Transaction* ndarray_to_transactions(ndarray_t data, size_t* num_transactions) {
    if (!data.data || data.shape[0] == 0 || data.shape[1] < 3) {
        printf("Erreur: données invalides (shape: %zu x %zu)\n", data.shape[0], data.shape[1]);
        *num_transactions = 0;
        return NULL;
    }

    size_t count = data.shape[0];
    Transaction* transactions = malloc(count * sizeof(Transaction));
    if (!transactions) {
        printf("Erreur: échec de l'allocation pour les transactions\n");
        *num_transactions = 0;
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        // Colonnes: user_id, item_id, category_id, rating, timestamp
        // On ajuste pour index 0 (les données sont en index 1)
        transactions[i].user_id = (size_t)data.data[i][0] - 1;
        transactions[i].item_id = (size_t)data.data[i][1] - 1;
        transactions[i].rating = data.data[i][3]; // La note est en colonne 3
        transactions[i].timestamp = data.data[i][4]; // Timestamp est la colone 4
    }

    *num_transactions = count;
    printf("Nombre de transactions converties : %zu\n", count);
    return transactions;
}

ndarray_t MF(const char* train_data, size_t batch_size, size_t k, double alpha, double lambda, size_t epochs) {
    // Charger les données d'entraînement avec ndmath
    ndarray_t train_array = load_ndarray(train_data, batch_size);
    if (!train_array.data) {
        printf("Erreur: échec du chargement des données depuis %s\n", train_data);
        ndarray_t empty = {0};
        return empty;
    }
    
    printf("Données chargées: %zu lignes x %zu colonnes\n", train_array.shape[0], train_array.shape[1]);

    // Convertir en tableau de transactions
    size_t num_transactions;
    Transaction* transactions = ndarray_to_transactions(train_array, &num_transactions);
    if (!transactions) {
        printf("Erreur: échec de la conversion des transactions\n");
        free_array(&train_array);
        ndarray_t empty = {0};
        return empty;
    }

    // Trouver le nombre max d'utilisateurs et d'items
    size_t max_users = 0, max_items = 0;
    for (size_t i = 0; i < num_transactions; i++) {
        if (transactions[i].user_id + 1 > max_users) max_users = transactions[i].user_id + 1;
        if (transactions[i].item_id + 1 > max_items) max_items = transactions[i].item_id + 1;
    }
    printf("max_users: %zu, max_items: %zu, k: %zu\n", max_users, max_items, k);
    
    if (max_users == 0 || max_items == 0 || k == 0) {
        printf("Erreur: dimensions invalides (max_users=%zu, max_items=%zu, k=%zu)\n", max_users, max_items, k);
        free(transactions);
        free_array(&train_array);
        ndarray_t empty = {0};
        return empty;
    }

    // Initialiser les matrices U, V, O, P
    srand(time(NULL));
    printf("Création de U (%zu x %zu)\n", max_users, k);
    ndarray_t U = array(max_users, k); // Facteurs latents utilisateurs
    printf("Création de V (%zu x %zu)\n", max_items, k);
    ndarray_t V = array(max_items, k); // Facteurs latents items
    printf("Création de O (%zu x 1)\n", max_users);
    ndarray_t O = array(max_users, 1); // Biais utilisateurs
    printf("Création de P (%zu x 1)\n", max_items);
    ndarray_t P = array(max_items, 1); // Biais items
    
    if (!U.data || !V.data || !O.data || !P.data) {
        printf("Erreur: échec de l'allocation d'une matrice (U=%p, V=%p, O=%p, P=%p)\n",
               (void*)U.data, (void*)V.data, (void*)O.data, (void*)P.data);
        clean(&U, &V, &O, &P, NULL);
        free(transactions);
        free_array(&train_array);
        ndarray_t empty = {0};
        return empty;
    }

    // Initialiser U et V avec des valeurs aléatoires entre 0 et 0.1
    for (size_t i = 0; i < max_users; i++) {
        for (size_t j = 0; j < k; j++) {
            U.data[i][j] = ((double)rand() / RAND_MAX) * 0.1;
        }
    }
    for (size_t i = 0; i < max_items; i++) {
        for (size_t j = 0; j < k; j++) {
            V.data[i][j] = ((double)rand() / RAND_MAX) * 0.1;
        }
    }
    // Initialiser O et P à 0
    for (size_t i = 0; i < max_users; i++) 
        O.data[i][0] = 0.0;
    for (size_t j = 0; j < max_items; j++) 
        P.data[j][0] = 0.0;

    // Descente de gradient stochastique
    printf("Début de l'entraînement (%zu époques)...\n", epochs);
    for (size_t epoch = 0; epoch < epochs; epoch++) {
        double total_error = 0.0;
        
        for (size_t t = 0; t < num_transactions; t++) {
            size_t i = transactions[t].user_id;
            size_t j = transactions[t].item_id;
            double r_ij = transactions[t].rating;

            // Calculer la prédiction
            double r_hat_ij = O.data[i][0] + P.data[j][0];
            for (size_t s = 0; s < k; s++) {
                r_hat_ij += U.data[i][s] * V.data[j][s];
            }

            // Calculer l'erreur
            double e_ij = r_ij - r_hat_ij;
            total_error += e_ij * e_ij;

            // Mettre à jour O et P
            O.data[i][0] += alpha * (e_ij - lambda * O.data[i][0]);
            P.data[j][0] += alpha * (e_ij - lambda * P.data[j][0]);

            // Mettre à jour U et V
            for (size_t s = 0; s < k; s++) {
                double u_is = U.data[i][s];
                U.data[i][s] += alpha * (e_ij * V.data[j][s] - lambda * u_is);
                V.data[j][s] += alpha * (e_ij * u_is - lambda * V.data[j][s]);
            }
        }
        
        // Afficher l'erreur toutes les 10 époques
        if ((epoch + 1) % 10 == 0) {
            double rmse = sqrt(total_error / num_transactions);
            printf("Époque %zu/%zu - RMSE: %.4f\n", epoch + 1, epochs, rmse);
        }
    }

    // Créer la matrice pleine R = U * V^T + O + P
    printf("Création de V_T (transpose de V)\n");
    ndarray_t V_T = transpose(&V);
    printf("Création de R (%zu x %zu)\n", max_users, max_items);
    ndarray_t R = array(max_users, max_items);
    
    if (!R.data || !V_T.data) {
        printf("Erreur: échec de l'allocation de R ou V_T (R=%p, V_T=%p)\n", 
               (void*)R.data, (void*)V_T.data);
        clean(&U, &V, &O, &P, &V_T, NULL);
        free(transactions);
        free_array(&train_array);
        ndarray_t empty = {0};
        return empty;
    }

    // Calculer U * V^T manuellement
    printf("Calcul de la matrice de recommandation finale...\n");
    for (size_t i = 0; i < max_users; i++) {
        for (size_t j = 0; j < max_items; j++) {
            R.data[i][j] = 0.0;
            for (size_t s = 0; s < k; s++) {
                R.data[i][j] += U.data[i][s] * V_T.data[s][j];
            }
            R.data[i][j] += O.data[i][0] + P.data[j][0];
        }
    }

    // Nettoyage
    clean(&U, &V, &O, &P, &V_T, NULL);
    free(transactions);
    free_array(&train_array);
    
    printf("Matrice de factorisation créée avec succès!\n");
    return R;
}

ndarray_t Predict_all_MF(ndarray_t full_matrix, size_t batch_size, const char* test_data) {
    // Charger les données de test avec ndmath
    ndarray_t test_array = load_ndarray(test_data, batch_size);
    if (!test_array.data || full_matrix.shape[0] == 0 || full_matrix.shape[1] == 0) {
        printf("Erreur: données de test invalides ou matrice vide\n");
        ndarray_t empty = {0};
        return empty;
    }

    printf("Données de test chargées: %zu lignes x %zu colonnes\n", 
           test_array.shape[0], test_array.shape[1]);

    // Convertir en tableau de transactions
    size_t num_transactions;
    Transaction* test_transactions = ndarray_to_transactions(test_array, &num_transactions);
    if (!test_transactions) {
        printf("Erreur: échec de la conversion des transactions de test\n");
        free_array(&test_array);
        ndarray_t empty = {0};
        return empty;
    }

    // Créer une matrice pour stocker les prédictions
    printf("Création de predictions (%zu x 3)\n", num_transactions);
    ndarray_t predictions = array(num_transactions, 3);
    if (!predictions.data) {
        free(test_transactions);
        free_array(&test_array);
        ndarray_t empty = {0};
        return empty;
    }

    // Remplir les prédictions
    size_t valid_predictions = 0;
    for (size_t i = 0; i < num_transactions; i++) {
        size_t user_id = test_transactions[i].user_id;
        size_t item_id = test_transactions[i].item_id;
        
        predictions.data[i][0] = (double)(user_id + 1); // Revenir à l'index 1
        predictions.data[i][1] = (double)(item_id + 1); // Revenir à l'index 1
        
        if (user_id < full_matrix.shape[0] && item_id < full_matrix.shape[1]) {
            predictions.data[i][2] = full_matrix.data[user_id][item_id];
            valid_predictions++;
        } else {
            predictions.data[i][2] = 0.0; // Valeur par défaut si hors limites
            printf("Attention: utilisateur %zu ou item %zu hors limites\n", user_id + 1, item_id + 1);
        }
    }

    printf("Prédictions générées: %zu/%zu valides\n", valid_predictions, num_transactions);

    // Afficher un échantillon des prédictions
    printf("Échantillon des prédictions (user_id, item_id, note prédite) :\n");
    size_t sample_size = (num_transactions < 5) ? num_transactions : 5;
    for (size_t i = 0; i < sample_size; i++) {
        printf("Utilisateur %.0f, Item %.0f: %.4f\n", 
               predictions.data[i][0], predictions.data[i][1], predictions.data[i][2]);
    }

    // Nettoyage
    free(test_transactions);
    free_array(&test_array);
    
    return predictions;
}