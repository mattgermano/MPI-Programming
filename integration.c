#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mpi.h>

#define MASTER      0  // Process ID of first process
#define FROM_MASTER 1  // Master message type
#define FROM_WORKER 2  // Worker message type

// Function prototypes
double generate_random(double, double);

int main(int argc, char* argv[])
{
    int process_id;                 // Process identifier
    int num_processes;              // Number of total processes
    int num_workers;                // Number of worker processes
    int destination, source;        // Send/receive identifiers
    int avg_samples, extra_samples; // Number of samples distributed to each worker node

    int i; // Loop variable

    int lower_bound, upper_bound, samples; // User inputs required to calculate the integral

    // Floating point variables used to calculate the integral
    double width;
    double factor; 
    double min_bound, max_bound;
    double start_time, end_time;
    double sum = 0, solution = 0;

    MPI_Status status; // Status for MPI_Recv

    MPI_Init(&argc, &argv);                        // Startup MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);    // Get rank
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes); // Get number of nodes

    num_workers = num_processes - 1; // One process is dedicated to master, all others are workers

    //  Master node
    if (process_id == MASTER)
    {
        start_time = MPI_Wtime(); // Get the program start time
        
        // Store the command line arguments
        lower_bound = atoi(argv[1]);
        upper_bound = atoi(argv[2]);
        samples     = atoi(argv[3]);

        printf("\nLower Integral Bound: %d; Upper Integral Bound: %d; Number of Samples: %d", lower_bound, upper_bound, samples);

        factor = (double) (upper_bound - lower_bound) / samples; // Constant that the sum is multiplied with to calculate h(x)

        if (num_workers == 0) // If there is only one process, the master must calculate the integral on its own
        {
            srand(time(0));

            for (i = 0; i < samples; i++)
            {
                sum += (exp(-pow(generate_random((double)lower_bound, (double)upper_bound), 2) / 2) / sqrt(2 * M_PI));
            }
            solution += sum;
        }
        else // If there is more than one process, divide up the work between each worker node
        {
            avg_samples = samples / num_workers;   // The number of samples split between each worker node
            extra_samples = samples % num_workers; // The number of leftover samples to be distributed to worker nodes
            
            /* The width defines the range of values that each worker node will sample from
            * For example, if the upper bound is 10, the lower bound is 1, and there are 9 workers: the width is 1 ((10-1) / 9)
            * The first worker will sample random numbers from 1 to 2, the second worker will sample from 2 to 3, the third worker will same from 3 to 4, etc...
            * This ensures that each worker does not generate duplicate random numbers as each worker samples from a different range
            */
            width = (double) (upper_bound - lower_bound) / num_workers; 
            min_bound = (double) lower_bound;

            // Loop through each worker node and send the required data
            for (destination = 1; destination <= num_workers; destination++)
            {
                samples = (destination <= extra_samples) ? (avg_samples + 1) : avg_samples; // Evenly distribute the extra samples to each node

                MPI_Send(&samples, 1, MPI_INT, destination, FROM_MASTER, MPI_COMM_WORLD);      // Send the number of samples to each worker node
                MPI_Send(&min_bound, 1, MPI_DOUBLE, destination, FROM_MASTER, MPI_COMM_WORLD); // Send the lower bound that the worker will sample from

                max_bound = min_bound + width; // Update the max bound
                MPI_Send(&max_bound, 1, MPI_DOUBLE, destination, FROM_MASTER, MPI_COMM_WORLD); // Send the max bound that the worker will sample from
                min_bound = max_bound;        // Update the lower bound
            }

            // Loop through each worker node and receive the calculated sum
            for (source = 1; source <= num_workers; source++)
            {
                MPI_Recv(&sum, 1, MPI_DOUBLE, source, FROM_WORKER, MPI_COMM_WORLD, &status); // Receive the sum
                solution += sum;
            }
        }

        solution *= factor;       // Multiply by the constant factor
        end_time = MPI_Wtime();   // Get the end time
        
        printf("\nSolution: %lf\n", solution);                    // Print the estimated integral answer
        printf("Execution Time: %lf\n\n", end_time - start_time); // Print the execution time

    }

    // Worker nodes
    if (process_id > MASTER)
    {
	    srand(time(0)); // Seed the random generator with the current time

        MPI_Recv(&samples, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);      // Receive the number of samples
        MPI_Recv(&min_bound, 1, MPI_DOUBLE, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status); // Receive the lower bound
        MPI_Recv(&max_bound, 1, MPI_DOUBLE, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status); // Receive the upper bound

        // Loop through all samples, generate a random value, and sum the results
        for (i = 0; i < samples; i++)
        {
            sum += (exp(-pow(generate_random(min_bound, max_bound), 2) / 2) / sqrt(2 * M_PI));
        }

        MPI_Send(&sum, 1, MPI_DOUBLE, MASTER, FROM_WORKER, MPI_COMM_WORLD); // Send the sum back to master

    }

    MPI_Finalize(); // Clean up any program allocation
    return 0;
}

double generate_random(double min, double max)
{
    // Generate a random number within the given bounds
    return min + (double)rand() * (max - min) / (double)RAND_MAX;
}
