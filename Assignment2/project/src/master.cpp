//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>

#include "RayTrace.h"
#include "master.h"
#include "slave.h"
#include "blockOps.h"

void masterMain(ConfigData* data)
{
    MPI_Status status;
    MPI_Datatype MPI_BlockHeader = create_block_header_type();

    //Depending on the partitioning scheme, different things will happen.
    //You should have a different function for each of the required 
    //schemes that returns some values that you need to handle.
    
    //Allocate space for the image on the master.
    float* pixels = new float[3 * data->width * data->height];
    
    //Execution time will be defined as how long it takes
    //for the given function to execute based on partitioning
    //type.
    double renderTime = 0.0, startTime, stopTime;

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
            startTime = MPI_Wtime();
            masterSequential(data, pixels);
            stopTime = MPI_Wtime();
            break;
        case PART_MODE_STATIC_STRIPS_HORIZONTAL:
        {
            startTime = MPI_Wtime();

            int stripHeight = data->height / data->mpi_procs;
            int numVals = 3 * data->width * stripHeight;

            MPI_BlockHeader = create_block_header_type();

            // send out strips to each slave
            for (int rank = 0; rank < data->mpi_procs; ++rank) {
                BlockHeader header;
                header.blockStartX = 0;
                header.blockStartY = stripHeight * rank;
                header.blockWidth = data->width;
                header.blockHeight = stripHeight;

                MPI_Send(&header, 1, MPI_BlockHeader, rank, 1, MPI_COMM_WORLD);
            }

            // spin up work for the master during this time using the same slave worker function
            slaveMain(data);

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

                // unpack header and pixel data for each block
                for (int i = 0; i < numBlocks; ++i) {
                    int headerData[4];
                    MPI_Unpack(buffer, messageSize, &position, headerData, 4, MPI_INT, MPI_COMM_WORLD);
                    BlockHeader header;
                    header.blockStartX = headerData[0];
                    header.blockStartY = headerData[1];
                    header.blockWidth = headerData[2];
                    header.blockHeight = headerData[3];

                    int numPixels = 3 * header.blockWidth * header.blockHeight;
                    float* blockData = new float[numPixels];
                    MPI_Unpack(buffer, messageSize, &position, blockData, numPixels, MPI_FLOAT, MPI_COMM_WORLD);

                    // copy into final image
                    cpyBlockToPixels(data, pixels, &header, blockData);
                    delete[] blockData;
                }
                delete[] buffer;
            }

            stopTime = MPI_Wtime();
            break;
        }
        case PART_MODE_STATIC_STRIPS_VERTICAL:
        {
            startTime = MPI_Wtime();

            int stripWidth = data->width / data->mpi_procs;
            int numVals = 3 * data->height * stripWidth;

            MPI_BlockHeader = create_block_header_type();

            // send out strips to each slave
            for (int rank = 0; rank < data->mpi_procs; ++rank) {
                BlockHeader header;
                header.blockStartX = stripWidth * rank;
                header.blockStartY = 0;
                header.blockWidth = stripWidth;
                header.blockHeight = data->height;

                MPI_Send(&header, 1, MPI_BlockHeader, rank, 1, MPI_COMM_WORLD);
            }

            // spin up work for the master during this time using the same slave worker function
            slaveMain(data);

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

                // unpack header and pixel data for each block
                for (int i = 0; i < numBlocks; ++i) {
                    int headerData[4];
                    MPI_Unpack(buffer, messageSize, &position, headerData, 4, MPI_INT, MPI_COMM_WORLD);
                    BlockHeader header;
                    header.blockStartX = headerData[0];
                    header.blockStartY = headerData[1];
                    header.blockWidth = headerData[2];
                    header.blockHeight = headerData[3];

                    int numPixels = 3 * header.blockWidth * header.blockHeight;
                    float* blockData = new float[numPixels];
                    MPI_Unpack(buffer, messageSize, &position, blockData, numPixels, MPI_FLOAT, MPI_COMM_WORLD);

                    // copy into final image
                    cpyBlockToPixels(data, pixels, &header, blockData);
                    delete[] blockData;
                }
                delete[] buffer;
            }
        }
            break;
        case PART_MODE_STATIC_BLOCKS:
            break;
        case PART_MODE_STATIC_CYCLES_HORIZONTAL:
            break;
        case PART_MODE_STATIC_CYCLES_VERTICAL:
            break;
        case PART_MODE_DYNAMIC:
            break;
        default:
            break;
    }

    renderTime = stopTime - startTime;
    std::cout << "Execution Time: " << renderTime << " seconds" << std::endl << std::endl;

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
