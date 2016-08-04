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

int BaseObject::respond(std::shared_ptr<BaseMessage> message) {
   // TODO: convert PV_SUCCESS, PV_FAILURE, etc. to enum
   if (message==nullptr) {
      return PV_SUCCESS;
   }
   else if (auto castMessage = dynamic_cast<ReadParamsMessage const*>(message.get())) {
      return respondReadParams(castMessage);
   }
   else if (auto castMessage = dynamic_cast<CommunicateInitInfoMessage const*>(message.get())) {
      return respondCommunicateInitInfo(castMessage);
   }
   else if (auto castMessage = dynamic_cast<WriteParamsMessage const*>(message.get())) {
      return respondWriteParams(castMessage);
   }
   else if (auto castMessage = dynamic_cast<AllocateDataMessage const*>(message.get())) {
      return respondAllocateData(castMessage);
   }
   else if (auto * castMessage = dynamic_cast<InitializeStateMessage const*>(message.get())) {
      return respondInitializeState(castMessage);
   }
   else {
      return PV_SUCCESS;
   }
}

int BaseObject::respondReadParams(ReadParamsMessage const * message) {
   return ioParamsFillGroup(PARAMS_IO_READ);
}

int BaseObject::respondCommunicateInitInfo(CommunicateInitInfoMessage const * message) {
   int status = PV_SUCCESS;
   if (getInitInfoCommunicatedFlag()) { return status; }
   status = communicateInitInfo(message);
   if (status==PV_SUCCESS) { setInitInfoCommunicatedFlag(); }
   return status;
}

int BaseObject::respondWriteParams(WriteParamsMessage const * message) {
   mPrintParamsStream = message->mPrintParamsStream;
   mPrintLuaParamsStream = message->mPrintLuaParamsStream;
   bool includeHeaderFooter = message->mIncludeHeaderFooter;
   mParams->writeParamsStartGroup(getName(), mPrintParamsStream, mPrintLuaParamsStream, includeHeaderFooter);
   int status = ioParamsFillGroup(PARAMS_IO_WRITE);
   mParams->writeParamsFinishGroup(getName(), mPrintParamsStream, mPrintLuaParamsStream, includeHeaderFooter);
   return status;
}

int BaseObject::respondAllocateData(AllocateDataMessage const * message) {
   int status = PV_SUCCESS;
   if (getDataStructuresAllocatedFlag()) { return status; }
   status = allocateDataStructures();
   if (status==PV_SUCCESS) { setDataStructuresAllocatedFlag(); }
   return status;
}

int BaseObject::respondInitializeState(InitializeStateMessage const * message) {
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
