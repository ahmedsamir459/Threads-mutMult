#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#define MAX_SIZE 20
#define MAX_FILENAME_LENGTH 50

struct thread_args
{
    int row;
    int col;
    FILE *out;
};
int matrix1[MAX_SIZE][MAX_SIZE];
int matrix2[MAX_SIZE][MAX_SIZE];
int row1, col1, row2, col2;

void *thread_per_matrix(void *args)
{
    int temp = 0;
    char out[MAX_FILENAME_LENGTH];
    strcpy(out, (char *)args);
    strcat(out, "_per_matrix.txt");
    FILE *fp = fopen(out, "w");
    fprintf(fp, "Method: A thread per matrix\n");
    fprintf(fp, "row=%d col=%d\n", row1, col2);

    for (int i = 0; i < row1; i++)
    {
        for (int j = 0; j < col2; j++)
        {
            for (int k = 0; k < col1; k++)
            {
                temp += matrix1[i][k] * matrix2[k][j];
            }
            fprintf(fp, "%d ", temp);
            temp = 0;
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    pthread_exit(NULL);
}

void *thread_per_row(void *arg)
{
    struct thread_args *args = (struct thread_args *)arg;
    int row = args->row;
    FILE *fp = args->out;
    int temp = 0;
    for (int z = 0; z < col2; z++)
    {
        for (int y = 0; y < col1; y++)
        {
            temp += matrix1[row][y] * matrix2[y][z];
        }
        fprintf(fp, "%d ", temp);
        temp = 0;
    }
    fprintf(fp, "\n");
    pthread_exit(NULL);
}

void *thread_per_element(void *arg)
{
    struct thread_args *args = (struct thread_args *)arg;
    int row = args->row;
    int col = args->col;
    FILE *fp = args->out;
    int temp = 0;
    for (int i = 0; i < row1; i++)
    {
        temp += matrix1[row][i] * matrix2[i][col];
    }
    fprintf(fp, "%d ", temp);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    struct thread_args args = {.row = 0, .out = NULL};
    struct timeval stop, start;

    pthread_t *threads;
    threads = (pthread_t *)malloc(sizeof(pthread_t));
    char in1[MAX_FILENAME_LENGTH];
    char in2[MAX_FILENAME_LENGTH];
    char out[MAX_FILENAME_LENGTH];

    if (argc == 4)
    {
        strncpy(in1, argv[1], MAX_FILENAME_LENGTH);
        strncpy(in2, argv[2], MAX_FILENAME_LENGTH);
        strncpy(out, argv[3], MAX_FILENAME_LENGTH);
    }
    else if (argc == 1)
    {
        strncpy(in1, "a.txt", MAX_FILENAME_LENGTH);
        strncpy(in2, "b.txt", MAX_FILENAME_LENGTH);
        strncpy(out, "c", MAX_FILENAME_LENGTH);
    }
    else
    {
        printf("Error: Invalid number of arguments\n");
        return 1;
    }
    strcat(in1, ".txt");
    strcat(in2, ".txt");

    FILE *fp1 = fopen(in1, "r");
    FILE *fp2 = fopen(in2, "r");

    if (fp1 == NULL || fp2 == NULL)
    {
        printf("Error opening input files\n");
        exit(1);
    }
    fscanf(fp1, "row=%d col=%d\n", &row1, &col1);
    fscanf(fp2, "row=%d col=%d\n", &row2, &col2);

    if (col1 != row2)
    {
        printf("Error: Matrix dimensions do not match");
        exit(1);
    }
    // Read matrix values from first file
    for (int i = 0; i < row1; i++)
    {
        for (int j = 0; j < col1; j++)
        {
            fscanf(fp1, "%d", &matrix1[i][j]);
        }
    }
    // Read matrix values from second file
    for (int i = 0; i < row2; i++)
    {
        for (int j = 0; j < col2; j++)
        {
            fscanf(fp2, "%d", &matrix2[i][j]);
        }
    }

    int rc;

    //Thread per matrix
    gettimeofday(&start, NULL); //start checking time
    rc = pthread_create(&threads[0], NULL, thread_per_matrix, (void *)out);
    if (rc)
    {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
    pthread_join(threads[0], NULL);
    gettimeofday(&stop, NULL); //end checking time
    printf("Thread per matrix method took:%lu milliseconds and operated on %d threads\n", stop.tv_usec - start.tv_usec, 1);

    threads = realloc(threads, (row1 + 1) * sizeof(pthread_t));

    //Thread per row
    {
        gettimeofday(&start, NULL); //start checking time
        char *temp = malloc(sizeof(char) * MAX_FILENAME_LENGTH);
        strcpy(temp, out);
        strcat(temp, "_per_row.txt");
        FILE *fp = fopen(temp, "w");
        args.out = fp;
        fprintf(fp, "Method: A thread per row\n");
        fprintf(fp, "row=%d col=%d\n", row1, col2);
        for (int i = 1; i <= row1; i++)
        {
            args.row = i - 1;
            rc = pthread_create(&threads[i], NULL, thread_per_row, (void *)&args);
            if (rc)
            {
                printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
            pthread_join(threads[i], NULL);
        }
        free(temp);
        fclose(fp);
        gettimeofday(&stop, NULL); //end checking time
        printf("Thread per row method took:%lu milliseconds and operated on %d threads\n", stop.tv_usec - start.tv_usec, row1);
    }
    threads = realloc(threads, (row1 * row1 + row1 + 1) * sizeof(pthread_t));

    // Thread per element
    {
        gettimeofday(&start, NULL); //start checking time
        strcat(out, "_per_element.txt");
        FILE *fp = fopen(out, "w");
        fprintf(fp, "Method: A thread per element\n");
        fprintf(fp, "row=%d col=%d\n", row1, col2);
        int count = row1;
        for (int i = 0; i < row1; i++)
        {
            for (int j = 0; j < col2; j++)
            {
                args.row = i;
                args.col = j;
                args.out = fp;
                rc = pthread_create(&threads[count], NULL, thread_per_element, (void *)&args);
                if (rc)
                {
                    printf("ERROR; return code from pthread_create() is %d\n", rc);
                    exit(-1);
                }
                pthread_join(threads[count], NULL);
                count++;
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
        gettimeofday(&stop, NULL); //end checking time
        printf("Thread per element method took:%lu milliseconds and operated on %d threads\n", stop.tv_usec - start.tv_usec, row1 * row1);
    }
    
    fclose(fp1);
    fclose(fp2);
    free(threads);
    pthread_exit(NULL);
    return 0;
}
