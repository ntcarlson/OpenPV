/*
 * RescaleLayer.cpp
 * Noise layer is a cloneVLayer, grabs activity from orig layer and adds white noise to it
 */

#ifndef NOISELAYER_HPP_
#define NOISELAYER_HPP_

#include "CloneVLayer.hpp"
#include <stdlib.h>

namespace PV {

// CloneLayer can be used to implement Sigmoid junctions between spiking neurons
class NoiseLayer : public CloneVLayer {
  public:
   NoiseLayer(const char *name, HyPerCol *hc);
   virtual ~NoiseLayer();
   virtual Response::Status communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) override;
   virtual void allocateV() override;
   virtual Response::Status updateState(double timef, double dt) override;
   virtual int setActivity() override;

   float getStdDev() { return stdDev; }

  protected:
   NoiseLayer();
   int initialize(const char *name, HyPerCol *hc);
   int ioParamsFillGroup(enum ParamsIOFlag ioFlag);

   void ioParam_stdDev(enum ParamsIOFlag ioFlag);
   void ioParam_seed(enum ParamsIOFlag ioFlag);



  private:
   int initialize_base();

   double rand_gaussian(double mean, double sdev);
   struct drand48_data rng_state;

  protected:
   double stdDev;
   long seed;
}; // class NoiseLayer

} // namespace PV

#endif /* NOISELAYER_HPP_ */
