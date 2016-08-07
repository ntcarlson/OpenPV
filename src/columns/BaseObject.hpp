/*
 * BaseObject.hpp
 *
 *  This is the base class for HyPerCol, layers, connections, probes, and
 *  anything else that the Factory object needs to know about.
 *
 *  All objects in the BaseObject hierarchy should have an associated
 *  instantiating function, with the prototype
 *  BaseObject * createNameOfClass(char const * name, HyPerCol * initData);
 *
 *  Each class's instantiating function should create an object of that class,
 *  with the arguments specifying the object's name and any necessary
 *  initializing data (for most classes, this is the parent HyPerCol.
 *  For HyPerCol, it is the PVInit object).  This way, the class can be
 *  registered with the Factory object by calling
 *  Factory::registerKeyword() with a pointer to the class's instantiating
 *  method.
 *
 *  Created on: Jan 20, 2016
 *      Author: pschultz
 */

#ifndef BASEOBJECT_HPP_
#define BASEOBJECT_HPP_

#include "observerpattern/Observer.hpp"
#include "observerpattern/Subject.hpp"
#include "columns/CommunicateInitInfoMessage.hpp"
#include "columns/Communicator.hpp"
#include "columns/Messages.hpp"
#include "include/pv_common.h"
#include "io/PVParams.hpp"
#include "utils/PVLog.hpp"
#include "utils/PVAssert.hpp"
#include "utils/PVAlloc.hpp"
#include <memory>

namespace PV {

class HyPerCol;

class BaseObject : public Observer, public Subject {
public:
   inline char const * getName() const { return name; }
   inline PVParams * getParams() const { return mParams; }
   inline Communicator * getCommunicator() const { return mCommunicator; }
   inline HyPerCol * getParent() const { return parent; }
   inline char const * getDescription_c() const { return description.c_str(); }
   char const * getKeyword() const;
   virtual int respond(std::shared_ptr<BaseMessage const> message) override; // TODO: should return enum with values corresponding to PV_SUCCESS, PV_FAILURE, PV_POSTPONE
   virtual ~BaseObject();

   /**
    * Get-method for mInitInfoCommunicatedFlag, which is false on initialization and
    * then becomes true once setInitInfoCommunicatedFlag() is called.
    */
   bool getInitInfoCommunicatedFlag() {return mInitInfoCommunicatedFlag;}

   /**
    * Get-method for mDataStructuresAllocatedFlag, which is false on initialization and
    * then becomes true once setDataStructuresAllocatedFlag() is called.
    */
   bool getDataStructuresAllocatedFlag() {return mDataStructuresAllocatedFlag;}

   /**
    * Get-method for mInitialValuesSetFlag, which is false on initialization and
    * then becomes true once setInitialValuesSetFlag() is called.
    */
   bool getInitialValuesSetFlag() {return mInitialValuesSetFlag;}

protected:
   BaseObject();
   int initialize(char const * name, HyPerCol * hc);
   int setName(char const * name);
   void setParams(PVParams * params);
   void setCommunicator(Communicator * comm);
   int setParent(HyPerCol * hc);
   virtual int setDescription();

   int respondReadParams(std::shared_ptr<ReadParamsMessage const> message);
   int respondCommunicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message);
   int respondWriteParams(std::shared_ptr<WriteParamsMessage const> message);
   int respondAllocateData(std::shared_ptr<AllocateDataMessage const> message);
   int respondInitializeState(std::shared_ptr<InitializeStateMessage const> message);

   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag) { return PV_SUCCESS; }
   virtual int communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) { return PV_SUCCESS; }
   virtual int allocateDataStructures() { return PV_SUCCESS; }
   virtual int initializeState() { return PV_SUCCESS; }

   template <typename T>
   void ioParamValue(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, T * val, T defaultValue, bool warnIfAbsent=true);

   template <typename T>
   void ioParamValueRequired(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, T * val);

   void ioParamString(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, char ** value, const char * defaultValue, bool warnIfAbsent=true);
   void ioParamStringRequired(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, char ** value);

   template <typename T>
   void ioParamArray(enum ParamsIOFlag ioFlag, const char * group_name, const char * param_name, T ** value, int * arraysize);

   /**
    * This method sets mInitInfoCommunicatedFlag to true.
    */
   void setInitInfoCommunicatedFlag() {mInitInfoCommunicatedFlag = true;}

   /**
    * This method sets mDataStructuresAllocatedFlag to true.
    */
   void setDataStructuresAllocatedFlag() {mDataStructuresAllocatedFlag = true;}

   /**
    * This method sets the flag returned by getInitialValuesSetFlag to true.
    */
   void setInitialValuesSetFlag() {mInitialValuesSetFlag = true;}

// Data members
protected:
   char * name = nullptr;
   PVParams * mParams = nullptr; // should be able to make const if PVParams is const-correct
   Communicator * mCommunicator = nullptr;
   HyPerCol * parent = nullptr; // TODO: eliminate HyPerCol argument to constructor in favor of PVParams argument
   bool mInitInfoCommunicatedFlag = false;
   bool mDataStructuresAllocatedFlag = false;
   bool mInitialValuesSetFlag = false;

   PV_Stream* mPrintParamsStream = nullptr; // file pointer associated with mPrintParamsFilename
   PV_Stream* mPrintLuaParamsStream = nullptr; // file pointer associated with the output lua file

   ObserverTable mParameterDependencies;
   int mBatchWidth = 1;
   int mBatchWidthGlobal = 1;

private:
   int initialize_base();
}; // class BaseObject

template <typename T>
void BaseObject::ioParamValue(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, T * value, T defaultValue, bool warnIfAbsent) {
   switch(ioFlag) {
   case PARAMS_IO_READ:
      *value = (T) mParams->value(groupName, paramName, defaultValue, warnIfAbsent);
      break;
   case PARAMS_IO_WRITE:
      writeFormattedParamValue(paramName, *value, mPrintParamsStream, mPrintLuaParamsStream);
      break;
   }
}

template <typename T>
void BaseObject::ioParamValueRequired(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, T * value) {
   switch(ioFlag) {
   case PARAMS_IO_READ:
      if (typeid(T) == typeid(int)) { *value = mParams->valueInt(groupName, paramName); }
      else { *value = mParams->value(groupName, paramName); }
      break;
   case PARAMS_IO_WRITE:
      writeFormattedParamValue(paramName, *value, mPrintParamsStream, mPrintLuaParamsStream);
      break;
   }
}

template <typename T>
void BaseObject::ioParamArray(enum ParamsIOFlag ioFlag, const char * groupName, const char * paramName, T ** value, int * arraysize) {
    if(ioFlag==PARAMS_IO_READ) {
       const double * param_array = mParams->arrayValuesDbl(groupName, paramName, arraysize);
       pvAssert(*arraysize>=0);
       if (*arraysize>0) {
          *value = (T *) calloc((size_t) *arraysize, sizeof(T));
          pvErrorIf(value==nullptr, "%s \"%s\": global rank %d process unable to copy array parameter %s: %s\n",
                   getParams()->groupKeywordFromName(name), name, getCommunicator()->globalCommRank(), paramName, strerror(errno));
          for (int k=0; k<*arraysize; k++) {
             (*value)[k] = (T) param_array[k];
          }
       }
       else {
          *value = nullptr;
       }
    }
    else if (ioFlag==PARAMS_IO_WRITE) {
       writeFormattedParamArray(paramName, *value, (size_t) *arraysize, mPrintParamsStream, mPrintLuaParamsStream);
    }
}

}  // namespace PV

#endif /* BASEOBJECT_HPP_ */
