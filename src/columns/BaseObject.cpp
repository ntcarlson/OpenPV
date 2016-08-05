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
   return status;
}

int BaseObject::respondWriteParams(std::shared_ptr<WriteParamsMessage const> message) {
   mPrintParamsStream = message->mPrintParamsStream;
   mPrintLuaParamsStream = message->mPrintLuaParamsStream;
   bool includeHeaderFooter = message->mIncludeHeaderFooter;
   mParams->writeParamsStartGroup(getName(), mPrintParamsStream, mPrintLuaParamsStream, includeHeaderFooter);
   int status = ioParamsFillGroup(PARAMS_IO_WRITE);
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
   status = initializeState();
   if (status==PV_SUCCESS) { setInitialValuesSetFlag(); }
   return status;
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


} /* namespace PV */
