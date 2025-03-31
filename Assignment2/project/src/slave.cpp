//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include "RayTrace.h"
#include "slave.h"
#include "blockOps.h"

void slaveMain(ConfigData* data)
{
    MPI_Status status;

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
            float* pixels = new float[numVals];

            // get our pixels from the master
            MPI_Recv(pixels, numVals, MPI_FLOAT, 0, 1, MPI_COMM_WORLD, &status);

            processBlock(data, pixels, data->width, stripHeight);

            // send them back
            MPI_Send(pixels, numVals, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);

            delete[] pixels;
            break;
        }
        case PART_MODE_STATIC_STRIPS_VERTICAL:
            break;
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
