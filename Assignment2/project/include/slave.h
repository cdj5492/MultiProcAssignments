#ifndef __SLAVE_PROCESS_H__
#define __SLAVE_PROCESS_H__

#include "RayTrace.h"
#include "blockOps.h"

void slaveMain( ConfigData *data );

char* processStaticBlocks(ConfigData* data, int numBlocks, MPI_Request* request);

#endif
