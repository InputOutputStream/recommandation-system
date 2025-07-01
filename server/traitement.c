#include <ndmath/all.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "../include/traitement.h"

static bool double_equals(double a, double b) {
    const double epsilon = 1e-9;
    return fabs(a - b) < epsilon;
}


date_t format_seconds_to_date(double seconds) {
    time_t timestamp = (time_t)seconds;
    
    struct tm *date_info = localtime(&timestamp);
    
    if (date_info == NULL) {
        fprintf(stderr, "Error: Failed to convert timestamp to date\n");
        date_t invalid_date = {0, 0, 0};
        return invalid_date;
    }
    
    // Extract day, month, year
    date_t date = {date_info->tm_mday, date_info->tm_mon + 1, date_info->tm_year + 1900};
    return date;    
}

bool is_less(date_t d1, date_t d2)
{
    if(d1.year < d2.year)
        return true;
    else if(d1.year == d2.year && d1.month < d2.month)
        return true;
    else if(d1.year == d2.year && d1.month == d2.month && d1.day < d2.day)
        return true;
    return false;
}

bool is_greater(date_t d1, date_t d2)
{
    if(d1.year > d2.year)
        return true;
    else if(d1.year == d2.year && d1.month > d2.month)
        return true;
    else if(d1.year == d2.year && d1.month == d2.month && d1.day > d2.day)
        return true;
    return false;
}

bool is_equal(date_t d1, date_t d2) {
    return d1.year == d2.year && d1.month == d2.month && d1.day == d2.day;
}

bool is_less_equal(date_t d1, date_t d2) {
    return is_less(d1, d2) || is_equal(d1, d2);
}

bool is_greater_equal(date_t d1, date_t d2)
{
    return is_greater(d1, d2) || is_equal(d1, d2);
}

bool is_valid_date(date_t date) {
    return date.year > 0 && date.month > 0 && date.day > 0;
}

ndarray_t get_timed_data_array(ndarray_t data, date_t start, date_t stop)
{
         
    if(is_less(stop, start))
    {
        fprintf(stderr, "Error: Start date is greater than stop date\n");
        ndarray_t empty = {0};
        return empty;
    }
        
    size_t n = data.shape[0];
    size_t m = data.shape[1];
    
    size_t timestamp_col = m - 1;
    
    size_t j = 0;
    for(size_t i = 0; i < n; i++)
    {
        date_t date = format_seconds_to_date(data.data[i][timestamp_col]);
        
        if (!is_valid_date(date)) {
            continue;
        }
        
        if(is_greater_equal(date, start) && is_less_equal(date, stop))
            j++;
    }

    if(j == 0)
    {
        fprintf(stdout, "No data matches the specified time interval\n");
        return data;
    }

    int *lines = malloc(j * sizeof(int));
    if (lines == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for line indices\n");
        return data;
    }
    
    size_t nlines = j;
    j = 0;

    for(size_t i = 0; i < n; i++)
    {
        date_t date = format_seconds_to_date(data.data[i][timestamp_col]);
        
        if (!is_valid_date(date)) {
            continue;
        }
        
        if(is_greater_equal(date, start) && is_less_equal(date, stop))
        {
            lines[j++] = (int)i;
        }
    }

    ndarray_t result = get_lines(&data, lines, nlines);
    
    // Clean up
    free(lines);

    if (result.data == NULL) {
        fprintf(stderr, "Error: Failed to create filtered result\n");
    }

    char name [70];
    sprintf(name, "transaction-%d-%d-to-%d-%d", start.month, start.year, stop.month, stop.year);
    save_ndarray(&result, name);
    return result;
}



ndarray_t load_timed_data(char *path, date_t start, date_t stop)
{
    // Input validation
    if (path == NULL) {
        fprintf(stderr, "Error: Path cannot be NULL\n");
        ndarray_t empty = {0};
        return empty;
    }
    
    ndarray_t data = load_ndarray(path, 0);
    return get_timed_data_array(data, start, stop);
}


size_t get_count_safe(ndarray_t array, double value, size_t index, bool is_row)
{
    ndarray_t result;
    if(is_row) {
        result = count_rows(array, value, index);
    } else {
        result = count_cols(array, value, index);
    }
    
    if(result.data == NULL) {
        return 0;
    }
    
    size_t count = (size_t)result.data[0][0];
    
    // Libérer la mémoire
    free(result.data[0]);
    free(result.data);    
    return count;
}


ndarray_t nettoyer_transaction(ndarray_t transactions, size_t minU, size_t minI)
{
    size_t n = transactions.shape[0];
    int lines[n];
    int idx = 0;
    
    for(size_t i = 0; i < n; i++)
    {
        double idU = transactions.data[i][0];
        double idI = transactions.data[i][1];

        // Fonction helper pour éviter la répétition
        size_t count_u = get_count_safe(transactions, idU, 0, true);  // true = count_rows
        size_t count_i = get_count_safe(transactions, idI, 1, true);
        
        if(count_u >= minU && count_i >= minI)
        {
            lines[idx++] = i;
        }
    }
    
    return get_lines(&transactions, lines, idx);
}

void train_test_split(ndarray_t this, float share, int random_state)
{
    return train_test_split_with_files(this, share, random_state, "data/train_split.txt", "data/test_split.txt");
}

// Garder les lignes de test où col0 ET col1 existent dans train
void clean_files(char *test_file, char *train_file, char *result_file_path)
{
    ndarray_t test = load_ndarray(test_file, 0);
    ndarray_t train = load_ndarray(train_file, 0);
    
    int lines[test.shape[0]];
    int idx = 0;
    
    // Pour chaque ligne de test
    for(size_t i = 0; i < test.shape[0]; i++)
    {
        double test_col0 = get(&test, i, 0);
        double test_col1 = get(&test, i, 1);
        
        bool col0_exists = false;
        bool col1_exists = false;
        
        // Vérifier si col0 et col1 existent quelque part dans train
        for(size_t j = 0; j < train.shape[0]; j++)
        {
            if(!col0_exists && double_equals(test_col0, get(&train, j, 0))) {
                col0_exists = true;
            }
            if(!col1_exists && double_equals(test_col1, get(&train, j, 1))) {
                col1_exists = true;
            }
            
            // Optimisation : arrêter si les deux sont trouvés
            if(col0_exists && col1_exists) {
                break;
            }
        }
        
        // Garder la ligne si BOTH colonnes existent dans train
        if(col0_exists && col1_exists)
        {
            lines[idx++] = i;
        }
    }

    ndarray_t new_test = get_lines(&test, lines, idx);
    save_ndarray(&new_test, result_file_path);
    fprintf(stdout, "Created Clean %s file \n", result_file_path);

    // Nettoyage mémoire si nécessaire
    free_array(&test);
    free_array(&train);
    free_array(&new_test);
}

void all_treatment(date_t *d1, date_t *d2, char *data_file, size_t batch_size, 
                   size_t minU, size_t minI, float split, int random_state, 
                   char *train_file, char *test_file)
{
    ndarray_t result = {0};
    ndarray_t transaction_clean = {0};
    
    // Variables pour gérer les noms de fichiers
    char *train_path = NULL;
    char *test_path = NULL;
    bool allocated_paths = false;

    // 1. Charger les données
    if(d1 != NULL && d2 != NULL) {
        result = load_timed_data(data_file, *d1, *d2);    
    } else {
        result = load_ndarray(data_file, batch_size);
    }

    if (result.data == NULL) {
        fprintf(stderr, "Error: Failed to load data from %s\n", data_file);
        goto cleanup;
    }
    
    printf("Loaded %zu rows from %s\n", result.shape[0], data_file);
    
    // 2. Nettoyer les transactions
    transaction_clean = nettoyer_transaction(result, minU, minI);
    
    if (transaction_clean.data == NULL) {
        fprintf(stderr, "Error: Failed to clean transactions\n");
        goto cleanup;
    }
    
    printf("After cleaning: %zu rows remaining (minU=%zu, minI=%zu)\n", 
           transaction_clean.shape[0], minU, minI);
    
    // 3. Gérer les noms de fichiers
    if(train_file == NULL || test_file == NULL)
    {
        // Allouer de nouveaux chemins par défaut
        train_path = malloc(strlen("data/train_split.txt") + 1);
        test_path = malloc(strlen("data/test_split.txt") + 1);
        
        if(train_path == NULL || test_path == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for file paths\n");
            goto cleanup;
        }
        
        strcpy(train_path, "data/train_split.txt");
        strcpy(test_path, "data/test_split.txt");
        allocated_paths = true;
    }
    else
    {
        // Utiliser les chemins fournis
        train_path = train_file;
        test_path = test_file;
    }

    // 4. Créer le nom du fichier de sortie nettoyé
    char *result_file = malloc(strlen(test_path) + 20);
    if(result_file == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for result file path\n");
        goto cleanup;
    }
    sprintf(result_file, "%s_clean", test_path);

    // 5. Train/test split - adapter selon votre signature de fonction
    train_test_split_with_files(transaction_clean, split, random_state, train_path, test_path);
    printf("Train/test split completed (%.1f%% train)\n", split * 100);
    
    // 6. Nettoyer les fichiers
    clean_files(test_path, train_path, result_file);
    printf("Files cleaned and saved to %s\n", result_file);
    
    free(result_file);

cleanup:
    // Libération sécurisée
    if (result.data != NULL) {
        free_array(&result);
    }
    if (transaction_clean.data != NULL) {
        free_array(&transaction_clean);
    }
    
    // Libérer seulement si on a alloué
    if (allocated_paths) {
        free(train_path);
        free(test_path);
    }
}

void train_test_split_with_files(ndarray_t this, float share, int random_state, 
                                 char *train_file, char *test_file)
{
    shuffle(&this, random_state);
    
    size_t total_lines = this.shape[0];
    size_t nlines = (size_t)(total_lines * share);
    
    int *train_lines = malloc(nlines * sizeof(int));
    int *test_lines = malloc((total_lines - nlines) * sizeof(int));
    
    if(!train_lines || !test_lines) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    // Remplissage
    for(size_t i = 0; i < nlines; i++) {
        train_lines[i] = i;
    }
    for(size_t i = nlines; i < total_lines; i++) {
        test_lines[i - nlines] = i;
    }

    ndarray_t train_split = get_lines(&this, train_lines, nlines);
    ndarray_t test_split = get_lines(&this, test_lines, total_lines - nlines);

    save_ndarray(&train_split, train_file);
    save_ndarray(&test_split, test_file);
    
    printf("Created the files %s and %s\n", train_file, test_file);
    
    // Nettoyage
    free(train_lines);
    free(test_lines);
}

int main()
{
    all_treatment(NULL, NULL, "data/transactions.txt", 1000, 
                  2, 2, 0.9, 0, NULL, NULL);
    return 0;
}