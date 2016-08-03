/*
 * PlasticCloneConn.hpp
 *
 *  Created on: May 24, 2011
 *      Author: peteschultz
 */

#ifndef PLASTICCLONECONN_HPP_
#define PLASTICCLONECONN_HPP_

#include "CloneConn.hpp"

namespace PV {

class PlasticCloneConn : public CloneConn{

public:
   PlasticCloneConn(const char * name, HyPerCol * hc);
   virtual ~PlasticCloneConn();

protected:
   PlasticCloneConn();
   int initialize(const char * name, HyPerCol * hc);
   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag) override;
   virtual void ioParam_keepKernelsSynchronized(enum ParamsIOFlag ioFlag);
   virtual void ioParam_normalizeDw(enum ParamsIOFlag ioFlag);
   virtual int communicateInitInfo(CommunicateInitInfoMessage const * message) override;
   virtual int cloneParameters();
   virtual int constructWeights();
   int deleteWeights();

private:
   int initialize_base();

}; // end class PlasticCloneConn

}  // end namespace PV

#endif /* CLONECONN_HPP_ */
