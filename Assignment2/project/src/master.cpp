//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>

#include "RayTrace.h"
#include "master.h"
#include "slave.h"

void masterMain(ConfigData* data)
{
    MPI_Status status;

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

            // send out strips to each slave
            for (int i = 0; i < data->mpi_procs; ++i) {
                MPI_Send(&pixels[numVals * i], numVals, MPI_FLOAT, i, 1, MPI_COMM_WORLD);
            }

            // spin up work for the master during this time
            slaveMain(data);

            // Get all the data back from the slaves
            for (int i = 0; i < data->mpi_procs; ++i) {
                MPI_Recv(&pixels[numVals * i], numVals, MPI_FLOAT, i, 1, MPI_COMM_WORLD, &status);
            }

            stopTime = MPI_Wtime();
            break;
        }
        case PART_MODE_STATIC_STRIPS_VERTICAL:
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
