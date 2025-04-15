#ifndef __SLAVE_PROCESS_H__
#define __SLAVE_PROCESS_H__

#include "RayTrace.h"
#include "blockOps.h"

void slaveMain( ConfigData *data );

double processStaticBlocks(ConfigData* data, int numBlocks, BlockHeader **headers, float ***pixels);

void packBlocks(double compTime, int numBlocks,
    BlockHeader* blockHeaders,
    float* blockData[], char** buffer, int* position);

#endif
