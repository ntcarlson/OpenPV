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

#include <observerpattern/Observer.hpp>
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

class BaseObject : public Observer {
public:
   inline char const * getName() const { return name; }
   inline PVParams * getParams() const { return mParams; }
   inline Communicator * getCommunicator() const { return mCommunicator; }
   inline HyPerCol * getParent() const { return parent; }
   inline char const * getDescription_c() const { return description.c_str(); }
   char const * getKeyword() const;
   virtual int respond(std::shared_ptr<BaseMessage> message) override; // TODO: should return enum with values corresponding to PV_SUCCESS, PV_FAILURE, PV_POSTPONE
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

   int respondReadParams(ReadParamsMessage const * message);
   int respondCommunicateInitInfo(CommunicateInitInfoMessage const * message);
   int respondWriteParams(WriteParamsMessage const * message);
   int respondAllocateData(AllocateDataMessage const * message);
   int respondInitializeState(InitializeStateMessage const * message);


   /**
    * Method for reading or writing the params from group in the parent HyPerCol's parameters.
    * The group from params is selected using the name of the connection.
    *
    * Note that ioParams is not virtual.  To add parameters in a derived class, override ioParamFillGroup.
    */
   int ioParams(enum ParamsIOFlag ioFlag);

   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag) { return PV_SUCCESS; }
   virtual int communicateInitInfo(CommunicateInitInfoMessage const * message) { return PV_SUCCESS; }
   virtual int allocateDataStructures() { return PV_SUCCESS; }
   virtual int initializeState() { return PV_SUCCESS; }

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

private:
   int initialize_base();
}; // class BaseObject

}  // namespace PV

#endif /* BASEOBJECT_HPP_ */
