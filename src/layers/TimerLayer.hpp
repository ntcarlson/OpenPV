/*
 * TimerLayer.hpp
 *
 * Dummy layer that activates at set times to trigger other layerse
 */

#ifndef TIMERLAYER_HPP_
#define TIMERLAYER_HPP_

#include "layers/HyPerLayer.hpp"

namespace PV {

class TimerLayer : public HyPerLayer {
  public:
   virtual ~TimerLayer();
   TimerLayer(const char *name, HyPerCol *hc);

   virtual bool needUpdate(double simTime, double dt) override;

  protected:
   TimerLayer();
   int initialize(char const *name, HyPerCol *hc);

   int ioParamsFillGroup(enum ParamsIOFlag ioFlag);
   virtual void ioParam_offset(enum ParamsIOFlag ioFlag);
   virtual void ioParam_period(enum ParamsIOFlag ioFlag);

   virtual double getDeltaUpdateTime() override;
   virtual Response::Status updateState(double timef, double dt) override;

  private:
   int initialize_base();
   int offset;
   int period;
   double internalTimer;
};

} // namespace PV

#endif /* TIMERLAYER_HPP_ */
