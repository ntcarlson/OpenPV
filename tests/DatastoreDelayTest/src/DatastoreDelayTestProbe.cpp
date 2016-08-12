/*
 * DatastoreDelayTestProbe.cpp
 *
 *  Created on:
 *      Author: garkenyon
 */

#include "DatastoreDelayTestProbe.hpp"
#include <string.h>
#include <utils/PVLog.hpp>

namespace PV {

DatastoreDelayTestProbe::DatastoreDelayTestProbe(const char * probeName, HyPerCol * hc) : StatsProbe()
{
   initDatastoreDelayTestProbe(probeName, hc);
}


int DatastoreDelayTestProbe::initDatastoreDelayTestProbe(const char * probeName, HyPerCol * hc) {
   initStatsProbe(probeName, hc);
   return PV_SUCCESS;
}

void DatastoreDelayTestProbe::ioParam_buffer(enum ParamsIOFlag ioFlag) {
   if (ioFlag == PARAMS_IO_READ) {
      requireType(BufActivity);
   }
}
int DatastoreDelayTestProbe::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   int status = StatsProbe::communicateInitInfo(message);
   if (status != PV_SUCCESS) {
      return status;
   }
   inputLayer = message->mTable->lookup<DatastoreDelayTestLayer>("input");
   if (inputLayer==nullptr) {
      if (getCommunicator()->commRank()==0) {
         pvErrorNoExit() << getDescription() << ": No DatastoreDelayTestLayer named \"input\" found.\n";
      }
      MPI_Barrier(getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
   return status;
}

int DatastoreDelayTestProbe::outputState(double timed) {
   HyPerLayer * l = getTargetLayer();
   Communicator * icComm = l->getCommunicator();
   const int rcvProc = 0;
   if( icComm->commRank() != rcvProc ) {
      return PV_SUCCESS;
   }
   int status = PV_SUCCESS;
   int numDelayLevels = inputLayer->getNumDelayLevels();
   if( timed >= numDelayLevels+2 ) {
      pvdata_t correctValue = numDelayLevels*(numDelayLevels+1)/2;
      pvdata_t * V = l->getV();
      for( int k=0; k<l->getNumNeuronsAllBatches(); k++ ) {
         if( V[k] != correctValue ) {
            outputStream->printf("%s: timef = %f, neuron %d: value is %f instead of %d\n", l->getDescription_c(), timed, k, V[k], (int) correctValue);
            status = PV_FAILURE;
         }
      }
      if( status == PV_SUCCESS) {
         outputStream->printf("%s: timef = %f, all neurons have correct value %d\n", l->getDescription_c(), timed, (int) correctValue);
      }
   }
   pvErrorIf(!(status == PV_SUCCESS), "Test failed.\n");
   return PV_SUCCESS;
}

DatastoreDelayTestProbe::~DatastoreDelayTestProbe() {
}

}  // end of namespace PV block
