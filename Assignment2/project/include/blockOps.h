#ifndef __BLOCK_OPS__
#define __BLOCK_OPS__

#include "RayTrace.h"
#include <mpi.h>

// information about a single rectangular image block
typedef struct {
    int blockStartX;
    int blockStartY;
    int blockWidth;
    int blockHeight;
} BlockHeader;


MPI_Datatype create_block_header_type();

void processBlock(ConfigData* data, BlockHeader* header, float* blockData, bool debugColoring);

void cpyBlockToPixels(ConfigData* data, float* pixels, BlockHeader* header, float* blockData);
void cpyPixelsToBlock(ConfigData* data, float* pixels, BlockHeader* header, float* blockData);

#endif