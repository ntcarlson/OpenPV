/*
 * RandomSpikeLayer.cpp
 * RandomSpikeLayer is a cloneVLayer, grabs activity from orig layer and randomly spikes a number of neurons
 */

#ifndef RANDOMSPIKELAYER_HPP_
#define RANDOMSPIKELAYER_HPP_

#include "CloneVLayer.hpp"
#include <stdlib.h>

namespace PV {

class RandomSpikeLayer : public CloneVLayer {
  public:
   RandomSpikeLayer(const char *name, HyPerCol *hc);
   virtual ~RandomSpikeLayer();
   virtual Response::Status communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) override;
   virtual Response::Status updateState(double timef, double dt) override;
   virtual int setActivity() override;

  protected:
   RandomSpikeLayer();
   int initialize(const char *name, HyPerCol *hc);

   int ioParamsFillGroup(enum ParamsIOFlag ioFlag);
   void ioParam_numSpike(enum ParamsIOFlag ioFlag);
   void ioParam_spikeValue(enum ParamsIOFlag ioFlag);
   void ioParam_seed(enum ParamsIOFlag ioFlag);

  private:
   int initialize_base();
   struct drand48_data rng_state;

  protected:
   double spikeValue;
   int numSpike;
   long seed;
}; // class RandomSpikeLayer

} // namespace PV

#endif /* RANDOMSPIKELAYER_HPP_ */
