/*
 * BaseObject.cpp
 *
 *  Created on: Jan 20, 2016
 *      Author: pschultz
 */

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include "BaseObject.hpp"
#include "columns/HyPerCol.hpp"

namespace PV {

BaseObject::BaseObject() {
   initialize_base();
   // Note that initialize() is not called in the constructor.
   // Instead, derived classes should call BaseObject::initialize in their own
   // constructor.
}

int BaseObject::initialize_base() {
   name = NULL;
   parent = NULL;
   return PV_SUCCESS;
}

int BaseObject::initialize(const char * name, HyPerCol * hc) {
   int status = setName(name);
   setParams(hc->parameters());
   setCommunicator(hc->getCommunicator());
   if (status==PV_SUCCESS) { status = setParent(hc); }
   if (status==PV_SUCCESS) { status = setDescription(); }
   if (status==PV_SUCCESS) { status = ioParamsFillGroup(PARAMS_IO_READ); }
   if (status==PV_SUCCESS) {
      mBatchWidth = hc->getNBatch();
      mBatchWidthGlobal = hc->getNBatchGlobal();

   }
   return status;
}

char const * BaseObject::getKeyword() const {
   return getParams()->groupKeywordFromName(getName());
}

int BaseObject::setName(char const * name) {
   pvAssert(this->name==NULL);
   int status = PV_SUCCESS;
   this->name = strdup(name);
   if (this->name==NULL) {
      pvErrorNoExit().printf("could not set name \"%s\": %s\n", name, strerror(errno));
      status = PV_FAILURE;
   }
   return status;
}

void BaseObject::setParams(PVParams * params) {
   if (params==nullptr) {  throw; }
   mParams = params;
}

void BaseObject::setCommunicator(Communicator * comm) {
   if (comm==nullptr) { throw; }
   mCommunicator = comm;
}

int BaseObject::setParent(HyPerCol * hc) {
   pvAssert(parent==NULL);
   HyPerCol * parentCol = dynamic_cast<HyPerCol*>(hc);
   int status = parentCol!=NULL ? PV_SUCCESS : PV_FAILURE;
   if (parentCol) {
      parent = parentCol;
   }
   return status;
}

int BaseObject::setDescription() {
   description.clear();
   description.append(getKeyword()).append(" \"").append(getName()).append("\"");
   return PV_SUCCESS;
}

int BaseObject::respond(std::shared_ptr<BaseMessage const> message) {
   // TODO: convert PV_SUCCESS, PV_FAILURE, etc. to enum
   if (message==nullptr) {
      return PV_SUCCESS;
   }
   else if (auto castMessage = std::dynamic_pointer_cast<ReadParamsMessage const>(message)) {
      return respondReadParams(castMessage);
   }
   else if (auto castMessage = std::dynamic_pointer_cast<CommunicateInitInfoMessage const>(message)) {
      return respondCommunicateInitInfo(castMessage);
   }
   else if (auto castMessage = std::dynamic_pointer_cast<WriteParamsMessage const>(message)) {
      return respondWriteParams(castMessage);
   }
   else if (auto castMessage = std::dynamic_pointer_cast<AllocateDataMessage const>(message)) {
      return respondAllocateData(castMessage);
   }
   else if (auto castMessage = std::dynamic_pointer_cast<InitializeStateMessage const>(message)) {
      return respondInitializeState(castMessage);
   }
   else if (auto castMessage = std::dynamic_pointer_cast<CheckpointReadMessage const>(message)) {
      return respondCheckpointRead(castMessage);
   }
   else if (auto castMessage = std::dynamic_pointer_cast<CheckpointWriteMessage const>(message)) {
      return respondCheckpointWrite(castMessage);
   }
   else {
      return PV_SUCCESS;
   }
}

int BaseObject::respondReadParams(std::shared_ptr<ReadParamsMessage const> message) {
   return ioParamsFillGroup(PARAMS_IO_READ);
}

int BaseObject::respondCommunicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   int status = PV_SUCCESS;
   if (getInitInfoCommunicatedFlag()) { return status; }
   status = communicateInitInfo(message);
   if (status==PV_SUCCESS) { setInitInfoCommunicatedFlag(); }
   notify(mParameterDependencies, message);
   return status;
}

int BaseObject::respondWriteParams(std::shared_ptr<WriteParamsMessage const> message) {
   mPrintParamsStream = message->mPrintParamsStream;
   mPrintLuaParamsStream = message->mPrintLuaParamsStream;
   bool includeHeaderFooter = message->mIncludeHeaderFooter;
   mParams->writeParamsStartGroup(getName(), mPrintParamsStream, mPrintLuaParamsStream, includeHeaderFooter);
   int status = ioParamsFillGroup(PARAMS_IO_WRITE);
   auto dependencyMessage = std::make_shared<WriteParamsMessage>(message->mPrintParamsStream, mPrintLuaParamsStream, false);
   notify(mParameterDependencies, dependencyMessage);
   mParams->writeParamsFinishGroup(getName(), mPrintParamsStream, mPrintLuaParamsStream, includeHeaderFooter);
   return status;
}

int BaseObject::respondAllocateData(std::shared_ptr<AllocateDataMessage const> message) {
   int status = PV_SUCCESS;
   if (getDataStructuresAllocatedFlag()) { return status; }
   status = allocateDataStructures();
   if (status==PV_SUCCESS) { setDataStructuresAllocatedFlag(); }
   return status;
}

int BaseObject::respondInitializeState(std::shared_ptr<InitializeStateMessage const> message) {
   int status = PV_SUCCESS;
   if (getInitialValuesSetFlag()) { return status; }
   status = initializeState(message->mCheckpointDir.c_str());
   if (status==PV_SUCCESS) { setInitialValuesSetFlag(); }
   return status;
}

int BaseObject::respondCheckpointRead(std::shared_ptr<CheckpointReadMessage const> message) {
   return checkpointRead(message->mCheckpointDir.c_str(), message->mTimestampPtr);
}

int BaseObject::respondCheckpointWrite(std::shared_ptr<CheckpointWriteMessage const> message) {
   return checkpointWrite(message->mSuppressCheckpointIfConstant, message->mCheckpointDir.c_str(), message->mTimestamp);
}

int BaseObject::respondWriteTimers(std::shared_ptr<WriteTimersMessage const> message) {
   return writeTimers(message->mStream, message->mPhase);
}

int BaseObject::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   ioParam_initializeFromCheckpointFlag(ioFlag);
   return PV_SUCCESS;
}

void BaseObject::ioParam_initializeFromCheckpointFlag(enum ParamsIOFlag ioFlag) {
   ioParamValue(ioFlag, name, "initializeFromCheckpointFlag", &mInitializeFromCheckpointFlag, parent->getDefaultInitializeFromCheckpointFlag(), true/*warnIfAbsent*/);
}

void BaseObject::ioParamString(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, char ** value, const char * defaultValue, bool warnIfAbsent) {
   const char * param_string = nullptr;
   switch(ioFlag) {
   case PARAMS_IO_READ:
      if ( mParams->stringPresent(groupName, paramName) ) {
         param_string = mParams->stringValue(groupName, paramName, warnIfAbsent);
      }
      else {
         // parameter was not set in params file; use the default.  But default might or might not be nullptr.
         if (getCommunicator()->commRank()==0 && warnIfAbsent==true) {
            if (defaultValue != nullptr) {
               pvWarn().printf("Using default value \"%s\" for string parameter \"%s\" in group \"%s\"\n", defaultValue, paramName, groupName);
            }
            else {
               pvWarn().printf("Using default value of nullptr for string parameter \"%s\" in group \"%s\"\n", paramName, groupName);
            }
         }
         param_string = defaultValue;
      }
      if (param_string!=nullptr) {
         *value = strdup(param_string);
         pvErrorIf(*value==nullptr, "Global rank %d process unable to copy param %s in group \"%s\": %s\n", getCommunicator()->globalCommRank(), paramName, groupName, strerror(errno));
      }
      else {
         *value = nullptr;
      }
      break;
   case PARAMS_IO_WRITE:
      writeFormattedParamString(paramName, *value, mPrintParamsStream, mPrintLuaParamsStream);
   }
}

void BaseObject::ioParamStringRequired(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, char ** value) {
   const char * param_string = nullptr;
   switch(ioFlag) {
   case PARAMS_IO_READ:
      param_string = mParams->stringValue(groupName, paramName, false/*warnIfAbsent*/);
      if (param_string!=nullptr) {
         *value = strdup(param_string);
         pvErrorIf(*value==nullptr, "Global Rank %d process unable to copy param %s in group \"%s\": %s\n", getCommunicator()->globalCommRank(), paramName, groupName, strerror(errno));
      }
      else {
         if (getCommunicator()->globalCommRank()==0) {
            pvErrorNoExit().printf("%s \"%s\": string parameter \"%s\" is required.\n",
                            mParams->groupKeywordFromName(groupName), groupName, paramName);
         }
         MPI_Barrier(mCommunicator->globalCommunicator());
         exit(EXIT_FAILURE);
      }
      break;
   case PARAMS_IO_WRITE:
      writeFormattedParamString(paramName, *value, mPrintParamsStream, mPrintLuaParamsStream);
   }

}

BaseObject::~BaseObject() {
   free(name);
}

template <>
void BaseObject::ioParamValue<>(enum ParamsIOFlag ioFlag, char const * groupName, char const * paramName, int * value, int defaultValue, bool warnIfAbsent) {
   switch(ioFlag) {
   case PARAMS_IO_READ:
      *value = mParams->valueInt(groupName, paramName, defaultValue, warnIfAbsent);
      break;
   case PARAMS_IO_WRITE:
      writeFormattedParamValue(paramName, *value, mPrintParamsStream, mPrintLuaParamsStream);
      break;
   }
}

int BaseObject::initializeState(char const * checkpointDir) {
   int status = PV_SUCCESS;
   if (mInitializeFromCheckpointFlag) {
      status = readStateFromCheckpoint(checkpointDir, nullptr);
   }
   else {
      status = setInitialValues();
   }
   return status;
}

} /* namespace PV */
