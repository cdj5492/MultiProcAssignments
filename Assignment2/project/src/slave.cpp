//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include <cmath>
#include "RayTrace.h"
#include "slave.h"
#include "blockOps.h"

void slaveMain(ConfigData* data)
{
    MPI_Datatype MPI_BlockHeader = create_block_header_type();

    // should be calculated based on the partitioning scheme
    int numBlocks = 0;

    // Just calculates how many blocks the slave will be processing up front
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //The slave will do nothing since this means sequential operation.
            break;
        case PART_MODE_STATIC_STRIPS_HORIZONTAL:
            numBlocks = 1;
            break;
        case PART_MODE_STATIC_STRIPS_VERTICAL:
            numBlocks = 1;
            break;
        case PART_MODE_STATIC_BLOCKS:
        {
            int blockSize = data->width / ((int)std::sqrt(data->mpi_procs));
            int numBlocksX = data->width / blockSize;
            int numBlocksY = data->height / blockSize;
            int leftoverX = data->width % blockSize;
            int leftoverY = data->height % blockSize;
            int totalBlocks = numBlocksX * numBlocksY 
                            + (leftoverX > 0 ? numBlocksY : 0)
                            + (leftoverY > 0 ? numBlocksX : 0)
                            + ((leftoverX > 0 && leftoverY > 0) ? 1 : 0);
            numBlocks = totalBlocks / data->mpi_procs + (data->mpi_rank < (totalBlocks % data->mpi_procs) ? 1 : 0);
            break;
        }
        case PART_MODE_STATIC_CYCLES_HORIZONTAL:
            // figure out how many rows we are assigned, taking into account
            // the number of rows that are not evenly divisible by the number of processes
            numBlocks = data->height / data->mpi_procs + (data->mpi_rank % data->mpi_procs < data->height % data->mpi_procs ? 1 : 0);
            break;
        case PART_MODE_STATIC_CYCLES_VERTICAL:
            // figure out how many columns we are assigned, taking into account
            // the number of columns that are not evenly divisible by the number of processes
            numBlocks = data->width / data->mpi_procs + (data->mpi_rank % data->mpi_procs < data->width % data->mpi_procs ? 1 : 0);
            break;
        case PART_MODE_DYNAMIC:
        {
            int numBlocksX = data->width / data->dynamicBlockWidth;
            int numBlocksY = data->height / data->dynamicBlockHeight;
            int leftoverX = data->width % data->dynamicBlockWidth;
            int leftoverY = data->height % data->dynamicBlockHeight;
            int totalBlocks = numBlocksX * numBlocksY
                            + (leftoverX > 0 ? numBlocksY : 0)
                            + (leftoverY > 0 ? numBlocksX : 0)
                            + ((leftoverX > 0 && leftoverY > 0) ? 1 : 0);
            int workerCount = data->mpi_procs - 1;  // workers are ranks 1..mpi_procs-1
            int workerID = data->mpi_rank - 1;      // adjust since rank 0 is master
            numBlocks = totalBlocks / workerCount + (workerID < (totalBlocks % workerCount) ? 1 : 0);
            break;
        }
        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }

    double totalCompTime = 0.0;

    // headers array
    BlockHeader* headers = new BlockHeader[numBlocks];
    // pixel buffers array
    float** pixels = new float*[numBlocks];

    for (int i = 0; i < numBlocks; i++) {
        // get our block assignment from the master
        BlockHeader header;
        MPI_Recv(&header, 1, MPI_BlockHeader, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // store header in array
        headers[i].blockStartX = header.blockStartX;
        headers[i].blockStartY = header.blockStartY;
        headers[i].blockWidth = header.blockWidth;
        headers[i].blockHeight = header.blockHeight;

        // allocate pixel buffer
        int numPixels = 3 * header.blockWidth * header.blockHeight;
        float* pixelBuffer = new float[numPixels];


        double startTime = MPI_Wtime();
        processBlock(data, &header, pixelBuffer);
        double stopTime = MPI_Wtime();
        double compTime = stopTime - startTime;
        totalCompTime += compTime;

        // store pixel buffer in array
        pixels[i] = pixelBuffer;
    }

    sendBlocks(totalCompTime, numBlocks, headers, pixels);

    // clean up
    for (int i = 0; i < numBlocks; i++) {
        delete[] pixels[i];
    }
    delete[] pixels;
    delete[] headers;
}


void sendBlocks(double compTime, int numBlocks,
        BlockHeader* blockHeaders,
        float* blockData[]) {
    // space for packing
    int packSize = 0, tempSize = 0;
    // compTime (double)
    MPI_Pack_size(1, MPI_DOUBLE, MPI_COMM_WORLD, &tempSize);
    packSize += tempSize;
    // numBlocks (int)
    MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &tempSize);
    packSize += tempSize;

    // loop through each block
    for (int i = 0; i < numBlocks; ++i) {
        // pack 4 ints for header of each block
        MPI_Pack_size(4, MPI_INT, MPI_COMM_WORLD, &tempSize);
        packSize += tempSize;
        // pack pixel data
        int numPixels = 3 * blockHeaders[i].blockWidth * blockHeaders[i].blockHeight;
        MPI_Pack_size(numPixels, MPI_FLOAT, MPI_COMM_WORLD, &tempSize);
        packSize += tempSize;
    }

    // alloc buffer for packed data
    char* buffer = new char[packSize];
    int position = 0;

    MPI_Pack(&compTime, 1, MPI_DOUBLE, buffer, packSize, &position, MPI_COMM_WORLD);
    MPI_Pack(&numBlocks, 1, MPI_INT, buffer, packSize, &position, MPI_COMM_WORLD);

    for (int i = 0; i < numBlocks; ++i) {
        int headerData[4] = {blockHeaders[i].blockStartX, blockHeaders[i].blockStartY, blockHeaders[i].blockWidth, blockHeaders[i].blockHeight};
        MPI_Pack(headerData, 4, MPI_INT, buffer, packSize, &position, MPI_COMM_WORLD);
        int numPixels = 3 * blockHeaders[i].blockWidth * blockHeaders[i].blockHeight;
        MPI_Pack(blockData[i], numPixels, MPI_FLOAT, buffer, packSize, &position, MPI_COMM_WORLD);
    }

    MPI_Send(buffer, position, MPI_PACKED, 0, 2, MPI_COMM_WORLD);

    delete[] buffer;
}