/*
 * RescaleLayer.cpp
 * Noise layer is a cloneVLayer, grabs activity from orig layer and adds white noise to it
 */

#ifndef NOISELAYER_HPP_
#define NOISELAYER_HPP_

#include "CloneVLayer.hpp"

namespace PV {

// CloneLayer can be used to implement Sigmoid junctions between spiking neurons
class NoiseLayer : public CloneVLayer {
  public:
   NoiseLayer(const char *name, HyPerCol *hc);
   virtual ~NoiseLayer();
   virtual int communicateInitInfo(CommunicateInitInfoMessage const *message) override;
   virtual int allocateV() override;
   virtual int updateState(double timef, double dt) override;
   virtual int setActivity() override;

   float getStdDev() { return stdDev; }

  protected:
   NoiseLayer();
   int initialize(const char *name, HyPerCol *hc);
   int ioParamsFillGroup(enum ParamsIOFlag ioFlag);

   void ioParam_stdDev(enum ParamsIOFlag ioFlag);



  private:
   int initialize_base();

   double rand_gaussian(double mean, double sdev);

  protected:
   double stdDev;
}; // class NoiseLayer

} // namespace PV

#endif /* NOISELAYER_HPP_ */
