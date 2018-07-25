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
   SPIKE_BELOW,
   SPIKE_ABOVE
} NeuronSpikeType;

typedef enum NeuronSpikeMethod {
   SPIKE_METHOD_SET,
   SPIKE_METHOD_ADD,
   SPIKE_METHOD_SCALE
} NeuronSpikeMethod;

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
   Response::Status spikeAboveBatch(float *ABatch);
   Response::Status spikeBelowBatch(float *ABatch);

   int ioParamsFillGroup(enum ParamsIOFlag ioFlag);
   void ioParam_numSpike(enum ParamsIOFlag ioFlag);
   void ioParam_spikeValue(enum ParamsIOFlag ioFlag);
   void ioParam_threshold(enum ParamsIOFlag ioFlag);
   void ioParam_seed(enum ParamsIOFlag ioFlag);
   void ioParam_neuronsToSpike(enum ParamsIOFlag ioFlag);
   void ioParam_spikeMethod(enum ParamsIOFlag ioFlag);

  private:
   int initialize_base();
   struct drand48_data rng_state;

  protected:
   double spikeValue;
   double threshold;
   int numSpike;
   long seed;

   char * neuronsToSpike;
   NeuronSpikeType neuronsToSpikeType;
   char * spikeMethod;
   NeuronSpikeMethod method;

   void spike(float *neuron);
   
}; // class RandomSpikeLayer

} // namespace PV

#endif /* RANDOMSPIKELAYER_HPP_ */
