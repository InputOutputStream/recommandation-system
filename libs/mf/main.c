#include <ndmath/array.h>
#include <ndmath/io.h>
#include <ndmath/helper.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mf.h"

// Fonction pour calculer la RMSE
double calculate_rmse(ndarray_t predictions, ndarray_t test_data) {
    if (predictions.shape[0] != test_data.shape[0]) {
        printf("Erreur: tailles incompatibles pour RMSE\n");
        return -1.0;
    }
    
    double sum_squared_error = 0.0;
    size_t valid_predictions = 0;
    
    for (size_t i = 0; i < predictions.shape[0]; i++) {
        // La note réelle est en colonne 3 dans test_data
        double actual_rating = test_data.data[i][3];
        double predicted_rating = predictions.data[i][2];
        
        // Vérifier que la prédiction est valide
        if (predicted_rating != 0.0) {
            double error = actual_rating - predicted_rating;
            sum_squared_error += error * error;
            valid_predictions++;
        }
    }
    
    if (valid_predictions == 0) {
        printf("Erreur: aucune prédiction valide pour RMSE\n");
        return -1.0;
    }
    
    return sqrt(sum_squared_error / valid_predictions);
}

// Fonction pour calculer la MAE
double calculate_mae(ndarray_t predictions, ndarray_t test_data) {
    if (predictions.shape[0] != test_data.shape[0]) {
        printf("Erreur: tailles incompatibles pour MAE\n");
        return -1.0;
    }
    
    double sum_abs_error = 0.0;
    size_t valid_predictions = 0;
    
    for (size_t i = 0; i < predictions.shape[0]; i++) {
        // La note réelle est en colonne 3 dans test_data
        double actual_rating = test_data.data[i][3];
        double predicted_rating = predictions.data[i][2];
        
        // Vérifier que la prédiction est valide
        if (predicted_rating != 0.0) {
            double error = fabs(actual_rating - predicted_rating);
            sum_abs_error += error;
            valid_predictions++;
        }
    }
    
    if (valid_predictions == 0) {
        printf("Erreur: aucune prédiction valide pour MAE\n");
        return -1.0;
    }
    
    return sum_abs_error / valid_predictions;
}

int main() {
    // Paramètres de la factorisation matricielle
    size_t k = 10; // Nombre de facteurs latents
    double alpha = 0.01; // Taux d'apprentissage
    double lambda = 0.1; // Paramètre de régularisation
    size_t epochs = 100; // Nombre d'époques

    // Chemins des fichiers de données (compatible avec traitement.c)
    const char* train_data = "../../data/train_split.txt";
    const char* test_data = "../../data/test_split.txt_clean";

    printf("=== Factorisation Matricielle avec données structurées ===\n");
    printf("Paramètres: k=%zu, alpha=%.3f, lambda=%.3f, epochs=%zu\n", 
           k, alpha, lambda, epochs);

    // Étape 1 : Construire la matrice de notes pleines avec MF
    printf("\n--- Construction de la matrice de notes pleines ---\n");
    ndarray_t full_matrix = MF(train_data, 0, k,  alpha, lambda, epochs);
    if (full_matrix.shape[0] == 0) {
        printf("Erreur: échec de la construction de la matrice pleine\n");
        return 1;
    }
    printf("Matrice pleine créée (shape: %zu x %zu)\n", full_matrix.shape[0], full_matrix.shape[1]);
    
    // Afficher un échantillon de la matrice (limité pour éviter trop de sortie)
    printf("\nÉchantillon de la matrice pleine (5 premières lignes/colonnes) :\n");
    size_t sample_rows = (full_matrix.shape[0] < 5) ? full_matrix.shape[0] : 5;
    size_t sample_cols = (full_matrix.shape[1] < 5) ? full_matrix.shape[1] : 5;
    
    for (size_t i = 0; i < sample_rows; i++) {
        for (size_t j = 0; j < sample_cols; j++) {
            printf("%6.2f ", full_matrix.data[i][j]);
        }
        printf("\n");
    }

    // Étape 2 : Prédire les notes pour les données de test
    printf("\n--- Prédiction des notes pour les données de test ---\n");
    ndarray_t predictions = Predict_all_MF(full_matrix, 0, test_data);
    if (predictions.shape[0] == 0) {
        printf("Erreur: échec de la prédiction\n");
        clean(&full_matrix, NULL);
        return 1;
    }

    // Charger les données de test pour comparaison (avec ndmath)
    ndarray_t test_array = load_ndarray(test_data, 0);
    if (!test_array.data) {
        printf("Erreur: échec du chargement des données de test pour l'évaluation\n");
        clean(&full_matrix, &predictions, NULL);
        return 1;
    }

    // Afficher quelques prédictions
    printf("\nÉchantillon des prédictions (user_id, item_id, note prédite, note réelle) :\n");
    size_t sample_size = (predictions.shape[0] < 10) ? predictions.shape[0] : 10;
    for (size_t i = 0; i < sample_size; i++) {
        printf("Utilisateur %.0f, Item %.0f : Prédite = %.2f, Réelle = %.2f\n",
               predictions.data[i][0], predictions.data[i][1],
               predictions.data[i][2], test_array.data[i][3]);
    }

    // Calculer et afficher les métriques RMSE et MAE
    printf("\n--- Évaluation des performances ---\n");
    double rmse = calculate_rmse(predictions, test_array);
    double mae = calculate_mae(predictions, test_array);
    
    if (rmse >= 0 && mae >= 0) {
        printf("Métriques d'évaluation :\n");
        printf("RMSE : %.4f\n", rmse);
        printf("MAE  : %.4f\n", mae);
        printf("Nombre de prédictions évaluées : %zu\n", predictions.shape[0]);
    } else {
        printf("Erreur dans le calcul des métriques\n");
    }

    // Statistiques supplémentaires
    printf("\n--- Statistiques supplémentaires ---\n");
    printf("Taille de la matrice complète : %zu utilisateurs x %zu items\n", 
           full_matrix.shape[0], full_matrix.shape[1]);
    printf("Nombre total de prédictions : %zu\n", predictions.shape[0]);
    
    // Calculer la sparsité du jeu de test
    double sparsity = (double)(predictions.shape[0]) / 
                     (double)(full_matrix.shape[0] * full_matrix.shape[1]) * 100.0;
    printf("Sparsité du jeu de test : %.2f%%\n", sparsity);

    // Nettoyage
    free_array(&test_array);
    clean(&full_matrix, &predictions, NULL);

    printf("\n=== Fin de l'évaluation ===\n");
    return 0;
}