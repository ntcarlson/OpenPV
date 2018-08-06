/*
 * SpikingProbe.cpp
 */

#include "SpikingProbe.hpp"
#include "columns/HyPerCol.hpp"
#include "layers/HyPerLayer.hpp"

namespace PV {

SpikingProbe::SpikingProbe() {
   initialize_base();
}

SpikingProbe::SpikingProbe(const char *name, HyPerCol *hc) {
   initialize_base();
   initialize(name, hc);
}

SpikingProbe::~SpikingProbe() {}

int SpikingProbe::initialize_base() {
   return PV_SUCCESS;
}

int SpikingProbe::initialize(const char *name, HyPerCol *hc) {
   maskLayer = NULL;
   maskLayerStr = NULL;
   return LayerProbe::initialize(name, hc);
}

int SpikingProbe::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = LayerProbe::ioParamsFillGroup(ioFlag);
   ioParam_delay(ioFlag);
   ioParam_period(ioFlag);
   ioParam_numSpike(ioFlag);
   ioParam_seed(ioFlag);
   ioParam_maskLayer(ioFlag);
   return status;
}

void SpikingProbe::ioParam_delay(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "delay", &delay, 0, false);
}

void SpikingProbe::ioParam_period(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "period", &period, 1, false);
}

void SpikingProbe::ioParam_numSpike(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "numSpike", &numSpike, 1, true /* warn if absent */);
}

void SpikingProbe::ioParam_seed(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "seed", &seed, time(NULL));

   int rank;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   srand48_r(seed + rank, &rng_state);
}

void SpikingProbe::ioParam_maskLayer(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamString(ioFlag, name, "maskLayer", &maskLayerStr, NULL);
}

Response::Status
SpikingProbe::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   auto status = LayerProbe::communicateInitInfo(message);
   if (!Response::completed(status)) {
      return status;
   }
   assert(targetLayer);
   
   if (maskLayerStr != NULL) {
      maskLayer = message->lookup<HyPerLayer>(std::string(maskLayerStr));
      if (maskLayer == NULL) {
         if (parent->columnId() == 0) {
            ErrorLog().printf(
                  "%s: layer \"%s\" is not a layer in the column.\n",
                  getDescription_c(),
                  maskLayerStr);
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }


      const PVLayerLoc *mainLoc = getTargetLayer()->getLayerLoc();
      const PVLayerLoc *loc    = maskLayer->getLayerLoc();
      assert(mainLoc != NULL && loc != NULL);
      if (mainLoc->nxGlobal != loc->nxGlobal || mainLoc->nyGlobal != loc->nyGlobal
            || mainLoc->nf != loc->nf) {
         if (parent->columnId() == 0) {
            ErrorLog(errorMessage);
            errorMessage.printf(
                  "%s: Mask layer \"%s\" does not have the same dimensions as targetLayer.\n",
                  getDescription_c(),
                  name);
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }
   }

   return Response::SUCCESS;
}

void SpikingProbe::calcValues(double timevalue) {
}

void SpikingProbe::findActive() {
   MPI_Comm comm = parent->getCommunicator()->communicator();
   int commSize  = parent->getCommunicator()->commSize();
   int rank      = parent->getCommunicator()->commRank();

   PVLayerLoc const *loc = getTargetLayer()->getLayerLoc();
   int const nx          = loc->nx;
   int const ny          = loc->ny;
   int const nf          = loc->nf;
   int const nb          = loc->nbatch;
   PVHalo const *halo    = &loc->halo;
   int const lt          = halo->lt;
   int const rt          = halo->rt;
   int const dn          = halo->dn;
   int const up          = halo->up;

   float const *aBuffer  = getTargetLayer()->getActivity();
   int numNeurons        = getTargetLayer()->getNumNeurons();

   neuronsToSpike.clear();
   neuronsToSpike.resize(numSpike);

   // Find the active neurons on this rank
   vector<int> activeIndices;
   int numActive = 0;
   for (int k = 0; k < numNeurons; k++) {
      int kExt = kIndexExtendedBatch(k, nb, nx, ny, nf, lt, rt, dn, up);
      if (aBuffer[kExt] > 0) {
         activeIndices.push_back(k);
         numActive++;
      }
   }

   // Collect a vector of all active indices on the root rank
   std::vector<int> recvCount(commSize);
   std::vector<int> displacement(commSize);
   MPI_Gather(&numActive, 1, MPI_INT, &recvCount.front(), 1,  MPI_INT, 0, comm);
   int totalActive = 0;
   if (rank == 0) {
      for (int i = 0; i < commSize; i++) {
         displacement[i] = totalActive;
         totalActive += recvCount[i];
      }
      activeIndices.resize(totalActive); 
   }
   MPI_Gatherv(&activeIndices.front(), numActive, MPI_INT,
         &activeIndices.front(), &recvCount.front(), &displacement.front(), MPI_INT,
         0, comm);

   // Randomly select neurons to spike from the list of active ones
   std::vector<int> globalNeuronsToSpike(commSize * numSpike, -1);
   if (rank == 0) {
      std::vector<int> batchCount(commSize);
      int numSpiked = 0;
      while (numSpiked < numSpike && numActive > 0) {
         long randk = 0;
         lrand48_r(&rng_state, &randk);
         randk = randk % totalActive;
         int k = activeIndices[randk];

         // Find which rank this neuron resides on
         int neuronRank = 0;
         while (randk > displacement[neuronRank] && neuronRank < commSize) {
            neuronRank++;
         }
         neuronRank--;

         // Put neuron in array which will be scattered across the ranks
         globalNeuronsToSpike[neuronRank*numSpike + batchCount[neuronRank]] = k;
         batchCount[neuronRank]++;

         activeIndices.erase(activeIndices.begin() + randk);
         numSpiked++;
         totalActive--;
      }
   }

   MPI_Scatter(&globalNeuronsToSpike.front(), numSpike, MPI_INT,
         &neuronsToSpike.front(), numSpike, MPI_INT,
         0, comm);

   int numToSpike = 0;
   while (neuronsToSpike[numToSpike] != -1 && numToSpike < numSpike) {
      numToSpike++;
   }
   neuronsToSpike.resize(numToSpike);
}

void SpikingProbe::findSpikeValues() {
   float *V = getTargetLayer()->getV();
   int rank = parent->getCommunicator()->commRank();

#ifdef PV_USE_CUDA
   if (getTargetLayer()->updatesGpu()) {
      getTargetLayer()->getDeviceV()->copyFromDevice(V);
   }
#endif
   spikeVals.clear();
   for (int neuron : neuronsToSpike) {
      float spikeVal = 2 * V[neuron];
      spikeVals.push_back(spikeVal);
      printf("Holding neuron %d on rank %d at value %f (original = %f)\n",
            neuron, rank, spikeVal, V[neuron]);
   }

#ifdef PV_USE_CUDA
   if (getTargetLayer()->updatesGpu()) {
      getTargetLayer()->getDeviceV()->copyToDevice(V);
   }
#endif
}


void SpikingProbe::applySpikeValues() {
   float *V = getTargetLayer()->getV();
   int rank = parent->getCommunicator()->commRank();

#ifdef PV_USE_CUDA
   if (getTargetLayer()->updatesGpu()) {
      getTargetLayer()->getDeviceV()->copyFromDevice(V);
   }
#endif
   for (int i = 0; i < neuronsToSpike.size(); i++) {
      V[neuronsToSpike[i]] = spikeVals[i];
   }

#ifdef PV_USE_CUDA
   if (getTargetLayer()->updatesGpu()) {
      getTargetLayer()->getDeviceV()->copyToDevice(V);
   }
#endif
}

void SpikingProbe::applyMask() {
   float *V = maskLayer->getV();
   for (int neuron : neuronsToSpike) {
     V[neuron] = 0; 
   }
}

Response::Status SpikingProbe::outputState(double timevalue) {
   float *V = getTargetLayer()->getV();

   long periodStep = ((long) timevalue) % period;
   
   if (periodStep == delay) {
      findActive();
      findSpikeValues();
   } else if (periodStep > delay) {
      applySpikeValues();
      if (maskLayer != NULL) {
         applyMask();
      }
   }
   return Response::SUCCESS;
}


} // end namespace PV
