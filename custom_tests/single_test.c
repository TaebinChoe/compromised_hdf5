#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define FILE_NAME "single_test_file.h5"

int
main(int argc, char **argv)
{
    int mpi_size, mpi_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if (mpi_rank == 0) {
        printf("[Single] Creating file...\n");
    }

    // Use default file access property list for a standard single-process file
    hid_t file = H5Fcreate(FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file < 0) {
        if (mpi_rank == 0) {
            fprintf(stderr, "[Single] Failed to create file\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (mpi_rank == 0) {
        printf("[Single] Closing file...\n");
    }
    H5Fclose(file);

    if (mpi_rank == 0) {
        printf("[Single] Opening file...\n");
    }
    file = H5Fopen(FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0) {
        if (mpi_rank == 0) {
            fprintf(stderr, "[Single] Failed to open file\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (mpi_rank == 0) {
        printf("[Single] Closing file...\n");
    }
    H5Fclose(file);

    if (mpi_rank == 0) {
        printf("[Single] Test completed successfully!\n");
    }

    MPI_Finalize();
    return 0;
}
