#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define N 20

// Global matrices and sizes
int A[N][N], B[N][N], C[N][N];
int rows_A, cols_A, rows_B, cols_B;
char input_file1[50] = "a.txt";
char input_file2[50] = "b.txt";
char output_prefix[50] = "c";

typedef struct {
    int rows_A;
    int cols_B;
    int cols_A;
} per_matrix;

typedef struct {
    int cols_A;
    int row_counter;
    int col_counter;
} per_element;

void read_matrix(const char* filename, int matrix[N][N], int* rows, int* cols) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    fscanf(file, "row=%d col=%d", rows, cols);
    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            fscanf(file, "%d", &matrix[i][j]);
        }
    }
    fclose(file);
}

void write_matrix(const char* prefix, const char* method) {
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_per_%s.txt", prefix, method);
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Error writing file");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "Method: A thread per %s\n", method);
    fprintf(file, "row=%d col=%d\n", rows_A, cols_B);
    for (int i = 0; i < rows_A; i++) {
        for (int j = 0; j < cols_B; j++) {
            fprintf(file, "%d\t", C[i][j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

void* thread_per_row(void* ptr) {
    int* row = (int*)ptr;
    for (int col = 0; col < cols_B; col++) {
        C[*row][col] = 0;
        for (int k = 0; k < cols_A; k++) {
            C[*row][col] += A[*row][k] * B[k][col];
        }
    }
    free(row);
    return NULL;
}

void* thread_per_matrix(void* ptr) {
    per_matrix* params = (per_matrix*)ptr;
    for (int i = 0; i < params->rows_A; i++) {
        for (int j = 0; j < params->cols_B; j++) {
            C[i][j] = 0;
            for (int k = 0; k < params->cols_A; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    free(ptr);
    return NULL;
}

void* thread_per_element(void* ptr) {
    per_element* params_element = (per_element*)ptr;
    C[params_element->row_counter][params_element->col_counter] = 0;
    for (int k = 0; k < params_element->cols_A; k++) {
        C[params_element->row_counter][params_element->col_counter] += A[params_element->row_counter][k] * B[k][params_element->col_counter];
    }
    free(ptr);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc == 4) {
        snprintf(input_file1, sizeof(input_file1), "%s.txt", argv[1]);
        snprintf(input_file2, sizeof(input_file2), "%s.txt", argv[2]);
        snprintf(output_prefix, sizeof(output_prefix), "%s", argv[3]);
    }

    read_matrix(input_file1, A, &rows_A, &cols_A);
    read_matrix(input_file2, B, &rows_B, &cols_B);

    if (cols_A != rows_B) {
        printf("Matrix multiplication not possible! (%dx%d) * (%dx%d)\n", rows_A, cols_A, rows_B, cols_B);
        return EXIT_FAILURE;
    }

    pthread_t thread1;
    per_matrix* params1 = malloc(sizeof(per_matrix));
    params1->rows_A = rows_A;
    params1->cols_B = cols_B;
    params1->cols_A = cols_A;
    pthread_create(&thread1, NULL, thread_per_matrix, (void*)params1);

    
    pthread_t threads[rows_A];
    for (int i = 0; i < rows_A; i++) {
        int* row = malloc(sizeof(int));
        *row = i;
        pthread_create(&threads[i], NULL, thread_per_row, (void*)row);
    }
    
    pthread_t* thread_for_element = malloc(rows_A * cols_B * sizeof(pthread_t));
    int k = 0;
    for (int i = 0; i < rows_A; i++) {
        for (int j = 0; j < cols_B; j++) {
            per_element* params2 = malloc(sizeof(per_element));
            params2->cols_A = cols_A;
            params2->row_counter = i;
            params2->col_counter = j;
            pthread_create(&thread_for_element[k++], NULL, thread_per_element, (void*)params2);
        }
    }
    
    pthread_join(thread1, NULL);
    write_matrix(output_prefix, "matrix");
    

    for (int i = 0; i < rows_A; i++) {
        pthread_join(threads[i], NULL);
    }
    write_matrix(output_prefix, "row");


    for (int l = 0; l < rows_A * cols_B; l++) {
        pthread_join(thread_for_element[l], NULL);
    }
    write_matrix(output_prefix, "element");
    free(thread_for_element);
    return 0;
}
