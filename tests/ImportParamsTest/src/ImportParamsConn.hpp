#ifndef IMPORTPARAMSCONN_HPP_ 
#define IMPORTPARAMSCONN_HPP_

#include <connections/HyPerConn.hpp>

namespace PV {

class ImportParamsConn: public PV::HyPerConn{
public:
   ImportParamsConn(const char* name, HyPerCol * hc);
   virtual int allocateDataStructures();

protected:
   int initialize(const char * name, HyPerCol * hc);
   virtual int communicateInitInfo(CommunicateInitInfoMessage<Observer*> const * message);

private:
   int initialize_base();
};

} /* namespace PV */
#endif
