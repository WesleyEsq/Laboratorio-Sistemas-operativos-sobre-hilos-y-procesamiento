#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

const int THREAD_BATCH_SIZE = 4; 

typedef struct {
    int** A;
    int** B;
    int** C;
    int row;
    int col;
    int colsA;
} TData;

//-------------------------------------------------------------------------
//                         Funciones auxiliares
//-------------------------------------------------------------------------

//Escribir los resultados de las operaciones
void writeLog(const char *filepath, double execution_time){
    FILE *log_file = fopen(filepath, "a"); 
    if (log_file == NULL) {
        printf("Error opening log file!\n");
        return;
    }
    fprintf(log_file, "%f\n", execution_time);
    fclose(log_file);
}

//Crear las matrices
int** createMatrix(const int rows, const int cols) {
    int** matrix = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++)
        matrix[i] = (int*)malloc(cols * sizeof(int));
    return matrix;
}

// Llenar con valores del 1 al 5
void fillRandomMatrix(int **matrix, const int rows, const int cols){
    for(int i = 0; i < rows; i++) 
        for(int j = 0; j < cols; j++)
            matrix[i][j] = (rand() % 5) + 1; 
}

void print_matrix(int** matrix, const int a, const int b) {
    printf("Matrix contents:\n");
    for (int i = 0; i < a; i++) {
        for (int j = 0; j < b; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

//Por las malas recorde hacer esto
void free_matrix(int** matrix, const int a) {
    for (int i = 0; i < a; i++) {
        free(matrix[i]); 
    }
    free(matrix);
}

//-------------------------------------------------------------------------
//                         Codigo del problema
//-------------------------------------------------------------------------

void multiplyMatrices(int** A, int** B, int** C, int rowsA, int colsA, int colsB) {
    for (int i = 0; i < rowsA; i++) 
        for (int j = 0; j < colsB; j++) 
        {
            C[i][j] = 0; 
            for (int k = 0; k < colsA; k++) 
                C[i][j] += A[i][k] * B[k][j];
            
        }
}

//Para cada thread
void* calCell(void* arg) {
    TData* data = (TData*)arg;
    int sum = 0;
    for (int k = 0; k < data->colsA; k++) {
        sum += data->A[data->row][k] * data->B[k][data->col];
    }
    data->C[data->row][data->col] = sum;
    return NULL;
}

//Idea boba para resolver esto de los threads:

//Arreglo de 8 threads, apenas terminan se eliminan.
//Van creandose threads en grupos de N hasta que se termine de calcular la matriz de resultado.
//Porque ¿Hacer una fila de 1 millon de elementos? ... NOPE eso se agota los 8gb de ram como si nada.

//¿Ir formandolos? Ok!
//Solución chafa: Un triple for para ir recorriendo C, se guardan N structs que guarden los datos para los threads en
// un arreglo de N tamaño, cuando se llena el arreglo (una variable contador que iba incrementando si 0 < N, osea un 4 for)
// llega, se ejecutan 8 threads nuevos con esos datos, se espera a que esos 8 terminen y se sigue calculando la matriz.
void multiplyMatricesThreaded(int** A, int** B, int** C, int rowsA, int colsA, int colsB, int batchSize) {
    pthread_t threads[batchSize];
    TData data[batchSize];//"colas" para ir ejecutando los resultados de las celdas, no, no quiero implementar una cola.

    int threadCount = 0;

    for (int i = 0; i < rowsA; i++) {
        for (int j = 0; j < colsB; j++) {
            //Preparar los datos
            data[threadCount].A = A;
            data[threadCount].B = B;
            data[threadCount].C = C;
            data[threadCount].row = i;
            data[threadCount].col = j;
            data[threadCount].colsA = colsA;
            threadCount++;

            // Si se llena la cantidad de threads a ejecutar, haga los threads
            if (threadCount == batchSize) {

                for (int k = 0; k < batchSize; k++) {
                    pthread_create(&threads[k], NULL, calCell, &data[k]);
                }

                //Esperar, esto ocasiona un sleep a veces en el proceso pero es algo.
                for (int k = 0; k < batchSize; k++) {
                    pthread_join(threads[k], NULL);
                }
                
                threadCount = 0;
                //printf("done 1 ");
            }
        }
    }

    //Si el total de celdas no es divisible por la cantidad de hilos adecuada
    if (threadCount > 0) {
        for (int k = 0; k < threadCount; k++) {
            pthread_create(&threads[k], NULL, calCell, &data[k]);
        }
        for (int k = 0; k < threadCount; k++) {
            pthread_join(threads[k], NULL);
        }
    }
}

//-------------------------------------------------------------------------
//                                  Main
//-------------------------------------------------------------------------

void do_100_multiplications() {
    int r1 = 1000, c1 = 1000;
    int r2 = 1000, c2 = 1000;
    if (c1 != r2) {
        printf("Matrix dimensions are not compatible for multiplication.\n");
        return;
    }

    for (int i = 0; i < 100; i++) {
        printf("repetition ");
        int** A = createMatrix(r1, c1);
        int** B = createMatrix(r2, c2);
        int** C = createMatrix(r1, c2);

        fillRandomMatrix(A, r1, c1);
        fillRandomMatrix(B, r2, c2);

        //Tomar tiempo
        clock_t start_time = clock();
        //multiplyMatrices(A, B, C, r1, c1, c2);
        multiplyMatricesThreaded(A, B, C, r1, c1, c2, THREAD_BATCH_SIZE);
        clock_t end_time = clock();
        
        //Logearlo
        double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        writeLog("execution_log.txt", time_spent);

        free_matrix(A, r1);
        free_matrix(B, r2);
        free_matrix(C, r1);
        printf("%d\n", i+1);
    }
    printf("Completed 100 matrix multiplications. Results are in execution_log.txt\n");
}


int main(int argc, char *argv[]){
    do_100_multiplications();
    
    return 0;
}