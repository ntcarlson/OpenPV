/*
 * LIFGap.hpp
 *
 *  Created on: Jul 29, 2011
 *      Author: garkenyon
 */

#ifndef LIFGAP_HPP_
#define LIFGAP_HPP_

#include "LIF.hpp"

#define NUM_LIFGAP_EVENTS   1 + NUM_LIF_EVENTS  // ???
//#define EV_LIF_GSYN_GAP     NUM_LIF_EVENTS + 1
#define EV_LIFGAP_GSYN_GAP     3
//#define EV_LIFGAP_ACTIVITY  4


namespace PV {

class HyPerConn;

class LIFGap: public PV::LIF {
public:
   LIFGap(const char* name, HyPerCol * hc);
   virtual ~LIFGap();

   int virtual updateState(double time, double dt);

   int virtual checkpointWrite(bool suppressCheckpointIfConstant, char const * cpDir, double timestamp);
   int virtual readStateFromCheckpoint(const char * cpDir, double const * timeptr);

   const pvgsyndata_t * getGapStrength() { return gapStrength; }

protected:

   LIFGap();
   int initialize(const char * name, HyPerCol * hc, const char * kernel_name);
   virtual int allocateDataStructures() override;
   virtual int allocateConductances(int num_channels);
   virtual int readGapStrengthFromCheckpoint(const char * cpDir, double const * timeptr);
private:
   int initialize_base();
   int calcGapStrength();

   pvgsyndata_t * gapStrength;
   bool gapStrengthInitialized;
   std::vector<HyPerConn*> gapConnections;

}; // class LIFGap

} /* namespace PV */
#endif /* LIFGAP_HPP_ */
