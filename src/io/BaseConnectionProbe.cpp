/*
 * BaseConnectionProbe.cpp
 *
 *  Created on: Oct 20, 2011
 *      Author: pschultz
 */

#include "BaseConnectionProbe.hpp"

namespace PV {

BaseConnectionProbe::BaseConnectionProbe() {
   initialize_base();
}

BaseConnectionProbe::BaseConnectionProbe(const char * probeName, HyPerCol * hc)
{
   initialize_base();
   initialize(probeName, hc);
}

BaseConnectionProbe::~BaseConnectionProbe() {
}

int BaseConnectionProbe::initialize_base() {
   targetConn = NULL;
   return PV_SUCCESS;
}

int BaseConnectionProbe::initialize(const char * probeName, HyPerCol * hc) {
   int status = BaseProbe::initialize(probeName, hc);
   return status;
}

void BaseConnectionProbe::ioParam_targetName(enum ParamsIOFlag ioFlag) {
   ioParamString(ioFlag, name, "targetConnection", &targetName, NULL, false);
   if(targetName == NULL){
      BaseProbe::ioParam_targetName(ioFlag);
   }
}

int BaseConnectionProbe::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   BaseProbe::communicateInitInfo(message);
   targetConn = message->mTable->lookup<BaseConnection>(targetName);
   if (targetConn==NULL) {
      if (getCommunicator()->commRank()==0) {
         pvErrorNoExit().printf("%s, rank %d process: targetConnection \"%s\" is not a connection in the HyPerCol.\n",
               getDescription_c(), getCommunicator()->commRank(), targetName);
      }
      MPI_Barrier(getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
   targetConn->insertProbe(this);
   return PV_SUCCESS;
}


}  // end of namespace PV


