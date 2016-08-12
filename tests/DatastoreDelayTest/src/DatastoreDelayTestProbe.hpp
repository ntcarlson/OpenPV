/*
 * DatastoreDelayTestProbe.hpp
 *
 *  Created on: Mar 10, 2009
 *      Author: garkenyon
 */

#ifndef DATASTOREDELAYTESTPROBE_HPP_
#define DATASTOREDELAYTESTPROBE_HPP_

#include <io/StatsProbe.hpp>
#include "DatastoreDelayTestLayer.hpp"
#include <include/pv_common.h>

namespace PV {

class DatastoreDelayTestProbe: public StatsProbe {
public:
   DatastoreDelayTestProbe(const char * probename, HyPerCol * hc);

   virtual int outputState(double timed);

   virtual ~DatastoreDelayTestProbe();

protected:
   int initDatastoreDelayTestProbe(const char * probename,  HyPerCol * hc);
   virtual int communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) override;
   virtual void ioParam_buffer(enum ParamsIOFlag ioFlag);

protected:
   DatastoreDelayTestLayer * inputLayer = nullptr;
};


}

#endif /* DATASTOREDELAYTESTPROBE_HPP_ */
