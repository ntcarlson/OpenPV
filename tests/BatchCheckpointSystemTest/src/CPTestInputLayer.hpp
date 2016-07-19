/*
 * CPTestInputLayer.hpp
 *
 *  Created on: Nov 10, 2011
 *      Author: pschultz
 */

#ifndef CPTESTINPUTLAYER_HPP_
#define CPTESTINPUTLAYER_HPP_

#include <layers/ANNLayer.hpp>

namespace PV {

class CPTestInputLayer : public ANNLayer {

public:
   CPTestInputLayer(const char * name, HyPerCol * hc);
   virtual ~CPTestInputLayer();
   virtual int allocateDataStructures();
   virtual int updateState(double timed, double dt);

protected:
   int initialize(const char * name, HyPerCol * hc);
   virtual int initializeV();

}; // end class CPTestInputLayer

BaseObject * createCPTestInputLayer(char const * name, HyPerCol * hc);

}  // end of namespace PV block


#endif /* CPTESTINPUTLAYER_HPP_ */
