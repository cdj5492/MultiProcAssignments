#ifndef __SLAVE_PROCESS_H__
#define __SLAVE_PROCESS_H__

#include "RayTrace.h"
#include "blockOps.h"

void slaveMain( ConfigData *data );

void sendBlocks(double compTime, int numBlocks,
                BlockHeader* blockHeaders,
                float* blockData[]);

#endif
