#include "blockOps.h"
#include <cstring>
#include <iostream>

// #define DEBUG_COLORING

void processBlock(ConfigData* data, BlockHeader* header, float* blockData) {
    for( int i = 0; i < header->blockHeight; ++i ) {
        for( int j = 0; j < header->blockWidth; ++j ) {
            int row = i;
            int column = j;

            //Calculate the index into the array.
            int baseIndex = 3 * ( row * header->blockWidth + column );

            // std::cout << "Rank " << data->mpi_rank << " processing global pixel (" << column+header->blockStartX << ", " << row+header->blockStartY 
            //     << ") in block (" << header->blockStartX << ", " << header->blockStartY << ")" << std::endl;

            // raytrace the pixel
            shadePixel(&(blockData[baseIndex]), row + header->blockStartY, column + header->blockStartX,data);

            #ifdef DEBUG_COLORING
            // color based on which rank it is (hsv coloring)
            float hue = (float)data->mpi_rank / (float)data->mpi_procs;
            float saturation = 1.0f;
            float value = 1.0f;
            float r, g, b;
            // HSV to RGB conversion
            float h = hue * 6.0f;
            int i = (int)h;
            float f = h - i;
            float p = value * (1.0f - saturation);
            float q = value * (1.0f - saturation * f);
            float t = value * (1.0f - saturation * (1.0f - f));

            switch (i % 6) {
                case 0: r = value; g = t;     b = p;     break;
                case 1: r = q;     g = value; b = p;     break;
                case 2: r = p;     g = value; b = t;     break;
                case 3: r = p;     g = q;     b = value; break;
                case 4: r = t;     g = p;     b = value; break;
                case 5: r = value; g = p;     b = q;     break;
            }

            // print out colors
            // std::cout << "Rank " << data->mpi_rank << " pixel (" << column+header->blockStartX << ", " << row+header->blockStartY 
            //     << ") color: (" << r << ", " << g << ", " << b << ")" << std::endl;

            // overwrite raytraced pixel with debug color
            blockData[baseIndex] = r;
            blockData[baseIndex + 1] = g;
            blockData[baseIndex + 2] = b;
            #endif
        }
    }
}

// Convenience function to create an MPI datatype for easily
// sending blocks between processes
MPI_Datatype create_block_header_type() {
    MPI_Datatype tmpType, MPI_Block;
    int blockLengths[4] = {1, 1, 1, 1};
    MPI_Aint displacements[4];

    displacements[0] = 0;
    displacements[1] = 1*sizeof(int);
    displacements[2] = 2*sizeof(int);
    displacements[3] = 3*sizeof(int);
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Type_create_struct(4, blockLengths, displacements, types, &tmpType);

    MPI_Aint lb = 0, extent = 4*sizeof(int);
    MPI_Type_create_resized(tmpType, lb, extent, &MPI_Block);
    MPI_Type_commit(&MPI_Block);
    MPI_Type_free(&tmpType);
    return MPI_Block;
}

void cpyBlockToPixels(ConfigData* data, float* pixels, BlockHeader* header, float* blockData) {
    // Copy the block pixel data into the full pixel array.
    for (int i = 0; i < header->blockHeight; ++i) {
        // Source row start in blockPixels.
        int srcStart = 3 * i * header->blockWidth;
        // Destination row start in full image.
        int destRow = header->blockStartY + i;
        int destStart = 3 * (destRow * data->width + header->blockStartX);
        // one row
        memcpy(&pixels[destStart], &blockData[srcStart], sizeof(float) * 3 * header->blockWidth);
    }
}