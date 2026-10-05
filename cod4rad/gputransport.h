/* GPU acceleration of the radiosity transport trace (-gpu) */

#ifndef GPUTRANSPORT_H
#define GPUTRANSPORT_H

#include "q_shared.h"


extern bool gpuTransportRequested;      /* set by the -gpu option */

/* Uploads the world triangles and BSP to the GPU.  Must be called after
   Geo_BuildCollisionData.  Returns false (after printing why) if the GPU can't
   be used, in which case the CPU path is used instead. */
bool GpuTransport_Init( void );

bool GpuTransport_Enabled( void );

void GpuTransport_Shutdown( void );

#endif
