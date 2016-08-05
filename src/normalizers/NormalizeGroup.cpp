/*
 * NormalizeGroup.cpp
 *
 *  Created on: Jun 22, 2016
 *      Author: pschultz
 */

#include <normalizers/NormalizeGroup.hpp>

namespace PV {

NormalizeGroup::NormalizeGroup(char const * name, HyPerCol * hc) {
   initialize_base();
   initialize(name, hc);
}

NormalizeGroup::NormalizeGroup() {
}

NormalizeGroup::~NormalizeGroup() {
   free(normalizeGroupName);
}

int NormalizeGroup::initialize_base() {
   return PV_SUCCESS;
}

int NormalizeGroup::initialize(char const * name, HyPerCol * hc) {
   int status = NormalizeBase::initialize(name, hc);
   return status;
}

int NormalizeGroup::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = NormalizeBase::ioParamsFillGroup(ioFlag);
   ioParam_normalizeGroupName(ioFlag);
   return status;
}

// The NormalizeBase parameters are overridden to do nothing in NormalizeGroup.
void NormalizeGroup::ioParam_strength(enum ParamsIOFlag ioFlag) {}
void NormalizeGroup::ioParam_normalizeArborsIndividually(enum ParamsIOFlag ioFlag) {}
void NormalizeGroup::ioParam_normalizeOnInitialize(enum ParamsIOFlag ioFlag) {}
void NormalizeGroup::ioParam_normalizeOnWeightUpdate(enum ParamsIOFlag ioFlag) {}

void NormalizeGroup::ioParam_normalizeGroupName(enum ParamsIOFlag ioFlag) {
   parent->ioParamStringRequired(ioFlag, name, "normalizeGroupName", &normalizeGroupName);
}

int NormalizeGroup::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   int status = NormalizeBase::communicateInitInfo(message);
   pvAssert(targetConn);

   HyPerConn * normalizeGroup = message->mTable->lookup<HyPerConn>(normalizeGroupName);
   if (normalizeGroup==nullptr) {
      if (getCommunicator()->commRank()==0) {
         pvErrorNoExit() << getDescription() << ": normalizeGroupName \"" << normalizeGroupName << "\" is not recognized.\n";
      }
      MPI_Barrier(getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
   normalizeGroup->addObserver(targetConn, ConnectionAddNormalizerMessage());
}

int NormalizeGroup::normalizeWeights() {
   return PV_SUCCESS;
}

} /* namespace PV */
