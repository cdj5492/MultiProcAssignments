//This file contains the code that the master process will execute.

#include <algorithm>
#include <cmath>
#include <iostream>
#include <mpi.h>

#include "RayTrace.h"
#include "master.h"
#include "slave.h"
#include "blockOps.h"

void masterMain(ConfigData* data)
{
    MPI_Datatype MPI_BlockHeader = create_block_header_type();
    
    //Allocate space for the image on the master.
    float* pixels = new float[3 * data->width * data->height];
    
    //Execution time will be defined as how long it takes
    //for the given function to execute based on partitioning
    //type.
    double renderTime = 0.0, startTime, stopTime;

    startTime = MPI_Wtime();

    //Add the required partitioning methods here in the case statement.
    //You do not need to handle all cases; the default will catch any
    //statements that are not specified. This switch/case statement is the
    //only place that you should be adding code in this function. Make sure
    //that you update the header files with the new functions that will be
    //called.
    //It is suggested that you use the same parameters to your functions as shown
    //in the sequential example below.
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //Call the function that will handle this.
            masterSequential(data, pixels);
            break;
        case PART_MODE_STATIC_STRIPS_HORIZONTAL:
        {
            // rounded up
            int stripHeight = (data->height + data->mpi_procs - 1) / data->mpi_procs;

            // send out strips to each slave
            for (int rank = 0; rank < data->mpi_procs; ++rank) {
                BlockHeader header;
                header.blockStartX = 0;
                header.blockStartY = stripHeight * rank;
                header.blockWidth = data->width;
                // squish last strip to fit
                header.blockHeight = std::min(stripHeight, data->height - header.blockStartY);

                MPI_Send(&header, 1, MPI_BlockHeader, rank, 1, MPI_COMM_WORLD);
            }

            // spin up work for the master during this time using the same slave worker function
            slaveMain(data);
            break;
        }
        case PART_MODE_STATIC_STRIPS_VERTICAL:
        {
            // rounded up
            int stripWidth = (data->width + data->mpi_procs - 1) / data->mpi_procs;

            // send out strips to each slave
            for (int rank = 0; rank < data->mpi_procs; ++rank) {
                BlockHeader header;
                header.blockStartX = stripWidth * rank;
                header.blockStartY = 0;
                // squish last strip to fit
                header.blockWidth = std::min(stripWidth, data->width - header.blockStartX);
                header.blockHeight = data->height;

                MPI_Send(&header, 1, MPI_BlockHeader, rank, 1, MPI_COMM_WORLD);
            }

            // spin up work for the master during this time using the same slave worker function
            slaveMain(data);
            break;
        }
        case PART_MODE_STATIC_BLOCKS:
        {
            // length of one side of a block (square blocks)
            int blockSize = data->width / ((int)std::sqrt(data->mpi_procs));

            // how many whole number blocks can we fit in x and y
            int numBlocksX = data->width / blockSize;
            int numBlocksY = data->height / blockSize;
            // how many pixels are left over in x and y
            int leftoverX = data->width % blockSize;
            int leftoverY = data->height % blockSize;

            int assignRank = 0;

            for (int y = 0; y < numBlocksY; ++y) {
                for (int x = 0; x < numBlocksX; ++x) {
                    BlockHeader header;
                    header.blockStartX = x * blockSize;
                    header.blockStartY = y * blockSize;
                    header.blockWidth = blockSize;
                    header.blockHeight = blockSize;

                    // send out full blocks to each slave
                    MPI_Send(&header, 1, MPI_BlockHeader, assignRank, 1, MPI_COMM_WORLD);

                    assignRank = (assignRank + 1) % data->mpi_procs;
                }
            }

            // send out leftover blocks to each slave
            if (leftoverX > 0) {
                for (int y = 0; y < numBlocksY; ++y) {
                    BlockHeader header;
                    header.blockStartX = numBlocksX * blockSize;
                    header.blockStartY = y * blockSize;
                    header.blockWidth = leftoverX;
                    header.blockHeight = blockSize;

                    MPI_Send(&header, 1, MPI_BlockHeader, assignRank, 1, MPI_COMM_WORLD);

                    assignRank = (assignRank + 1) % data->mpi_procs;
                }
            }
            if (leftoverY > 0) {
                for (int x = 0; x < numBlocksX; ++x) {
                    BlockHeader header;
                    header.blockStartX = x * blockSize;
                    header.blockStartY = numBlocksY * blockSize;
                    header.blockWidth = blockSize;
                    header.blockHeight = leftoverY;

                    MPI_Send(&header, 1, MPI_BlockHeader, assignRank, 1, MPI_COMM_WORLD);

                    assignRank = (assignRank + 1) % data->mpi_procs;
                }
            }

            // send out the last corner block to each slave
            if (leftoverX > 0 && leftoverY > 0) {
                BlockHeader header;
                header.blockStartX = numBlocksX * blockSize;
                header.blockStartY = numBlocksY * blockSize;
                header.blockWidth = leftoverX;
                header.blockHeight = leftoverY;

                MPI_Send(&header, 1, MPI_BlockHeader, assignRank, 1, MPI_COMM_WORLD);
            }

            // spin up work for the master during this time using the same slave worker function
            slaveMain(data);
            break;
        }
        case PART_MODE_STATIC_CYCLES_HORIZONTAL:
        {
            // send out each row to a different slave
            for (int row = 0; row < data->height; ++row) {
                BlockHeader header;
                header.blockStartX = 0;
                header.blockStartY = row;
                header.blockWidth = data->width;
                header.blockHeight = 1;

                MPI_Send(&header, 1, MPI_BlockHeader, row % data->mpi_procs, 1, MPI_COMM_WORLD);
            }

            // spin up work for the master during this time using the same slave worker function
            slaveMain(data);
            break;
        }
        case PART_MODE_STATIC_CYCLES_VERTICAL:
            // send out each column to a different slave
            for (int column = 0; column < data->height; ++column) {
                BlockHeader header;
                header.blockStartX = column;
                header.blockStartY = 0;
                header.blockWidth = 1;
                header.blockHeight = data->height;

                MPI_Send(&header, 1, MPI_BlockHeader, column % data->mpi_procs, 1, MPI_COMM_WORLD);
            }

            // spin up work for the master during this time using the same slave worker function
            slaveMain(data);

            break;
        case PART_MODE_DYNAMIC:
        {
            // how many whole number blocks can we fit in x and y
            int numBlocksX = data->width / data->dynamicBlockWidth;
            int numBlocksY = data->height / data->dynamicBlockHeight;
            // how many pixels are left over in x and y
            int leftoverX = data->width % data->dynamicBlockWidth;
            int leftoverY = data->height % data->dynamicBlockHeight;

            int assignRank = 0;

            for (int y = 0; y < numBlocksY; ++y) {
                for (int x = 0; x < numBlocksX; ++x) {
                    BlockHeader header;
                    header.blockStartX = x * data->dynamicBlockWidth;
                    header.blockStartY = y * data->dynamicBlockHeight;
                    header.blockWidth = data->dynamicBlockWidth;
                    header.blockHeight = data->dynamicBlockHeight;

                    // send out full blocks to each slave
                    MPI_Send(&header, 1, MPI_BlockHeader, assignRank+1, 1, MPI_COMM_WORLD);

                    assignRank = (assignRank + 1) % (data->mpi_procs - 1);
                }
            }

            // send out leftover blocks to each slave
            if (leftoverX > 0) {
                for (int y = 0; y < numBlocksY; ++y) {
                    BlockHeader header;
                    header.blockStartX = numBlocksX * data->dynamicBlockWidth;
                    header.blockStartY = y * data->dynamicBlockHeight;
                    header.blockWidth = leftoverX;
                    header.blockHeight = data->dynamicBlockHeight;

                    MPI_Send(&header, 1, MPI_BlockHeader, assignRank+1, 1, MPI_COMM_WORLD);

                    assignRank = (assignRank + 1) % (data->mpi_procs - 1);
                }
            }
            if (leftoverY > 0) {
                for (int x = 0; x < numBlocksX; ++x) {
                    BlockHeader header;
                    header.blockStartX = x * data->dynamicBlockWidth;
                    header.blockStartY = numBlocksY * data->dynamicBlockHeight;
                    header.blockWidth = data->dynamicBlockWidth;
                    header.blockHeight = leftoverY;

                    MPI_Send(&header, 1, MPI_BlockHeader, assignRank+1, 1, MPI_COMM_WORLD);

                    assignRank = (assignRank + 1) % (data->mpi_procs - 1);
                }
            }

            // send out the last corner block to each slave
            if (leftoverX > 0 && leftoverY > 0) {
                BlockHeader header;
                header.blockStartX = numBlocksX * data->dynamicBlockWidth;
                header.blockStartY = numBlocksY * data->dynamicBlockHeight;
                header.blockWidth = leftoverX;
                header.blockHeight = leftoverY;

                MPI_Send(&header, 1, MPI_BlockHeader, assignRank+1, 1, MPI_COMM_WORLD);
            }

            // don't run anything else on the master
            break;
        }
        default:
            break;
    }

    double largestCompTime = 0;

    // Get all the data back from the slaves
    for (int rank = 0; rank < data->mpi_procs; ++rank) {
        MPI_Status status;
        MPI_Probe(rank, 2, MPI_COMM_WORLD, &status);
        int messageSize = 0;
        MPI_Get_count(&status, MPI_PACKED, &messageSize);
        char* buffer = new char[messageSize];
        MPI_Recv(buffer, messageSize, MPI_PACKED, rank, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // begin unpacking
        int position = 0;
        double compTime;
        int numBlocks;
        MPI_Unpack(buffer, messageSize, &position, &compTime, 1, MPI_DOUBLE, MPI_COMM_WORLD);
        MPI_Unpack(buffer, messageSize, &position, &numBlocks, 1, MPI_INT, MPI_COMM_WORLD);

        // for debug, just print out the compTime
        // std::cout << "Rank " << rank << " computation time: " << compTime << std::endl;

        if (compTime > largestCompTime) {
            largestCompTime = compTime;
        }

        // unpack header and pixel data for each block
        for (int i = 0; i < numBlocks; ++i) {
            int headerData[4];
            MPI_Unpack(buffer, messageSize, &position, headerData, 4, MPI_INT, MPI_COMM_WORLD);
            BlockHeader header;
            header.blockStartX = headerData[0];
            header.blockStartY = headerData[1];
            header.blockWidth = headerData[2];
            header.blockHeight = headerData[3];

            // print block info, including which rank it came from
            // std::cout << "Received block from rank " << rank << ": " << header.blockStartX << ", " << header.blockStartY << ", " << header.blockWidth << ", " << header.blockHeight << std::endl;

            int numPixels = 3 * header.blockWidth * header.blockHeight;

            // if the block is contiguous in memory, don't use a seperate blockData buffer.
            // Helps speed up readback if we don't need to copy here
            if (header.blockWidth == data->width || header.blockHeight == 1) {
                MPI_Unpack(buffer, messageSize, &position, &(pixels[3*(header.blockStartY*data->width + header.blockStartX)]), numPixels, MPI_FLOAT, MPI_COMM_WORLD);
            } else {
                float* blockData = new float[numPixels];
                MPI_Unpack(buffer, messageSize, &position, blockData, numPixels, MPI_FLOAT, MPI_COMM_WORLD);

                // copy into final image
                cpyBlockToPixels(data, pixels, &header, blockData);
                delete[] blockData;
            }
        }
        delete[] buffer;
    }

    stopTime = MPI_Wtime();
    renderTime = stopTime - startTime;


    std::cout << "Total execution Time: " << renderTime << " seconds" << std::endl << std::endl;

    std::cout << "Largest computation time: " << largestCompTime << " seconds" << std::endl;

    double communicationTime = renderTime - largestCompTime;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / largestCompTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;

    //After this gets done, save the image.
    std::cout << "Image will be save to: ";
    std::string file = "renders/" + generateFileName();
    std::cout << file << std::endl;
    savePixels(file, pixels, data);

    //Delete the pixel data.
    delete[] pixels; 
}

void masterSequential(ConfigData* data, float* pixels)
{
    //Start the computation time timer.
    double computationStart = MPI_Wtime();

    //Render the scene.
    for( int i = 0; i < data->height; ++i )
    {
        for( int j = 0; j < data->width; ++j )
        {
            int row = i;
            int column = j;

            //Calculate the index into the array.
            int baseIndex = 3 * ( row * data->width + column );

            //Call the function to shade the pixel.
            shadePixel(&(pixels[baseIndex]),row,j,data);
        }
    }

    //Stop the comp. timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    //After receiving from all processes, the communication time will
    //be obtained.
    double communicationTime = 0.0;

    //Print the times and the c-to-c ratio
	//This section of printing, IN THIS ORDER, needs to be included in all of the
	//functions that you write at the end of the function.
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}
