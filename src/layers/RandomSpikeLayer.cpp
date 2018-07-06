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

RandomSpikeLayer::~RandomSpikeLayer() {
   free(neuronsToSpike);
}

int RandomSpikeLayer::initialize_base() {
   originalLayer = NULL;
   neuronsToSpike = NULL;
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
   ioParam_neuronsToSpike(ioFlag);
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

void RandomSpikeLayer::ioParam_neuronsToSpike(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamString(
         ioFlag,
         name,
         "neuronsToSpike",
         &neuronsToSpike,
         "any",
         true /*warnIfAbsent*/);
   if (neuronsToSpike == NULL || !strcmp(neuronsToSpike, "")) {
      free(neuronsToSpike);
      neuronsToSpike     = strdup("any");
      neuronsToSpikeType = SPIKE_ANY;
   }
   else if (!strcmp(neuronsToSpike, "onlyActive")) {
      neuronsToSpikeType = SPIKE_ONLYACTIVE;
   }
   else if (!strcmp(neuronsToSpike, "onlyInactive")) {
      neuronsToSpikeType = SPIKE_ONLYINACTIVE;
   }
   else {
      if (parent->columnId() == 0) {
         ErrorLog().printf(
               "%s: neuronsToSpike=\"%s\" is unrecognized.\n",
               getDescription_c(),
               neuronsToSpike);
      }
      MPI_Barrier(parent->getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
}

int RandomSpikeLayer::setActivity() {
   float *activity = clayer->activity->data;
   memset(activity, 0, sizeof(float) * clayer->numExtendedAllBatches);
   return 0;
}

#include <stdio.h>
Response::Status RandomSpikeLayer::spikeAnyBatch(float *ABatch) {
   float *V              = originalLayer->getV();
   const PVLayerLoc *loc = getLayerLoc();
   int nx                = loc->nx;
   int ny                = loc->ny;
   int nf                = loc->nf;
   int lt                = loc->halo.lt;
   int rt                = loc->halo.rt;
   int up                = loc->halo.up;
   int dn                = loc->halo.dn;
   int num_neurons       = nx * ny * nf;
   int nbatch            = loc->nbatch;

   for (int numSpiked = 0; numSpiked < numSpike; numSpiked++) {
      long index;
      lrand48_r(&rng_state, &index);
      index = index % num_neurons;
      fprintf(stderr, "spiking neuron %d with value %f\n", index, V[index]);
      V[index] = spikeValue;
   }
   return Response::SUCCESS;
}

Response::Status RandomSpikeLayer::spikeInactiveBatch(float *ABatch) {
   float *V              = originalLayer->getV();
   const PVLayerLoc *loc = getLayerLoc();
   int nx                = loc->nx;
   int ny                = loc->ny;
   int nf                = loc->nf;
   int lt                = loc->halo.lt;
   int rt                = loc->halo.rt;
   int up                = loc->halo.up;
   int dn                = loc->halo.dn;
   int num_neurons       = nx * ny * nf;
   int nbatch            = loc->nbatch;

   int numSpiked = 0;
   while (numSpiked < numSpike) {
      long index;
      lrand48_r(&rng_state, &index);
      index = index % num_neurons;
      int index_ext = kIndexExtended(index, nx, ny, nf, lt, rt, dn, up);
      if (ABatch[index_ext] <= 0) {
         // Check that the neuron is inactive
         fprintf(stderr, "spiking neuron %d with value %f\n", index, V[index]);
         V[index] = spikeValue;
         numSpiked++;
      }
   }
   return Response::SUCCESS;
}

Response::Status RandomSpikeLayer::spikeActiveBatch(float *ABatch) {
   float *V                   = originalLayer->getV();
   PVLayerLoc const *loc      = getLayerLoc();
   int nxExt                  = loc->nx + loc->halo.lt + loc->halo.rt;
   int nyExt                  = loc->ny + loc->halo.dn + loc->halo.up;
   int nf                     = loc->nf;
   int num_extended           = originalLayer->clayer->numExtended;

   // Find the active neurons
   vector<int> activeIndices;
   int numActive = 0;
   for (int kex = 0; kex < num_extended; kex++) {
      if (ABatch[kex] > 0) {
         // Convert from extended indices
         int x = kxPos(kex, nxExt, nyExt, nf) - loc->halo.lt;
         if (x < 0 or x >= loc->nx) {
            continue;
         }
         int y = kyPos(kex, nxExt, nyExt, nf) - loc->halo.up;
         if (y < 0 or y >= loc->ny) {
            continue;
         }
         x += loc->kx0;
         y += loc->ky0;
         int f = featureIndex(kex, nxExt, nyExt, nf);

         // Get global restricted index.
         int k = (uint32_t)kIndex(x, y, f, loc->nxGlobal, loc->nyGlobal, nf);
         activeIndices.push_back(k);
         numActive++;
      }
   }

   int numSpiked = 0;
   while (numSpiked < numSpike && numActive > 0) {
      long randk = 0;
      lrand48_r(&rng_state, &randk);
      randk = randk % numActive;
      int k = activeIndices[randk];
         
      fprintf(stderr, "numActive = %d", numActive);
      fprintf(stderr, "spiking active neuron %d with value %f\n", k, V[k]);
      V[k] = spikeValue;
      activeIndices.erase(activeIndices.begin() + randk);
      numSpiked++;
      numActive--;
   }

   return Response::SUCCESS;
}


Response::Status RandomSpikeLayer::updateState(double timef, double dt) {
   Response::Status status;

   auto *A          = originalLayer->clayer->activity->data;
   int num_extended = originalLayer->clayer->numExtended;
   float *V         = originalLayer->getV();
   int nbatch       = getLayerLoc()->nbatch;

#ifdef PV_USE_CUDA
   // TODO: originalLayer->mUpdateGpu is a protected field
   // How can we figure out if we need to sync with the device?
   //   if (mUpdateGpu) {
   originalLayer->getDeviceV()->copyFromDevice(V);
   //   }
#endif

#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for schedule(static)
#endif
   for (int batch = 0; batch < nbatch; batch++) {
      float *ABatch = A + batch * num_extended;
      fprintf(stderr, "time %f: ", timef);
      switch (neuronsToSpikeType) {
         case SPIKE_ANY:
            status = spikeAnyBatch(ABatch);
            break;
         case SPIKE_ONLYINACTIVE:
            status = spikeInactiveBatch(ABatch);
            break;
         case SPIKE_ONLYACTIVE:
            status = spikeActiveBatch(ABatch);
            break;
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
