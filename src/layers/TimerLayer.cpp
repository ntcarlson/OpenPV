/*
 * TimerLayer.cpp
 */

#include "TimerLayer.hpp"

namespace PV {

TimerLayer::TimerLayer() {
   initialize_base();
}

TimerLayer::TimerLayer(const char *name, HyPerCol *hc) {
   initialize_base();
   initialize(name, hc);
}

int TimerLayer::initialize_base() {
    return PV_SUCCESS;
}

int TimerLayer::initialize(char const *name, HyPerCol *hc) {
   int status = HyPerLayer::initialize(name, hc);
   mLastUpdateTime = 0;
   internalTimer = 0;
   return status;
}


int TimerLayer::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = HyPerLayer::ioParamsFillGroup(ioFlag);
   ioParam_offset(ioFlag);
   ioParam_period(ioFlag);
   return status;
}

void TimerLayer::ioParam_offset(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "offset", &offset, 0);
}

void TimerLayer::ioParam_period(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "period", &period, 0, true);
}


TimerLayer::~TimerLayer() {}


bool TimerLayer::needUpdate(double simTime, double dt) {
   internalTimer = simTime;
   double nextUpdate = mLastUpdateTime + getDeltaUpdateTime()*dt;
   if ((simTime >= nextUpdate && simTime < nextUpdate + dt) || simTime <= mLastUpdateTime) {
      return true;
   }
   return false;
}

double TimerLayer::getDeltaUpdateTime() {
    if (internalTimer <= offset) {
        return offset;
    } else {
        return period;
    }
}

Response::Status TimerLayer::updateState(double timef, double dt) {
   return Response::SUCCESS;
}

} // namespace PV
