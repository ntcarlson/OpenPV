/*
 * DependentSharedWeights.cpp
 *
 *  Created on: Jan 5, 2018
 *      Author: pschultz
 */

#include "DependentSharedWeights.hpp"
#include "columns/HyPerCol.hpp"
#include "columns/ObjectMapComponent.hpp"
#include "components/OriginalConnNameParam.hpp"
#include "connections/BaseConnection.hpp"
#include "utils/MapLookupByType.hpp"

namespace PV {

DependentSharedWeights::DependentSharedWeights(char const *name, HyPerCol *hc) {
   initialize(name, hc);
}

DependentSharedWeights::DependentSharedWeights() {}

DependentSharedWeights::~DependentSharedWeights() {}

int DependentSharedWeights::initialize(char const *name, HyPerCol *hc) {
   return SharedWeights::initialize(name, hc);
}

int DependentSharedWeights::setDescription() {
   description = "DependentSharedWeights \"";
   description += name;
   description += "\"";
   return PV_SUCCESS;
}

int DependentSharedWeights::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   return SharedWeights::ioParamsFillGroup(ioFlag);
}

void DependentSharedWeights::ioParam_sharedWeights(enum ParamsIOFlag ioFlag) {
   if (ioFlag == PARAMS_IO_READ) {
      parent->parameters()->handleUnnecessaryParameter(name, "sharedWeights");
   }
   // During the communication phase, sharedWeights will be copied from originalConn
}

int DependentSharedWeights::communicateInitInfo(
      std::shared_ptr<CommunicateInitInfoMessage const> message) {
   auto hierarchy = message->mHierarchy;

   char const *originalConnName = getOriginalConnName(hierarchy);
   pvAssert(originalConnName);

   auto *originalSharedWeights = getOriginalSharedWeights(hierarchy, originalConnName);
   pvAssert(originalSharedWeights);

   if (!originalSharedWeights->getInitInfoCommunicatedFlag()) {
      if (parent->getCommunicator()->globalCommRank() == 0) {
         InfoLog().printf(
               "%s must wait until original connection \"%s\" has finished its communicateInitInfo "
               "stage.\n",
               getDescription_c(),
               originalConnName);
      }
      return PV_POSTPONE;
   }
   mSharedWeights = originalSharedWeights->getSharedWeights();
   parent->parameters()->handleUnnecessaryParameter(name, "sharedWeights", mSharedWeights);

   int status = SharedWeights::communicateInitInfo(message);
   return status;
}

char const *DependentSharedWeights::getOriginalConnName(
      std::map<std::string, Observer *> const hierarchy) const {
   OriginalConnNameParam *originalConnNameParam =
         mapLookupByType<OriginalConnNameParam>(hierarchy, getDescription());
   FatalIf(
         originalConnNameParam == nullptr,
         "%s requires an OriginalConnNameParam component.\n",
         getDescription_c());
   char const *originalConnName = originalConnNameParam->getOriginalConnName();
   return originalConnName;
}

SharedWeights *DependentSharedWeights::getOriginalSharedWeights(
      std::map<std::string, Observer *> const hierarchy,
      char const *originalConnName) const {
   ObjectMapComponent *objectMapComponent =
         mapLookupByType<ObjectMapComponent>(hierarchy, getDescription());
   pvAssert(objectMapComponent);
   BaseConnection *originalConn =
         objectMapComponent->lookup<BaseConnection>(std::string(originalConnName));
   if (originalConn == nullptr) {
      if (parent->getCommunicator()->globalCommRank() == 0) {
         ErrorLog().printf(
               "%s: originalConnName \"%s\" does not correspond to a BaseConnection in the "
               "column.\n",
               getDescription_c(),
               originalConnName);
      }
      MPI_Barrier(parent->getCommunicator()->globalCommunicator());
      exit(PV_FAILURE);
   }

   auto *originalSharedWeights = originalConn->getComponentByType<SharedWeights>();
   FatalIf(
         originalSharedWeights == nullptr,
         "%s original connection \"%s\" does not have an SharedWeights.\n",
         getDescription_c(),
         originalConnName);
   return originalSharedWeights;
}

} // namespace PV
