#ifndef BINNINGLAYER_HPP_
#define BINNINGLAYER_HPP_

#include "HyPerLayer.hpp"

namespace PV {

class BinningLayer: public PV::HyPerLayer {
public:
   BinningLayer(const char * name, HyPerCol * hc);
   virtual int allocateDataStructures();
   virtual int requireMarginWidth(int marginWidthNeeded, int * marginWidthResult, char axis);
   virtual bool activityIsSpiking() { return false; }
   virtual ~BinningLayer();

protected:
   BinningLayer();
   int initialize(const char * name, HyPerCol * hc);
   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag) override;
   void ioParam_originalLayerName(enum ParamsIOFlag ioFlag);
   void ioParam_binMaxMin(enum ParamsIOFlag ioFlag);
   void ioParam_delay(enum ParamsIOFlag ioFlag);
   void ioParam_binSigma(enum ParamsIOFlag ioFlag);
   void ioParam_zeroNeg(enum ParamsIOFlag ioFlag);
   void ioParam_zeroDCR(enum ParamsIOFlag ioFlag);
   void ioParam_normalDist(enum ParamsIOFlag ioFlag);
   virtual int communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) override;
   int allocateV();
   int initializeV();
   virtual int initializeActivity();
   virtual int updateState(double timef, double dt);
   virtual int doUpdateState(double timed, double dt, const PVLayerLoc * origLoc,
         const PVLayerLoc * currLoc, const pvdata_t * origData, pvdata_t * currV, float binMax, float binMin);

   float getSigma(){return binSigma;}
   float calcNormDist(float xVal, float mean, float binSigma);
private:
   int initialize_base();
   int delay;
   float binMax;
   float binMin;
   float binSigma;
   bool zeroNeg;
   bool zeroDCR;
   bool normalDist;

protected:
   char * originalLayerName;
   HyPerLayer * originalLayer;
}; // class BinningLayer

} /* namespace PV */
#endif 
