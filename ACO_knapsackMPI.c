#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <mpi.h>


#define FALSE 0
#define TRUE 1


struct Solution {
	int procid;
	int profit;
};


void max_solution(struct Solution *in, struct Solution *inout, int *len, MPI_Datatype *dptr) {
	int i;
	
	for (i = 0; i < *len; i++) {
		if (in->profit > inout->profit) {
			inout[i] = in[i];
		}
	}
}


MPI_Op solution_max_op; 
MPI_Datatype solution_type;


int pick_item(const double* probabilities, const int item_count)
{
	double prob = rand() / 1.0 / RAND_MAX;
	double sum = 0;
	int i;
	for (i = 0; i < item_count; i++) {
		sum += probabilities[i];
		if (prob < sum) {
			return i;
		}
	}
	fprintf(stderr, "Index out of bound.\n");
	exit(-1);
}


void ACO_knapsack(
	int const* profits, 
	const int** knapsack_weights_matrix,
	int const* knapsack_weights_capacities,
	const char** knapsack_weights_names,
	const int item_count,
	const int knapsack_weights_count,
	const int procid,
	const int proccount
) {
	double *atractiveness, *pheromones, *probabilities;
	unsigned char *is_in_knapsack, *best_local_solution;
	int *current_knapsack_weights_capacities;
	struct Solution best_global_solution;
	unsigned char* best_global_is_in_knapsack;
	
	unsigned char is_load_capacity_exided;
	double sum, prod;
	int last_best_profit, counter;
	int best_local_profit;
	int item;
	int profit;
	int i, j, k, m;
	int id;
	int iteration_counter = 0;
	
	
	best_global_is_in_knapsack = (unsigned char*)calloc(item_count , sizeof(unsigned char));
			
	if (!best_global_is_in_knapsack) {
		fprintf(stderr, "Failed to allocate memory!\n");
		exit(-1);
	}
			
	last_best_profit = 0;
	counter = 0;
	
	atractiveness = (double*)malloc(item_count * sizeof(double));
	pheromones 	= (double*)calloc(item_count, sizeof(double));
	probabilities = (double*)malloc(item_count * sizeof(double));
	is_in_knapsack = (unsigned char*)calloc(item_count, sizeof(unsigned char));
	best_local_solution = (unsigned char*)calloc(item_count, sizeof(unsigned char));
	current_knapsack_weights_capacities = (int*)malloc(knapsack_weights_count * sizeof(int));
		
	if (!atractiveness || !pheromones || !probabilities || !is_in_knapsack || !best_local_solution || !current_knapsack_weights_capacities)
	{
		fprintf(stderr, "Failed to allocate memory!\n");
		exit(-1);
	}
	
	const double const_pheromone = 1;
	const double d_pheromone = 0.1;
	const double evaporation = 0.05;
	const int ant_workers = 100;
	const int patience = 400;
	
	srand((int)time(NULL) ^ (procid+1));

	best_global_solution.profit = 0;
	
	if (!procid) {
		for (i = 0; i < item_count; i++)
		{
			printf("%d,", i);
		}
		printf("Profit,True Profit,Weight,Patience,Iteration\n");
	}

	while (counter < patience) {
		best_local_profit = 0;
	

		for (j = 0; j < ant_workers / proccount; j++)
		{
			for (k = 0; k < knapsack_weights_count; k++) {
				current_knapsack_weights_capacities[k] = knapsack_weights_capacities[k];
			}
			
			profit = 0;
			for (i = 0; i < item_count; i++) {
				is_in_knapsack[i] = FALSE;
			}
			
			for (k = 0; k < item_count; k++)
			{
				sum = 0;
				for (i = 0; i < item_count; i++) {
					if (is_in_knapsack[i])
						continue;
					
					prod = 1;
					is_load_capacity_exided = FALSE;
					
					for (m = 0; m < knapsack_weights_count; m++) {
						if (knapsack_weights_matrix[m][i] > current_knapsack_weights_capacities[m]) {
							is_load_capacity_exided = TRUE;
							break;
						}
						prod *= knapsack_weights_matrix[m][i] / 1.0 / current_knapsack_weights_capacities[m];
					}
					
					if (is_load_capacity_exided) {
						probabilities[i] = 0;
						continue;
					} else {
						probabilities[i] = 1;
					}
					
					atractiveness[i] = profits[i] / 1.0 / prod;
					sum += atractiveness[i] * (const_pheromone + pheromones[i]);
				}
				
				// check if there is any item left that can fit into the knapsack
				if (sum == 0)
					break;
				
				for (i = 0; i < item_count; i++) {
					if (probabilities[i] && !is_in_knapsack[i]) {
						probabilities[i] = (atractiveness[i] * (const_pheromone + pheromones[i])) / sum;
					} else {
						probabilities[i] = 0;
					}
				}
				
				item = pick_item(probabilities, item_count);
				profit += profits[item];
				for (m = 0; m < knapsack_weights_count; m++) {
					current_knapsack_weights_capacities[m] -= knapsack_weights_matrix[m][item];
				}
				is_in_knapsack[item] = TRUE;
			}
			
			if (profit > best_local_profit) {
				best_local_profit = profit;
				for (i = 0; i < item_count; i++) {
					best_local_solution[i] = is_in_knapsack[i];
				}
			}
		}
		
		unsigned char new_best = FALSE;
		if (best_local_profit > best_global_solution.profit) {
			new_best |= TRUE;
			best_global_solution.profit = best_local_profit;
			best_global_solution.procid = procid;
			for (i = 0; i < item_count; i++) {
				best_global_is_in_knapsack[i] = best_local_solution[i];
			}
		}
		
		struct Solution out_solution;
		MPI_Allreduce(&best_global_solution, &out_solution, 1, solution_type, solution_max_op, MPI_COMM_WORLD);
		if (out_solution.profit > best_global_solution.profit) {
			new_best |= TRUE;
		}
		
		best_global_solution = out_solution;
		
		counter = new_best ? 0 : counter + 1;
	
		MPI_Request request;
		if (procid == best_global_solution.procid) {
			for (i = 0; i < item_count; i++) {
				pheromones[i] -= pheromones[i] * evaporation;
				if (best_global_is_in_knapsack[i]) {
					pheromones[i] += d_pheromone;
				}
			}
			
			MPI_Ibcast(pheromones, item_count, MPI_DOUBLE, best_global_solution.procid, MPI_COMM_WORLD, &request);
			
			// print solution
			sum = 0;
			for (j = 0; j < knapsack_weights_count; j++) {
				current_knapsack_weights_capacities[j] = 0;
			}
			
			for (i = 0; i < item_count; i++) {
				if (best_global_is_in_knapsack[i]) {
					printf("1,", i);
					for (j = 0; j < knapsack_weights_count; j++) {
						current_knapsack_weights_capacities[j] += knapsack_weights_matrix[j][i];
					}
					sum += profits[i];
				} else {
					printf("0,", i);
				}
			}

			printf("%d,%d,", best_global_solution.profit, (int)sum);
			
			for (i = 0; i < knapsack_weights_count; i++) {
				printf("%d,", current_knapsack_weights_capacities[i]);
			}
			printf("%d,%d\n", patience - counter, iteration_counter);
			fflush(stdout);
			iteration_counter++;
		} else {
			MPI_Ibcast(pheromones, item_count, MPI_DOUBLE, best_global_solution.procid, MPI_COMM_WORLD, &request);
			
		}
		MPI_Barrier(MPI_COMM_WORLD);
		
		/*for (i = 0; i < item_count; i++) {
			if (pheromones[i] > 0.001f) {
				printf("#%d p[%d]:%f ", procid, i, pheromones[i]);
			}
		}
		printf("\n");*/
	}
	
	free(atractiveness);		
	free(pheromones);
	free(probabilities);
	free(is_in_knapsack);
	free(best_local_solution);
	free(current_knapsack_weights_capacities);
	free(best_global_is_in_knapsack);
	
	if (procid == 0) {
		fprintf(stderr, "Out of patience!\n");
	}
}


int main(int argc, char** argv)
{
	int proccount, procid;
	
	MPI_Init(&argc, &argv);
	
	MPI_Comm_size(MPI_COMM_WORLD, &proccount);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &procid);
	
	if (!procid) {
		fprintf(stderr, "Number of processes: %d\n", proccount);
	}
	
	MPI_Type_contiguous(2, MPI_INT, &solution_type); 
	MPI_Type_commit(&solution_type);
	
	MPI_Op_create(&max_solution, TRUE, &solution_max_op); 
	
	srand(1234);
	const char *names[] = { "weight" };
	
	int n = 2000;
	const int m = sizeof(names) / sizeof(*names);
	
	int profits[n];
	int *knapsack_weights_matrix[sizeof(names) / sizeof(*names)];
	int knapsack_weights_capacities[sizeof(names) / sizeof(*names)];
	int i, j;
	
	for (i = 0; i < m; i++) {
		knapsack_weights_matrix[i] = (int*)malloc(sizeof(int) * n);
		if (!knapsack_weights_matrix[i]) {
			fprintf(stderr, "Failed to allocate memory!\n");
			exit(-1);
		}
	}
	
	/*for (i = 0; i < n; i++) {
		profits[i] = rand() / 1.0 / RAND_MAX * 99 + 1;
		for (j = 0; j < m; j++) {
			knapsack_weights_matrix[j][i] = rand() / 1.0 / RAND_MAX * 9 + 1;
		}
	}*/
	
	FILE *f_c, *f_w, *f_p;
	f_c = fopen(argv[1], "r");
	if (!f_c) {
		fprintf(stderr, "Cannot open file %s with capacity!", argv[1]);
		exit(-1);
	}
	
	f_w = fopen(argv[2], "r");
	if (!f_w) {
		fprintf(stderr, "Cannot open file %s with weights!", argv[2]);
		exit(-1);
	}
	
	f_p = fopen(argv[3], "r");
	if (!f_p) {
		fprintf(stderr, "Cannot open file %s with profits!", argv[3]);
		exit(-1);
	}
	
	char buffer[256];
	
	fgets(buffer, sizeof(buffer), f_c);
	sscanf(buffer, "%d", knapsack_weights_capacities);
	
	for (i = 0; fgets(buffer, sizeof(buffer), f_w); i++) {
		sscanf(buffer, "%d", &knapsack_weights_matrix[0][i]);
	}
	n = i;
	
	for (i = 0; fgets(buffer, sizeof(buffer), f_p); i++) {
		sscanf(buffer, "%d", &profits[i]);
	}
	
	fclose(f_c);
	fclose(f_w);
	fclose(f_p);
	
	if (i != n) {
		fprintf(stderr, "Different number of profits and weights!");
		exit(-1);
	}
	
	ACO_knapsack(profits, (const int**)knapsack_weights_matrix, knapsack_weights_capacities, names, n, m, procid, proccount);
	
	for (i = 0; i < m; i++) {
		free(knapsack_weights_matrix[i]);
	}
	
	return 0;
}
