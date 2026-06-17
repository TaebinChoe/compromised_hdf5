#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define FILE_NAME "multi_test_file.h5"
#define DATASET_NAME "numerical_dataset"
#define ARRAY_SIZE 8

int main(int argc, char **argv) {
    int mpi_size, mpi_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if (mpi_size != 4) {
        if (mpi_rank == 0) {
            fprintf(stderr, "This test is designed to run with exactly 4 processes.\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (mpi_rank == 0) {
        printf("[Multi] Initialized MPI with %d processes\n", mpi_size);
    }

    // Set up file access property list with parallel I/O access
    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);

    if (mpi_rank == 0) {
        printf("[Multi] Creating file collectively...\n");
    }
    hid_t file = H5Fcreate(FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
    if (file < 0) {
        fprintf(stderr, "[Rank %d] Failed to create parallel file\n", mpi_rank);
        H5Pclose(fapl_id);
        MPI_Finalize();
        return 1;
    }

    // Create a 1D dataspace of size 8
    hsize_t dims[1] = {ARRAY_SIZE};
    hid_t dataspace = H5Screate_simple(1, dims, NULL);

    // Create a dataset
    hid_t dataset = H5Dcreate2(file, DATASET_NAME, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset < 0) {
        fprintf(stderr, "[Rank %d] Failed to create dataset\n", mpi_rank);
        H5Sclose(dataspace);
        H5Fclose(file);
        H5Pclose(fapl_id);
        MPI_Finalize();
        return 1;
    }

    // Each rank writes 2 elements
    hsize_t count[1] = {2};
    hsize_t start[1] = {(hsize_t)(mpi_rank * 2)};
    H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start, NULL, count, NULL);

    // Memory dataspace
    hid_t memspace = H5Screate_simple(1, count, NULL);

    // Initial data: rank 0 writes {10, 20}, rank 1 writes {30, 40}, etc.
    int write_buf[2] = {(mpi_rank * 2 + 1) * 10, (mpi_rank * 2 + 2) * 10};
    printf("[Rank %d] Writing: [%d, %d] to offset %lld\n", mpi_rank, write_buf[0], write_buf[1], (long long)start[0]);

    // Use collective dataset transfer property list
    hid_t dxpl_id = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(dxpl_id, H5FD_MPIO_COLLECTIVE);

    herr_t status = H5Dwrite(dataset, H5T_NATIVE_INT, memspace, dataspace, dxpl_id, write_buf);
    if (status < 0) {
        fprintf(stderr, "[Rank %d] Write failed\n", mpi_rank);
    }

    // Close dataset and file
    H5Pclose(dxpl_id);
    H5Sclose(memspace);
    H5Dclose(dataset);
    H5Sclose(dataspace);

    if (mpi_rank == 0) {
        printf("[Multi] Closing file... (Mutation patch should trigger now)\n");
    }
    // Set environment variable in application context if needed, but we run with HDF5_USE_FILE_LOCKING=FALSE
    H5Fclose(file);

    // Synchronize to make sure all processes finished closing/mutating
    MPI_Barrier(MPI_COMM_WORLD);

    // Open file again and read data back to verify if it was mutated
    if (mpi_rank == 0) {
        printf("\n[Multi] Opening file again collectively to verify mutation...\n");
    }
    
    file = H5Fopen(FILE_NAME, H5F_ACC_RDONLY, fapl_id);
    if (file >= 0) {
        dataset = H5Dopen2(file, DATASET_NAME, H5P_DEFAULT);
        if (dataset >= 0) {
            int read_buf[ARRAY_SIZE] = {0};
            // Read collectively on rank 0
            if (mpi_rank == 0) {
                status = H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, read_buf);
                if (status >= 0) {
                    printf("[Multi] Read data from file:\n");
                    for (int i = 0; i < ARRAY_SIZE; i++) {
                        printf("  Element[%d]: %d (Original: %d)\n", i, read_buf[i], (i + 1) * 10);
                    }
                } else {
                    printf("[Multi] Read failed\n");
                }
            }
            H5Dclose(dataset);
        }
        H5Fclose(file);
    } else {
        fprintf(stderr, "[Rank %d] Failed to reopen file\n", mpi_rank);
    }

    H5Pclose(fapl_id);
    MPI_Finalize();
    return 0;
}
