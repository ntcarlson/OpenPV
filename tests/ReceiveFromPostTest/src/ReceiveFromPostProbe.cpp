/*
 * receiveFromPostProbe.cpp
 * Author: slundquist
 */

#include "ReceiveFromPostProbe.hpp"
#include <include/pv_arch.h>
#include <layers/HyPerLayer.hpp>
#include <utils/PVLog.hpp>
#include <string.h>

namespace PV {
ReceiveFromPostProbe::ReceiveFromPostProbe(const char * probeName, HyPerCol * hc)
   : StatsProbe()
{
   initReceiveFromPostProbe_base();
   initReceiveFromPostProbe(probeName, hc);
}

int ReceiveFromPostProbe::initReceiveFromPostProbe_base() {
   tolerance = (pvadata_t) 1e-3;
   return PV_SUCCESS;
}

int ReceiveFromPostProbe::initReceiveFromPostProbe(const char * probeName, HyPerCol * hc) {
   return initStatsProbe(probeName, hc);
}

int ReceiveFromPostProbe::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = StatsProbe::ioParamsFillGroup(ioFlag);
   ioParam_tolerance(ioFlag);
   return status;
}

void ReceiveFromPostProbe::ioParam_buffer(enum ParamsIOFlag ioFlag) {
   requireType(BufActivity);
}

void ReceiveFromPostProbe::ioParam_tolerance(enum ParamsIOFlag ioFlag) {
   ioParamValue(ioFlag, getName(), "tolerance", &tolerance, tolerance);
}

int ReceiveFromPostProbe::outputState(double timed){
   int status = StatsProbe::outputState(timed);
   const PVLayerLoc * loc = getTargetLayer()->getLayerLoc();
   int numExtNeurons = getTargetLayer()->getNumExtended();
   const pvdata_t * A = getTargetLayer()->getLayerData();
   for (int i = 0; i < numExtNeurons; i++){
      if(fabs(A[i]) != 0){
         int xpos = kxPos(i, loc->nx+loc->halo.lt+loc->halo.rt, loc->ny+loc->halo.dn+loc->halo.up, loc->nf);
         int ypos = kyPos(i, loc->nx+loc->halo.lt+loc->halo.rt, loc->ny+loc->halo.dn+loc->halo.up, loc->nf);
         int fpos = featureIndex(i, loc->nx+loc->halo.lt+loc->halo.rt, loc->ny+loc->halo.dn+loc->halo.up, loc->nf);
         //pvInfo() << "[" << xpos << "," << ypos << "," << fpos << "] = " << std::fixed << A[i] << "\n";
      }
      //For roundoff errors
      if(fabs(A[i]) >= tolerance) {
         pvErrorNoExit().printf("%s %s activity outside of tolerance %f: extended index %d has activity %f\n",
               getMessage(), getTargetLayer()->getDescription_c(), tolerance, i, A[i]);
         status = PV_FAILURE;
      }
      if (status != PV_SUCCESS) {
         exit(EXIT_FAILURE);
      }
   }
   return status;
}

}  // end namespace PV
