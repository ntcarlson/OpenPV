/*
 * TestAllZerosProbe.cpp
 *
 * A subclass of StatsProbe that verifies a layer is all zeros.
 *
 *  Created on: Dec 6, 2011
 *      Author: pschultz
 */

#include "TestAllZerosProbe.hpp"

namespace PV {

TestAllZerosProbe::TestAllZerosProbe(const char * probeName, HyPerCol * hc) {
   initTestAllZerosProbe_base();
   initTestAllZerosProbe(probeName, hc);
}

int TestAllZerosProbe::outputState(double timed) {
   int status = StatsProbe::outputState(timed);
   if (status != PV_SUCCESS) {
      fprintf(stderr, "!!Time %f: TestAllZerosProbe::outputState failed for layer \"%s\"\n", timed, getTargetLayer()->getName());
      exit(EXIT_FAILURE);
   }
#ifdef PV_USE_MPI
   InterColComm * icComm = getTargetLayer()->getParent()->icCommunicator();
   const int rcvProc = 0;
   if( icComm->commRank() != rcvProc ) {
      return 0;
   }
#endif // PV_USE_MPI
   for(int b = 0; b < parent->getNBatch(); b++){
      if (nnz[b] != 0) {
         fprintf(stderr, "!!Time %f batch %d: layer \"%s\" is not all zeroes.\n", timed, b, getTargetLayer()->getName());
         exit(EXIT_FAILURE);
      }
   }
   return status;
}

int TestAllZerosProbe::initTestAllZerosProbe(const char * probeName, HyPerCol * hc) {
   return StatsProbe::initStatsProbe(probeName, hc);
}

void TestAllZerosProbe::ioParam_buffer(enum ParamsIOFlag ioFlag) {
   requireType(BufActivity);
}

}; // namespace PV