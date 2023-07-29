#include <upc.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Shared matrices */
shared int aMatrix[THREADS];
shared int bMatrix[THREADS];
shared int resultMatrix[THREADS];

/* Define the matrix DIMENSION as the square root of the number of threads */
shared int DIMENSION = (int)sqrt(THREADS);

#define MASTER 0  // Master thread index
bool VERBOSE = 0;  // Log visibility flag

/**
 * @brief Print the given matrix.
 *
 * @param matrix Matrix to print.
 */
void printMatrix(shared int matrix[])
{
    /* Iterate over all elements in the matrix */
    for (int i = 0; i < DIMENSION * DIMENSION; i++)
    {
        /* Print the current element */
        printf("%d\t", matrix[i]);

        /* At the end of each row, print a newline character */
        if ((i % DIMENSION) == (DIMENSION - 1))
            printf("\n");
    }
    printf("\n");
}

/**
 * @brief Initialize a matrix from a file.
 *
 * @param file Pointer to file from which the matrix will be imported.
 * @param matrixToFill Pointer to the matrix to be filled.
 */
void readMatrix(FILE* file, shared int matrixToFill[])
{
    /* Read each integer from the file and store it in the matrix */
    for (int i = 0; i < DIMENSION * DIMENSION; i++)
    {
        int element;
        fscanf(file, "%d", &element);
        matrixToFill[i] = element;
    }
    /* Close the file after reading from it */
    fclose(file);
}

/**
 * @brief Save the matrix to a file.
 *
 * @param file Pointer to the file where the matrix will be exported.
 * @param matrix Matrix to save.
 */
void saveMatrix(FILE* file, shared int matrix[])
{
    if (VERBOSE)
    {
        printf("Saving matrix to output file.\n");
    }

    /* Iterate over all elements in the matrix */
    for (int i = 0; i < DIMENSION * DIMENSION; i++)
    {
        /* Write the current element to the file */
        fprintf(file, "%d\t", matrix[i]);

        /* At the end of each row, write a newline character to the file */
        if ((i % DIMENSION) == (DIMENSION - 1))
            fprintf(file, "\n");
    }
    /* Close the file after writing to it */
    fclose(file);
}

int main(int argc, char* argv[])
{
    int threadId;
    FILE* file;
    char file1[256], file2[256], outputFileName[256];

    /* Check if the number of threads is a perfect square */
    if (sqrt(THREADS) - (int)sqrt(THREADS) != 0)
    {
        printf("ERROR: Root square of number of threads should be integer (natural number)!\n");
        return 1;
    }

    /* Only the master thread executes this block */
    if (MYTHREAD == MASTER)
    {
        /* Check if enough command line arguments are provided */
        if (argc >= 4)
        {
            /* Copy file names from command line arguments */
            strcpy(file1, argv[1]);
            strcpy(file2, argv[2]);
            strcpy(outputFileName, argv[3]);
        }

        /* If the fourth argument is "-v", set VERBOSE to 1 */
        if (argc >= 5 && strcmp(argv[4], "-v") == 0)
        {
            VERBOSE = 1;
        }

        /* Open the first file and read its contents into aMatrix */
        file = fopen(file1, "r");
        if (file == NULL)
        {
            printf("ERROR: No such file for Matrix 1!\n");
            return 1;
        }
        readMatrix(file, aMatrix);

        /* Print aMatrix if VERBOSE is true */
        if (VERBOSE)
        {
            printf("Matrix 1: \n");
            printMatrix(aMatrix);
        }

        /* Open the second file and read its contents into bMatrix */
        file = fopen(file2, "r");
        if (file == NULL)
        {
            printf("ERROR: No such file for Matrix 2!\n");
            return 1;
        }
        readMatrix(file, bMatrix);

        /* Print bMatrix if VERBOSE is true */
        if (VERBOSE)
        {
            printf("Matrix 2: \n");
            printMatrix(bMatrix);
        }
    }

    /* Synchronize all threads before starting matrix multiplication */
    upc_barrier;

    /* Each thread performs part of the matrix multiplication */
    upc_forall(threadId = 0; threadId < THREADS; threadId++; &resultMatrix[threadId])
    {
        /* Calculate the row and column indices for this thread */
        int row = threadId / DIMENSION;
        int column = threadId % DIMENSION;

        /* Compute the initial position of this thread in aMatrix and bMatrix */
        int aMatrixColumn = (column + row) % DIMENSION;
        int bMatrixRow = (row + column) % DIMENSION;
        int rowDimension = row * DIMENSION; // Compute once and use it multiple times

        /* Each thread multiplies one row of aMatrix with one column of bMatrix */
        for (int shift = 0; shift < DIMENSION; shift++)
        {
            /* Shift the column of aMatrix and the row of bMatrix to the right */
            aMatrixColumn = (aMatrixColumn + 1) % DIMENSION;
            bMatrixRow = (bMatrixRow + 1) % DIMENSION;

            /* Compute the linear indices in aMatrix and bMatrix */
            int indexA = rowDimension + aMatrixColumn; // Merged computation
            int indexB = bMatrixRow * DIMENSION + column; // Merged computation

            /* Multiply the corresponding elements of aMatrix and bMatrix and add the result to resultMatrix */
            resultMatrix[threadId] += aMatrix[indexA] * bMatrix[indexB];
        }
    }

    /* Synchronize all threads after matrix multiplication */
    upc_barrier;

    /* Only the master thread executes this block */
    if (MYTHREAD == MASTER)
    {
        /* Print resultMatrix if VERBOSE is true */
        if (VERBOSE)
        {
            printf("Output matrix: \n");
            printMatrix(resultMatrix);
        }

        /* Open the output file and write resultMatrix to it */
        file = fopen(outputFileName, "w");
        if (file == NULL)
        {
            printf("ERROR: File cannot be created for result matrix!\n");
            return 1;
        }
        saveMatrix(file, resultMatrix);
        printf("Program execution finished.\n");
    }
    return 0;
}
