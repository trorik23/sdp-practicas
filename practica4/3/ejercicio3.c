/*
Resolver la expresión R = AU + LB.
A y B son matrices cuadradas de NxN.
U y L son matrices triangulares superior e inferior, respectivamente.

U al ser el segundo en una multiplicacion de matrices me conviene almacenarla en columnas.
Ucol[i + j(j+1)/2]

Entonces, L me conviene por filas.
Lrow[j + i(i+1)/2]

Tamanio = N(N+1)/2
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

void master(void);
void slave(void);
void multiplyAU(double *);
void multiplyLB_and_acc(double *);
void imprimir(double *m);
int check(double *A, int n);
double dwalltime(void);

unsigned int N; //matris NxN

double *A, *B, *U, *L, *R;

int rank;
int P;

int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &P);

	N = atoi(argv[1]);

	if (rank == 0)
	{
		master();
	}
	else
	{
		slave();
	}

	free(A);
	free(B);
	free(U);
	free(L);
	free(R);
	MPI_Finalize();
	return 0;
}

void master()
{

	A = malloc(sizeof(double) * (N * N));
	B = malloc(sizeof(double) * (N * N));
	U = malloc(sizeof(double) * (N * (N + 1)) / 2);
	L = malloc(sizeof(double) * (N * (N + 1)) / 2);
	R = malloc(sizeof(double) * (N * N));

	for (int i = 0; i < N * N; i++)
	{
		A[i] = i;
		B[i] = i;
	}
	for (int i = 0; i < (N * (N + 1)) / 2; i++)
		U[i] = i;
	for (int i = 0; i < (N * (N + 1)) / 2; i++)
		L[i] = i;

	double start = dwalltime();

	// A.U
	MPI_Scatter(A, N * N / P, MPI_DOUBLE, A, N * N / P, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(U, (N * (N + 1)) / 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	multiplyAU(R);

	// L.B
	int *sendcounts = malloc(sizeof(int) * P);
	int *displs = malloc(sizeof(int) * P);
	displs[0] = 0;
	sendcounts[0] = (N / P) * ((N / P) + 1) / 2;
	for (int i = 1; i < P; i++)
	{
		sendcounts[i] = (N / P * (i + 1)) * ((N / P * (i + 1)) + 1) / 2 - (N / P * i) * ((N / P * i) + 1) / 2;
		displs[i] = displs[i - 1] + sendcounts[i - 1];
	}
	MPI_Scatterv(L, sendcounts, displs, MPI_DOUBLE, L, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(B, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	multiplyLB_and_acc(R);


	MPI_Gather(R, N * N / P, MPI_DOUBLE, R, N * N / P, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	printf("Tardo: %f\n", dwalltime() - start);

	free(sendcounts);
	free(displs);
}

void slave()
{
	
	A = malloc(sizeof(double) * N * N / P);
	U = malloc(sizeof(double) * (N * (N + 1)) / 2);

	int size_L = (N / P * (rank + 1)) * ((N / P * (rank + 1)) + 1) / 2 - (N / P * rank) * ((N / P * rank) + 1) / 2;
	L = malloc(sizeof(double) * size_L);
	B = malloc(sizeof(double) * (N * N));

	R = malloc(sizeof(double) * N * N / P);

	// A.U
	MPI_Scatter(A, N * N / P, MPI_DOUBLE, A, N * N / P, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(U, (N * (N + 1)) / 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	multiplyAU(R);

	// L.B
	int *sendcounts = malloc(sizeof(int) * P);
	int *displs = malloc(sizeof(int) * P);
	MPI_Scatterv(L, sendcounts, displs, MPI_DOUBLE, L, size_L, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	// printf("Rank %d\n", rank);
	// for(int i = 0; i < size_L; i++)
	// 	printf("%.0f ",L[i]);
	// printf("\n");
	MPI_Bcast(B, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	multiplyLB_and_acc(R);

	MPI_Gather(R, N * N / P, MPI_DOUBLE, R, N * N / P, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	free(sendcounts);
	free(displs);
}

void multiplyAU(double *resul)
{
	double acc = 0;
	for (int i = 0; i < N / P; i++) //fila
	{
		for (int j = 0; j < N; j++) //col
		{
			acc = 0;
			for (int k = 0; k < N; k++)
			{
				//chequear si es cero o no
				if (j >= k)
					acc += A[i * N + k] * U[k + j * (j + 1) / 2]; // A por filas y U tringular superior por columnas
			}
			resul[i * N + j] = acc; //x filas
		}
	}
}

void multiplyLB_and_acc(double *resul)
{
	double acc = 0;
	for (int i = 0; i < N / P; i++) //fila
	{
		for (int j = 0; j < N; j++) //col
		{
			acc = 0;
			for (int k = 0; k < N; k++)
			{
				if (k <= i + rank*N/P)
					acc += L[rank*N/P*i + k + (i * (i + 1))/2] * B[k + j * N]; // L tringular inferior por filas y B por columnas
			}
			resul[i * N + j] += acc; //x filas
		}
	}
}

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	sec = tv.tv_sec + tv.tv_usec / 1000000.0;
	return sec;
}

void imprimir(double *m)
{
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			printf("%.0f\t", m[i * N + j]);
		}
		printf("\n");
	}
	printf("\n");
}
int check(double *A, int n)
{
	int ok = 1;
	for (int i = 0; i < n * n; i++)
		ok = ok && (A[i] == n);
	return ok;
}