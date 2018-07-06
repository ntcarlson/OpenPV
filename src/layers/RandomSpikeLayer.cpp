/*
 * RescaleLayer.cpp
 */

#include "RandomSpikeLayer.hpp"

#include "../include/default_params.h"

namespace PV {
RandomSpikeLayer::RandomSpikeLayer() { initialize_base(); }

RandomSpikeLayer::RandomSpikeLayer(const char *name, HyPerCol *hc) {
   initialize_base();
   initialize(name, hc);
}

RandomSpikeLayer::~RandomSpikeLayer() { /* free(rescaleMethod);  */}

int RandomSpikeLayer::initialize_base() {
   originalLayer = NULL;
   return PV_SUCCESS;
}

int RandomSpikeLayer::initialize(const char *name, HyPerCol *hc) {
   int status_init = CloneVLayer::initialize(name, hc);

   mLastUpdateTime = -1; // Stops the layer from updating on the first timestep

   return status_init;
}

Response::Status RandomSpikeLayer::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   auto status = CloneVLayer::communicateInitInfo(message);
   // CloneVLayer sets originalLayer and errors out if originalLayerName is not valid
   return status;
}

int RandomSpikeLayer::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   CloneVLayer::ioParamsFillGroup(ioFlag);
   ioParam_spikeValue(ioFlag);
   ioParam_numSpike(ioFlag);
   ioParam_seed(ioFlag);
   return PV_SUCCESS;
}

void RandomSpikeLayer::ioParam_spikeValue(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "spikeValue", &spikeValue, 1.0, true /* warn if absent */);
}

void RandomSpikeLayer::ioParam_numSpike(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "numSpike", &numSpike, 1, true /* warn if absent */);
}

void RandomSpikeLayer::ioParam_seed(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "seed", &seed, time(NULL));
   srand48_r(seed, &rng_state);
}

int RandomSpikeLayer::setActivity() {
   float *activity = clayer->activity->data;
   memset(activity, 0, sizeof(float) * clayer->numExtendedAllBatches);
   return 0;
}


#include <stdio.h>
Response::Status RandomSpikeLayer::updateState(double timef, double dt) {
   Response::Status status = Response::SUCCESS;

   const PVLayerLoc *loc = getLayerLoc();
   float *A              = originalLayer->clayer->activity->data;
   float *V              = originalLayer->getV(); // TODO: V might be on the GPU
   int nx                = loc->nx;
   int ny                = loc->ny;
   int nf                = loc->nf;
   int lt                = loc->halo.lt;
   int rt                = loc->halo.rt;
   int up                = loc->halo.up;
   int dn                = loc->halo.dn;
   int num_neurons       = nx * ny * nf;
   int nbatch            = loc->nbatch;

#ifdef PV_USE_CUDA
// TODO: originalLayer->mUpdateGpu is a protected field
// How can we figure out if we need to sync with the device?
//   if (mUpdateGpu) {
      originalLayer->getDeviceV()->copyFromDevice(V);
//   }
#endif

   // Spike numSpike inactive neurons in each batch
#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for schedule(static)
#endif
   for (int batch = 0; batch < nbatch; batch++) {
      long index = 0;
      int numSpiked = 0;
      float *ABatch = A + batch * ((nx + lt + rt) * (ny + up + dn) * nf);
      while (numSpiked < numSpike) {
          // This code makes the naive assumption that most neurons are inactive
          lrand48_r(&rng_state, &index);
          index = index % num_neurons;
          int index_ext = kIndexExtended(index, nx, ny, nf, lt, rt, dn, up);
          if (ABatch[index_ext] <= 0) {
              fprintf(stderr, "time %f: spiking neuron %ld (%d); previous value = %f; new value = %f\n", timef, index, index_ext, V[index], spikeValue);
              V[index] = spikeValue;
              numSpiked++;
          }
       }
   }

#ifdef PV_USE_CUDA
//   if (mUpdateGpu) {
      originalLayer->getDeviceV()->copyToDevice(V);
//   }
#endif

   return status;
}

} // namespace PV
