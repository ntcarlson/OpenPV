/*
 * RescaleLayer.cpp
 */

#include "NoiseLayer.hpp"
#include <stdio.h>

#include "../include/default_params.h"

namespace PV {
NoiseLayer::NoiseLayer() { initialize_base(); }

NoiseLayer::NoiseLayer(const char *name, HyPerCol *hc) {
   initialize_base();
   initialize(name, hc);
}

NoiseLayer::~NoiseLayer() { /* free(rescaleMethod);  */}

int NoiseLayer::initialize_base() {
   originalLayer = NULL;
   stdDev = 0;
   return PV_SUCCESS;
}

int NoiseLayer::initialize(const char *name, HyPerCol *hc) {
   int status_init = CloneVLayer::initialize(name, hc);

   return status_init;
}

Response::Status NoiseLayer::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   auto status = CloneVLayer::communicateInitInfo(message);
   // CloneVLayer sets originalLayer and errors out if originalLayerName is not valid
   return status;
}

// Noise layer does not use the V buffer, so absolutely fine to clone off of an null V layer
void NoiseLayer::allocateV() {
   // Do nothing
}

int NoiseLayer::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   CloneVLayer::ioParamsFillGroup(ioFlag);
   ioParam_stdDev(ioFlag);
   return PV_SUCCESS;
}

void NoiseLayer::ioParam_stdDev(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "stdDev", &stdDev, stdDev);
}

int NoiseLayer::setActivity() {
   float *activity = clayer->activity->data;
   memset(activity, 0, sizeof(float) * clayer->numExtendedAllBatches);
   return 0;
}

double NoiseLayer::rand_gaussian(double mean, double sdev){
    const double epsilon = std::numeric_limits<double>::min();
    const double two_pi = 2.0*3.14159265358979323846;

    static double z0, z1;
    static bool generate;


    generate = !generate;

    if (!generate){
	    return z1 * sdev + mean;
    }
    double u1, u2;
    do {
    	// TODO: CHANGE
    	u1 = double(rand()) * (1.0 / RAND_MAX);
    	u2 = double(rand()) * (1.0 / RAND_MAX);
    } while ( u1 <= epsilon );

    z0 = sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
    z1 = sqrt(-2.0 * log(u1)) * sin(two_pi * u2);


    return z0 * sdev + mean;

}


#include <stdio.h>

Response::Status NoiseLayer::updateState(double timef, double dt) {
    Response::Status status = Response::SUCCESS;

   int numNeurons                = originalLayer->getNumNeurons();
   float *A                      = clayer->activity->data;
   const float *originalA        = originalLayer->getCLayer()->activity->data;
   const PVLayerLoc *loc         = getLayerLoc();
   const PVLayerLoc *locOriginal = originalLayer->getLayerLoc();
   int nbatch                    = loc->nbatch;
   // Make sure all sizes match
   assert(locOriginal->nx == loc->nx);
   assert(locOriginal->ny == loc->ny);
   assert(locOriginal->nf == loc->nf);


   for (int b = 0; b < nbatch; b++) {
      const float *originalABatch = originalA + b * originalLayer->getNumExtended();
      float *ABatch               = A + b * getNumExtended();
#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for
#endif // PV_USE_OPENMP_THREADS
       for (int k = 0; k < numNeurons; k++) {
		   int kExt = kIndexExtended(
				 k,
				 loc->nx,
				 loc->ny,
				 loc->nf,
				 loc->halo.lt,
				 loc->halo.rt,
				 loc->halo.dn,
				 loc->halo.up);
		   int kExtOriginal = kIndexExtended(
				 k,
				 locOriginal->nx,
				 locOriginal->ny,
				 locOriginal->nf,
				 locOriginal->halo.lt,
				 locOriginal->halo.rt,
				 locOriginal->halo.dn,
				 locOriginal->halo.up);

		   double randd = rand_gaussian(0, stdDev);
		   ABatch[kExt] = originalABatch[kExtOriginal] + randd;

		}
	 }
   	 return status;
}

} // end namespace PV
