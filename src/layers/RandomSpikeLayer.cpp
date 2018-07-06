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
   ioParam_threshold(ioFlag);
   ioParam_numSpike(ioFlag);
   ioParam_seed(ioFlag);
   ioParam_neuronsToSpike(ioFlag);
   return PV_SUCCESS;
}

void RandomSpikeLayer::ioParam_spikeValue(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "spikeValue", &spikeValue, 1.0, true /* warn if absent */);
}

void RandomSpikeLayer::ioParam_threshold(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "threshold", &threshold, 1.0, true /* warn if absent */);
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
   else if (!strcmp(neuronsToSpike, "aboveThreshold")) {
      neuronsToSpikeType = SPIKE_ABOVE;
   }
   else if (!strcmp(neuronsToSpike, "belowThreshold")) {
      neuronsToSpikeType = SPIKE_BELOW;
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
Response::Status RandomSpikeLayer::spikeAnyBatch(float *VBatch) {
   int numNeurons       = originalLayer->clayer->numNeurons;

   for (int numSpiked = 0; numSpiked < numSpike; numSpiked++) {
      long index;
      lrand48_r(&rng_state, &index);
      index = index % numNeurons;
      fprintf(stderr, "spiking neuron %d with value %f\n", index, VBatch[index]);
      VBatch[index] = spikeValue;
   }
   return Response::SUCCESS;
}

Response::Status RandomSpikeLayer::spikeBelowBatch(float *VBatch) {
   int numNeurons       = originalLayer->clayer->numNeurons;

   int numSpiked = 0;
   while (numSpiked < numSpike) {
      long index;
      lrand48_r(&rng_state, &index);
      index = index % numNeurons;
      if (VBatch[index] <= threshold) {
         // Naively assume that the majority of neurons are below the threshold
         fprintf(stderr, "spiking neuron %d with value %f\n", index, VBatch[index]);
         VBatch[index] = spikeValue;
         numSpiked++;
      }
   }
   return Response::SUCCESS;
}

Response::Status RandomSpikeLayer::spikeAboveBatch(float *VBatch) {
   PVLayerLoc const *loc      = getLayerLoc();
   int numNeurons             = originalLayer->clayer->numNeurons;

   // Find the active neurons
   vector<int> activeIndices;
   int numActive = 0;
   for (int k = 0; k < numNeurons; k++) {
      if (VBatch[k] > threshold) {
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
      fprintf(stderr, "spiking active neuron %d with value %f\n", k, VBatch[k]);
      VBatch[k] = spikeValue;
      activeIndices.erase(activeIndices.begin() + randk);
      numSpiked++;
      numActive--;
   }

   return Response::SUCCESS;
}


Response::Status RandomSpikeLayer::updateState(double timef, double dt) {
   Response::Status status;

   auto *A          = originalLayer->clayer->activity->data;
   int numNeurons   = originalLayer->clayer->numNeurons;
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
      float *VBatch = V + batch * numNeurons;
      fprintf(stderr, "time %f: ", timef);
      switch (neuronsToSpikeType) {
         case SPIKE_ANY:
            status = spikeAnyBatch(VBatch);
            break;
         case SPIKE_BELOW:
            status = spikeBelowBatch(VBatch);
            break;
         case SPIKE_ABOVE:
            status = spikeAboveBatch(VBatch);
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
