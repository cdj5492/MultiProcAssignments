//This file contains the code that the slave processes will execute.

#include <iostream>
#include <mpi.h>
#include <cmath>
#include "RayTrace.h"
#include "slave.h"
#include "blockOps.h"

void packBlocks(double compTime, int numBlocks,
    BlockHeader* blockHeaders,
    float* blockData[], char** buffer, int* position) {
    // space for packing
    int packSize = 0, tempSize = 0;
    // compTime (double)
    MPI_Pack_size(1, MPI_DOUBLE, MPI_COMM_WORLD, &tempSize);
    packSize += tempSize;
    // numBlocks (int)
    MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &tempSize);
    packSize += tempSize;

    // std::cout << " sending " << numBlocks << " blocks" << std::endl;

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
    *buffer = new char[packSize];

    // std::cout << " packing " << packSize << " bytes" << std::endl;

    MPI_Pack(&compTime, 1, MPI_DOUBLE, *buffer, packSize, position, MPI_COMM_WORLD);
    MPI_Pack(&numBlocks, 1, MPI_INT, *buffer, packSize, position, MPI_COMM_WORLD);

    for (int i = 0; i < numBlocks; ++i) {
        int headerData[4] = {blockHeaders[i].blockStartX, blockHeaders[i].blockStartY, blockHeaders[i].blockWidth, blockHeaders[i].blockHeight};
        MPI_Pack(headerData, 4, MPI_INT, *buffer, packSize, position, MPI_COMM_WORLD);
        int numPixels = 3 * blockHeaders[i].blockWidth * blockHeaders[i].blockHeight;
        MPI_Pack(blockData[i], numPixels, MPI_FLOAT, *buffer, packSize, position, MPI_COMM_WORLD);
    }
}

void sendBlocks(double compTime, int numBlocks,
        BlockHeader* blockHeaders,
        float* blockData[]) {
    
    int position = 0;
    char* buffer;
    packBlocks(compTime, numBlocks, blockHeaders, blockData, &buffer, &position);

    // std::cout << " sending packed data of size " << position << std::endl;

    // this line blocks, but only for static partitioning for some reason
    // MPI_Isend(buffer, position, MPI_PACKED, 0, 2, MPI_COMM_WORLD, request);
    MPI_Send(buffer, position, MPI_PACKED, 0, 2, MPI_COMM_WORLD);

    // std::cout << " sent packed data" << std::endl;

    delete[] buffer;

    // don't delete the buffer here, since it is still being used by the send.
    // Let the caller free it.
    // return buffer;
}

double processStaticBlocks(ConfigData* data, int numBlocks, BlockHeader **headers, float ***pixels) {
    MPI_Datatype MPI_BlockHeader = create_block_header_type();
    double totalCompTime = 0.0;

    // headers array
    *headers = new BlockHeader[numBlocks];
    // pixel buffer array
    *pixels = new float*[numBlocks];

    // std::cout << "Starting static block processing. Rank:" << data->mpi_rank << std::endl;

    for (int i = 0; i < numBlocks; i++) {
        // get our block assignment from the master
        BlockHeader header;
        MPI_Recv(&header, 1, MPI_BlockHeader, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // store header in array
        (*headers)[i].blockStartX = header.blockStartX;
        (*headers)[i].blockStartY = header.blockStartY;
        (*headers)[i].blockWidth = header.blockWidth;
        (*headers)[i].blockHeight = header.blockHeight;

        // allocate pixel buffer
        int numPixels = 3 * header.blockWidth * header.blockHeight;
        float* pixelBuffer = new float[numPixels];


        double startTime = MPI_Wtime();
        processBlock(data, &header, pixelBuffer);
        double stopTime = MPI_Wtime();
        double compTime = stopTime - startTime;
        totalCompTime += compTime;

        // store pixel buffer in array
        (*pixels)[i] = pixelBuffer;
    }

    // std::cout << "Ending static block processing. Rank:" << data->mpi_rank << std::endl;

    // sendBlocks(totalCompTime, numBlocks, headers, pixels);

    // std::cout << "Rank " << data->mpi_rank << " finished processing all blocks." << std::endl;
    MPI_Type_free(&MPI_BlockHeader);

    return totalCompTime;
}

void processDynamicBlocks(ConfigData* data) {
    // same exact code, but instead of knowing how many blocks we are going to do, we run until 
    // we get a message with tag 3 from the master

    MPI_Datatype MPI_BlockHeader = create_block_header_type();
    MPI_Status status;
    double totalCompTime = 0.0;

    while (true) {
        // get our block assignment from the master
        BlockHeader header;
        MPI_Recv(&header, 1, MPI_BlockHeader, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == 3) {
            // we are done
            break;
        }

        // std::cout << "Rank " << data->mpi_rank << " received block assignment: "
        //         << header.blockStartX << ", " << header.blockStartY << ", "
        //         << header.blockWidth << ", " << header.blockHeight << std::endl;

        // allocate pixel buffer
        int numPixels = 3 * header.blockWidth * header.blockHeight;
        float* pixelBuffer = new float[numPixels];

        double startTime = MPI_Wtime();
        processBlock(data, &header, pixelBuffer);
        double stopTime = MPI_Wtime();
        double compTime = stopTime - startTime;
        totalCompTime += compTime;

        // std::cout << "Rank " << data->mpi_rank << " finished processing block: "
        //         << header.blockStartX << ", " << header.blockStartY << ", "
        //         << header.blockWidth << ", " << header.blockHeight
        //         << " in " << compTime << " seconds" << std::endl;


        sendBlocks(totalCompTime, 1, &header, &pixelBuffer);

        // clean up
        delete[] pixelBuffer;
        // delete[] buffer;
    }

    MPI_Type_free(&MPI_BlockHeader);
}

void slaveMain(ConfigData* data) {
    // should be calculated based on the partitioning scheme
    int numBlocks = 0;

    // Just calculates how many blocks the slave will be processing up front
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //The slave will do nothing since this means sequential operation.
            break;
        case PART_MODE_STATIC_STRIPS_HORIZONTAL:
        {
            numBlocks = 1;
            BlockHeader *headers;
            float **pixels;
            double compTime = processStaticBlocks(data, numBlocks, &headers, &pixels);
            sendBlocks(compTime, numBlocks, headers, pixels);

            delete[] headers;
            for (int i = 0; i < numBlocks; i++) {
                delete[] pixels[i];
            }
            delete[] pixels;

            break;
        }
        case PART_MODE_STATIC_STRIPS_VERTICAL:
        {
            numBlocks = 1;
            BlockHeader *headers;
            float **pixels;
            double compTime = processStaticBlocks(data, numBlocks, &headers, &pixels);
            sendBlocks(compTime, numBlocks, headers, pixels);

            delete[] headers;
            for (int i = 0; i < numBlocks; i++) {
                delete[] pixels[i];
            }
            delete[] pixels;
            break;
        }
        case PART_MODE_STATIC_BLOCKS:
        {
            int blockSize;
            int s = std::floor(std::sqrt(data->mpi_procs));
            if (data->width < data->height) {
                if (s*s + s >= data->mpi_procs) {
                    blockSize = data->width / s;
                } else {
                    blockSize = data->width / (s + 1);
                }
            } else if (data->width > data->height) {
                if (s*s + s >= data->mpi_procs) {
                    blockSize = data->height / s;
                } else {
                    blockSize = data->height / (s + 1);
                }
            } else {
                blockSize = data->width / (std::ceil(std::sqrt(data->mpi_procs)));
            }
            int numBlocksX = data->width / blockSize;
            int numBlocksY = data->height / blockSize;
            int leftoverX = data->width % blockSize;
            int leftoverY = data->height % blockSize;
            int totalBlocks = numBlocksX * numBlocksY 
                            + (leftoverX > 0 ? numBlocksY : 0)
                            + (leftoverY > 0 ? numBlocksX : 0)
                            + ((leftoverX > 0 && leftoverY > 0) ? 1 : 0);
            numBlocks = totalBlocks / data->mpi_procs + (data->mpi_rank < (totalBlocks % data->mpi_procs) ? 1 : 0);

            BlockHeader *headers;
            float **pixels;
            double compTime = processStaticBlocks(data, numBlocks, &headers, &pixels);
            sendBlocks(compTime, numBlocks, headers, pixels);

            delete[] headers;
            for (int i = 0; i < numBlocks; i++) {
                delete[] pixels[i];
            }
            delete[] pixels;
            break;
        }
        case PART_MODE_STATIC_CYCLES_HORIZONTAL:
        {
            // figure out how many rows we are assigned, taking into account
            // the number of rows that are not evenly divisible by the number of processes
            int blockCount = (data->height + data->cycleSize - 1) / data->cycleSize;
            numBlocks = blockCount / data->mpi_procs + (data->mpi_rank < blockCount % data->mpi_procs ? 1 : 0);

            BlockHeader *headers;
            float **pixels;
            double compTime = processStaticBlocks(data, numBlocks, &headers, &pixels);
            sendBlocks(compTime, numBlocks, headers, pixels);

            delete[] headers;
            for (int i = 0; i < numBlocks; i++) {
                delete[] pixels[i];
            }
            delete[] pixels;
            break;
        }
        case PART_MODE_STATIC_CYCLES_VERTICAL:
        {
            // figure out how many columns we are assigned, taking into account
            // the number of columns that are not evenly divisible by the number of processes
            int blockCount = (data->width + data->cycleSize - 1) / data->cycleSize;
            numBlocks = blockCount / data->mpi_procs + (data->mpi_rank < blockCount % data->mpi_procs ? 1 : 0);

            BlockHeader *headers;
            float **pixels;
            double compTime = processStaticBlocks(data, numBlocks, &headers, &pixels);
            sendBlocks(compTime, numBlocks, headers, pixels);

            delete[] headers;
            for (int i = 0; i < numBlocks; i++) {
                delete[] pixels[i];
            }
            delete[] pixels;
            break;
        }
        case PART_MODE_DYNAMIC:
        {
            processDynamicBlocks(data);
            break;
        }
        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }
}

