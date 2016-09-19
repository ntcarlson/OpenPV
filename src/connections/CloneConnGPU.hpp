#ifndef _CLONECONNGPU_HPP_
#define _CLONECONNGPU_HPP_

#include "HyPerConnGPU.hpp"

namespace GPULCA {

class CloneConnGPU : public HyPerConnGPU {
 public:
  CloneConnGPU(const char* name, PV::HyPerCol* hc);
  virtual ~CloneConnGPU();
  virtual int communicateInitInfo();
  virtual int allocateDataStructures();
  virtual int deliver();
  HyPerConnGPU* getOriginalConn() { return originalConn; }

 protected:
  CloneConnGPU();
  virtual int cloneParameters();

 private:
  char* originalConnName;
  HyPerConnGPU* originalConn;

  /*  CUDA Handler  */
  const cudnnTensorDescriptor_t* cudnnTensorDescriptorPreP,
      *cudnnTensorDescriptorPostP;
};
}

#endif  // _CLONECONNGPU_HPP_