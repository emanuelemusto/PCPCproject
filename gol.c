#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>



void seed_line(char *world, int row, int cols, int start_row, int start_col) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < cols; j++) {
            world[i * cols + j] = 'd';
        }
    }

    for (int i = start_col; i < start_col + 3; i++) {
        world[i + start_row * cols] = 'a';
    }
}

void seed_glider(char *world, int row, int cols, int start_row, int start_col) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < cols; j++) {
            world[i * cols + j] = 'd';
        }
    }
	
    world[start_col + start_row * cols] = 'a';
    world[1 + start_col + (start_row + 1) * cols] = 'a';
    seed_line(world, row, cols, start_row + 2, start_col - 1);

}



void seed_shape(char *world, int row, int cols, int start_row, int start_col) {

    for (int i = 0; i < row; i++) {
        for (int j = 0; j < cols; j++) {
            world[i * cols + j] = 'd';
        }
    }

    world[start_col + start_row * cols] = 'a';
    seed_line(world, row, cols, start_row + 1, start_col - 1);
}



void print_world(char *world, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (world[j + i * cols] == 'd') {
                printf("|   ");
            } else {
                printf("| a ");
            }
            if (j == cols - 1) {
                printf("|\n");
            }
        }
    }
    printf("\n\n");
}

int count_neighbors(char *world, int row_idx, int col_idx, int rows, int cols) {
    int count = 0;

    for (int i = row_idx - 1; i <= row_idx + 1; i++) {
        for (int j = col_idx - 1; j <= col_idx + 1; j++) {
            if (i < 0 || j < 0 || i >= rows || j >= cols) {
                continue;
            } else if (i == row_idx && j == col_idx) {
                continue;
            } else if (world[j + i * cols] == 'a') {
                count++;
            }
        }
    }
    return count;
}

void compute_next_round(int me, int nproc, char *world, int rows, int cols) {
    char *world_tmp = malloc(rows * cols * sizeof(char));
    int local_count;
    int a, b;
    if (me == 0) {
        a = 0;
        b = rows - 1;
    } else if (me == nproc - 1) {
        a = 1;
        b = rows;
    } else {
        a = 1;
        b = rows - 1;
    }
	for (int i = a; i < b; i++) {
        for (int j = 0; j < cols; j++) {
            int count = count_neighbors(world, i, j, rows, cols);
            if (world[j + i * cols] == 'a') {
                if (count < 2 || count > 3) {
                    world_tmp[j + i * cols] = 'd';
                } else {
                    world_tmp[j + i * cols] = 'a';
                }
            } else {
                if (count == 3) {
                    world_tmp[j + i * cols] = 'a';
                } else {
                    world_tmp[j + i * cols] = 'd';
                }
            }
        }
    }


    // Gather results from all processes
    char *recv_buf = NULL;
    int recv_count = rows * cols / nproc;
    if (me == 0) {
        recv_buf = malloc(rows * cols * sizeof(char));
    }
	
    MPI_Gather(world_tmp + cols, recv_count, MPI_CHAR, recv_buf, recv_count, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Update world with received data
    if (me == 0) {
        for (int i = 0; i < rows * cols; i++) {
            world[i] = recv_buf[i];
        }
        free(recv_buf);
    }
    free(world_tmp);
}



int main(int argc, char **argv) {
    int rows = 1024;
    int cols = 1024;

    int me, nproc;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    int local_rows = rows / nproc;
	
    char *world = malloc(rows * cols * sizeof(char));
    double T_inizio,T_fine,T_max;



    // Initialize world
    if (me == 0) {
        seed_glider(world, rows, cols, 2, 2);
        // print_world(world, rows, cols);
    }



    // Distribute initial world to all processes
    char *local_world = malloc(local_rows * cols * sizeof(char));
    MPI_Scatter(world + cols, local_rows * cols, MPI_CHAR, local_world, local_rows * cols, MPI_CHAR, 0, MPI_COMM_WORLD);

	// Start timer for times
	T_inizio=MPI_Wtime();
	
    // Compute and update world for each round
    for (int round = 0; round < 100; round++) {
        compute_next_round(me, nproc, local_world, local_rows, cols);
        // Synchronize all processes
        MPI_Barrier(MPI_COMM_WORLD);
        // Gather and update the complete world
        MPI_Gather(local_world, local_rows * cols, MPI_CHAR, world + cols, local_rows * cols, MPI_CHAR, 0, MPI_COMM_WORLD);
        // Print world at the end of each round
        if (me == 0) {
            printf("Round %d:\n", round + 1);
            // print_world(world, rows, cols);
        }
	    
        // Distribute updated world to all processes
        MPI_Scatter(world + cols, local_rows * cols, MPI_CHAR, local_world, local_rows * cols, MPI_CHAR, 0, MPI_COMM_WORLD);

    }

    free(world);
    free(local_world);

    T_fine=MPI_Wtime()-T_inizio; // final time calculator
	if(me==0){
		printf("\nFinal time: %lf\n", T_fine);
	}

    MPI_Finalize();
	
    return 0;
}
