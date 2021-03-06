// El programa traspuesta.c calcula la transpuesta de una matriz triangular de N*N.
// Compilar y ejecutar para 4 threads comparándolo con el algoritmo secuencial.
// El programa tiene un problema, describir de que problema se trata. ¿Qué cláusula
// usaría para corregir el problema? Describir brevemente la cláusula OpenMP que
// resuelve el problema y las opciones que tiene. Corregir y ejecutar de nuevo
// comparando con los resultados anteriores.
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* Time in seconds from some point in the past */
double dwalltime();

int main(int argc, char *argv[])
{
	double *A;
	double temp;
	int i, j, N, tid;
	int check = 1;
	double timetick;
	double time_total = 0;

	//Controla los argumentos al programa
	if (argc < 3)
	{
		printf("\n Faltan argumentos:: N dimension de la matriz, T cantidad de threads \n");
		return 0;
	}
	N = atoi(argv[1]);
	int numThreads = atoi(argv[2]);
	omp_set_num_threads(numThreads);

	//Aloca memoria para las matrices
	A = (double *)malloc(sizeof(double) * N * N);

	//Inicializa la matriz con unos en el triangulo inferior y ceros en el triangulo superior.
	for (i = 0; i < N; i++)
	{
		for (j = 0; j < N; j++)
		{
			if (i >= j)
				A[i * N + j] = 1.0;
			else
				A[i * N + j] = 0.0;
		}
	}
	time_total = dwalltime();
#pragma omp parallel default(none) private(i, j, temp, timetick, tid) shared(A, N)
	{
		tid = omp_get_thread_num();
		timetick = dwalltime();
		unsigned int iteration_counter[] = {0,0,0,0};
#pragma omp for private(i, j, temp) nowait schedule(dynamic,1)
		for (i = 0; i < N; i++)
		{
			for (j = i + 1; j < N; j++)
			{
				temp = A[i * N + j];
				A[i * N + j] = A[j * N + i];
				A[j * N + i] = temp;
				iteration_counter[tid]++;
			}
		}
		printf("Tiempo en segundos para el thread %d: %.5f \n", tid, dwalltime() - timetick);
		printf("Iteraciones para %d: %d\n",tid, iteration_counter[tid]);
	}
	printf("Tiempo total: %.5f \n", dwalltime() - time_total);

	//Chequea los resultados
	for (i = 0; i < N; i++)
	{
		for (j = 0; j < N; j++)
		{
			if (i > j)
			{
				check = check && (A[i * N + j] == 0.0);
			}
			else
			{
				check = check && (A[i * N + j] == 1.0);
			}
		}
	}

	if (check)
	{
		printf("Resultado correcto\n");
	}
	else
	{
		printf("Resultado erroneo\n");
	}
	free(A);
	return (0);
}

/*****************************************************************/

#include <sys/time.h>

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	sec = tv.tv_sec + tv.tv_usec / 1000000.0;
	return sec;
}
