#ifndef AVGPOOLTESTLAYER_HPP_ 
#define AVGPOOLTESTLAYER_HPP_

#include <layers/ANNLayer.hpp>

namespace PV {

class AvgPoolTestLayer: public PV::ANNLayer{
public:
   AvgPoolTestLayer(const char* name, HyPerCol * hc);

protected:
   int updateState(double timef, double dt);

private:
}; // end class AvgPoolTestLayer



} /* namespace PV */
#endif
