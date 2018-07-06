/*
 * RandomSpikeLayer.cpp
 * RandomSpikeLayer is a cloneVLayer, grabs activity from orig layer and randomly spikes a number of neurons
 */

#ifndef RANDOMSPIKELAYER_HPP_
#define RANDOMSPIKELAYER_HPP_

#include "CloneVLayer.hpp"
#include <stdlib.h>

namespace PV {

typedef enum NeuronSpikeTypeEnum {
   SPIKE_ANY,
   SPIKE_ONLYACTIVE,
   SPIKE_ONLYINACTIVE
} NeuronSpikeType;

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


   Response::Status spikeAnyBatch(float *ABatch);
   Response::Status spikeInactiveBatch(float *ABatch);
   Response::Status spikeActiveBatch(float *ABatch);

   int ioParamsFillGroup(enum ParamsIOFlag ioFlag);
   void ioParam_numSpike(enum ParamsIOFlag ioFlag);
   void ioParam_spikeValue(enum ParamsIOFlag ioFlag);
   void ioParam_seed(enum ParamsIOFlag ioFlag);
   void ioParam_neuronsToSpike(enum ParamsIOFlag ioFlag);

  private:
   int initialize_base();
   struct drand48_data rng_state;

  protected:
   double spikeValue;
   int numSpike;
   long seed;

   char * neuronsToSpike; // Possible values are any, onlyActive, onlyInactive
   NeuronSpikeType neuronsToSpikeType;
   
}; // class RandomSpikeLayer

} // namespace PV

#endif /* RANDOMSPIKELAYER_HPP_ */
