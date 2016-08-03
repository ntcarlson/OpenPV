/*
 * CudaBuffer.hpp
 *
 *  Created on: Aug 6, 2014
 *      Author: Sheng Lundquist
 */

#ifndef CUDABUFFER_HPP_
#define CUDABUFFER_HPP_

#include "arch/cuda/CudaDevice.hpp"
#include <cuda_runtime_api.h>

////////////////////////////////////////////////////////////////////////////////

namespace PVCuda {

class CudaDevice;

/**
 * A class to handle device memory allocations and transfers
 */
class CudaBuffer {

public:

   /**
    * Constructor to create a buffer of size inSize on a given stream
    * @param inSize The size of the buffer to create on the device
    * @param stream The cuda stream any transfer commands should go on
    */
   CudaBuffer(size_t inSize, CudaDevice * inDevice);
   virtual ~CudaBuffer();
   
   /**
    * A function to copy host memory to device memory. Note that the host and device memory must have the same size, otherwise undefined behavior
    * @param hostPointer The pointer to the host address to copy to the device
    * #return Returns PV_Success if successful
    */
   virtual int copyToDevice(const void * hostPointer);

   /**
    * A function to copy device memory to host memory. Note that the host and device memory must have the same size, otherwise undefined behavior
    * @param hostPointer The pointer to the host address to copy from the device
    * @param inSize The size of the data to copy. Defaults to the size of the buffer.
    * #return Returns PV_Success if successful
    */
   virtual int copyFromDevice(void* hostPointer);
   virtual int copyFromDevice(void* hostPointer, size_t inSize);

   /**
    * A getter function to return the device pointer allocated
    * #return Returns the device pointer
    */
   virtual void* getDevicePointer(){return mDevicePointer;}

   /**
    * A getter function to return the size of the device memory
    * #return Returns the size of the device memory
    */
   size_t getSize(){return mSize;}

   void permuteWeightsPVToCudnn(void *d_inPtr, int numArbors, int numKernels, int nxp, int nyp, int nfp);

private:
   /**
    * A function to copy host memory to device memory. Note that the host and device memory must have the same size, otherwise undefined behavior
    * @param hostPointer The pointer to the host address to copy to the device
    * @param inSize The size of the data to copy. Defaults to the size of the buffer.
    * #return Returns PV_Success if successful
    */
   virtual int copyToDevice(const void * hostPointer, size_t inSize);
   void callCudaPermuteWeightsPVToCudnn(int gridSize, int blockSize, void* d_inPtr, int numArbors, int outFeatures, int ny, int nx, int inFeatures);

protected:
   void * mDevicePointer;             // pointer to buffer on device
   size_t mSize;
   CudaDevice * mDevice;
}; // end class CudaBuffer

} // end namespace PVCuda

#endif
