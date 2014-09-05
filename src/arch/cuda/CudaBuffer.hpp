/*
 * CudaBuffer.hpp
 *
 *  Created on: Aug 6, 2014
 *      Author: Sheng Lundquist
 */

#ifndef CUDABUFFER_HPP_
#define CUDABUFFER_HPP_

////////////////////////////////////////////////////////////////////////////////

namespace PVCuda {
#include <cuda_runtime_api.h>

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
   CudaBuffer(size_t inSize, cudaStream_t stream);
   CudaBuffer();
   virtual ~CudaBuffer();
   
   /**
    * A function to copy host memory to device memory. Note that the host and device memory must have the same size, otherwise undefined behavior
    * @param h_ptr The pointer to the host address to copy to the device
    * #return Returns PV_Success if successful
    */
   virtual int copyToDevice(void * h_ptr);

   /**
    * A function to copy device memory to host memory. Note that the host and device memory must have the same size, otherwise undefined behavior
    * @param h_ptr The pointer to the host address to copy from the device
    * #return Returns PV_Success if successful
    */
   virtual int copyFromDevice(void* h_ptr);

   /**
    * A getter function to return the device pointer allocated
    * #return Returns the device pointer
    */
   virtual void* getPointer(){return d_ptr;}

   /**
    * A getter function to return the size of the device memory
    * #return Returns the size of the device memory
    */
   size_t getSize(){return size;}

protected:
   void * d_ptr;                       // pointer to buffer on host
   size_t size;
   cudaStream_t stream;
};

} // namespace PV

#endif
