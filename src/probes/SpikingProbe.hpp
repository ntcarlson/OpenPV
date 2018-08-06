/*
 * AbstractNormProbe.hpp
 *
 *  Created on: Aug 11, 2015
 *      Author: pschultz
 */

#ifndef SPIKINGPROBE_HPP_
#define SPIKINGPROBE_HPP_

#include "LayerProbe.hpp"

namespace PV {

class SpikingProbe : public LayerProbe {
  public:
   SpikingProbe(const char *name, HyPerCol *hc);
   virtual ~SpikingProbe();

  protected:
   SpikingProbe();
   int initialize(const char *name, HyPerCol *hc);

   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag) override;
   void ioParam_delay(enum ParamsIOFlag ioFlag);
   void ioParam_period(enum ParamsIOFlag ioFlag);
   void ioParam_seed(enum ParamsIOFlag ioFlag);
   void ioParam_numSpike(enum ParamsIOFlag ioFlag);
   void ioParam_maskLayer(enum ParamsIOFlag ioFlag);

   virtual void calcValues(double timevalue) override;

   /**
    * Calls LayerProbe::communicateInitInfo to set up the targetLayer and
    * attach the probe; and then checks the masking layer if masking is used.
    */
   virtual Response::Status
   communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) override;

   /**
    * Implements the outputState method required by classes derived from
    * BaseProbe.
    * Prints to the outputFile the probe message, timestamp, number of neurons,
    * and norm value for
    * each batch element.
    */
   virtual Response::Status outputState(double timevalue) override;



  private:
   void findActive();
   void findSpikeValues();
   void applySpikeValues();
   void applyMask();

   HyPerLayer * maskLayer;
   char * maskLayerStr;

   std::vector<int> neuronsToSpike;
   std::vector<float> spikeVals;
   int initialize_base();
   float spikeVal;
   int delay;
   int period;
   int numSpike;
   struct drand48_data rng_state;
   long seed;

}; // end class SpikingProbe

} // end namespace PV

#endif /* SPIKINGPROBE_HPP_ */
