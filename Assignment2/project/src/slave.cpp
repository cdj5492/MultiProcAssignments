//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include "RayTrace.h"
#include "slave.h"
#include "blockOps.h"

void slaveMain(ConfigData* data)
{
    // MPI_Status status;
    MPI_Datatype MPI_BlockHeader = create_block_header_type();

    //Depending on the partitioning scheme, different things will happen.
    //You should have a different function for each of the required 
    //schemes that returns some values that you need to handle.
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //The slave will do nothing since this means sequential operation.
            break;
        case PART_MODE_STATIC_STRIPS_HORIZONTAL:
        {
            // assumes it can divide evenly
            int stripHeight = data->height / data->mpi_procs;
            int numVals = 3 * data->width * stripHeight;
            // float* pixels = new float[numVals];

            // get our block assignment from the master
            BlockHeader header;
            MPI_Recv(&header, 1, MPI_BlockHeader, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // allocate pixel buffer
            int numPixels = 3 * header.blockWidth * header.blockHeight;
            float* pixels = new float[numPixels];

            double startTime = MPI_Wtime();
            processBlock(data, &header, pixels);
            double stopTime = MPI_Wtime();
            double compTime = stopTime - startTime;

            int numBlocks = 1;

            // space for packing
            int packSize = 0, tempSize = 0;
            MPI_Pack_size(1, MPI_DOUBLE, MPI_COMM_WORLD, &tempSize);
            packSize += tempSize;
            MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &tempSize);
            packSize += tempSize;
            // pack 4 ints for header of each block
            MPI_Pack_size(4, MPI_INT, MPI_COMM_WORLD, &tempSize);
            packSize += numBlocks * tempSize;
            // pack pixel data
            MPI_Pack_size(numPixels, MPI_FLOAT, MPI_COMM_WORLD, &tempSize);
            packSize += numBlocks * tempSize;

            char* buffer = new char[packSize];
            int position = 0;

            MPI_Pack(&compTime, 1, MPI_DOUBLE, buffer, packSize, &position, MPI_COMM_WORLD);
            MPI_Pack(&numBlocks, 1, MPI_INT, buffer, packSize, &position, MPI_COMM_WORLD);
            int headerData[4] = {header.blockStartX, header.blockStartY, header.blockWidth, header.blockHeight};
            MPI_Pack(headerData, 4, MPI_INT, buffer, packSize, &position, MPI_COMM_WORLD);
            MPI_Pack(pixels, numPixels, MPI_FLOAT, buffer, packSize, &position, MPI_COMM_WORLD);

            MPI_Send(buffer, position, MPI_PACKED, 0, 2, MPI_COMM_WORLD);
            
            // clean up
            delete[] pixels;
            delete[] buffer;

            break;
        }
        case PART_MODE_STATIC_STRIPS_VERTICAL:
        {
            // assumes it can divide evenly
            int stripHeight = data->height / data->mpi_procs;
            int numVals = 3 * data->width * stripHeight;
            // float* pixels = new float[numVals];

            // get our block assignment from the master
            BlockHeader header;
            MPI_Recv(&header, 1, MPI_BlockHeader, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // allocate pixel buffer
            int numPixels = 3 * header.blockWidth * header.blockHeight;
            float* pixels = new float[numPixels];

            double startTime = MPI_Wtime();
            processBlock(data, &header, pixels);
            double stopTime = MPI_Wtime();
            double compTime = stopTime - startTime;

            int numBlocks = 1;

            // space for packing
            int packSize = 0, tempSize = 0;
            MPI_Pack_size(1, MPI_DOUBLE, MPI_COMM_WORLD, &tempSize);
            packSize += tempSize;
            MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &tempSize);
            packSize += tempSize;
            // pack 4 ints for header of each block
            MPI_Pack_size(4, MPI_INT, MPI_COMM_WORLD, &tempSize);
            packSize += numBlocks * tempSize;
            // pack pixel data
            MPI_Pack_size(numPixels, MPI_FLOAT, MPI_COMM_WORLD, &tempSize);
            packSize += numBlocks * tempSize;

            char* buffer = new char[packSize];
            int position = 0;

            MPI_Pack(&compTime, 1, MPI_DOUBLE, buffer, packSize, &position, MPI_COMM_WORLD);
            MPI_Pack(&numBlocks, 1, MPI_INT, buffer, packSize, &position, MPI_COMM_WORLD);
            int headerData[4] = {header.blockStartX, header.blockStartY, header.blockWidth, header.blockHeight};
            MPI_Pack(headerData, 4, MPI_INT, buffer, packSize, &position, MPI_COMM_WORLD);
            MPI_Pack(pixels, numPixels, MPI_FLOAT, buffer, packSize, &position, MPI_COMM_WORLD);

            MPI_Send(buffer, position, MPI_PACKED, 0, 2, MPI_COMM_WORLD);
            
            // clean up
            delete[] pixels;
            delete[] buffer;

            break;
        }
        case PART_MODE_STATIC_BLOCKS:
            break;
        case PART_MODE_STATIC_CYCLES_HORIZONTAL:
            // float* pixels = new float[3 * data->width];

            // get pixels from the master

            // for( int j = 0; j < data->width; ++j ) {
            //     for row in 
            //     int column = j;

            //     //Calculate the index into the array.
            //     int baseIndex = 3 * ( row * data->width + column );

            //     //Call the function to shade the pixel.
            //     shadePixel(&(pixels[baseIndex]),row,j,data);
            // }

            // send processed pixels back

            break;
        case PART_MODE_STATIC_CYCLES_VERTICAL:
            break;
        case PART_MODE_DYNAMIC:
            break;
        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }
}
