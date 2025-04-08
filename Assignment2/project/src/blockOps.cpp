#include "blockOps.h"
#include <cstring>
#include <iostream>

void processBlock(ConfigData* data, BlockHeader* header, float* blockData) {
    for( int i = 0; i < header->blockHeight; ++i ) {
        for( int j = 0; j < header->blockWidth; ++j ) {
            int row = i;
            int column = j;

            //Calculate the index into the array.
            int baseIndex = 3 * ( row * header->blockWidth + column );

            // std::cout << "Rank " << data->mpi_rank << " processing global pixel (" << column+header->blockStartX << ", " << row+header->blockStartY 
            //     << ") in block (" << header->blockStartX << ", " << header->blockStartY << ")" << std::endl;

            //Call the function to shade the pixel.
            shadePixel(&(blockData[baseIndex]), row + header->blockStartY, column + header->blockStartX,data);
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