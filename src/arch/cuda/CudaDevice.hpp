/*
 * CudaDevice.hpp
 *
 *  Created on: July 30, 2014 
 *      Author: Sheng Lundquist
 */

#ifndef CUDADEVICE_HPP_
#define CUDADEVICE_HPP_

#include "../../include/pv_arch.h"
#include <stdio.h>
#include <cuda_runtime_api.h>

namespace PVCuda{
   
/**
 * A class to handle initialization of cuda devices
 */
class CudaDevice {

public:

   long reserveMem(size_t size);
   void incrementConvKernels();
   size_t getDeviceMemory(){return mDeviceMemory;}
   size_t getNumConvKernels(){return mNumConvKernels;}

   static int getNumDevices();

   /**
    * A constructor to create the device object
    * @param device The device number to use
    */
   CudaDevice(int device);
   virtual ~CudaDevice();

   /**
    * A function to initialize the device
    * @param device The device number to initialize
    */
   int initialize(int device);

   /**
    * A getter function to return what device is being used
    * @return The device number of the device being used
    */
   int id()  { return mDeviceId; }

// createBuffer removed Aug 3, 2016.  Instead, CudaBuffer's constuctor calls CudaDevice's reserveMem().

   /**
    * A function to return the cuda stream the device is using
    * @return The stream the device is using
    */
   cudaStream_t getStream(){return mStream;}
   
   /**
    * A synchronization barrier to block the cpu from running until the gpu stream has finished
    */
   void syncDevice();
  
   /**
    * A function to query all device information
    */
   int query_device_info();
   /**
    * A function to query a given device's information
    * @param id The device ID to get infromation from
    */
   void query_device(int id);

   /**
    * A getter function to return the max threads of the currently used device
    * @return The max number of threads on the device
    */
   int get_max_threads();

   /**
    * A getter function to return the max block size of a given dimension of the currently used device
    * @param dimension The dimension of the block size. Has to be 0-2 inclusive
    * @return The max block size of the given dimension on the device
    */
   int get_max_block_size_dimension(int dimension);

   /**
    * A getter function to return the max grid size of a given dimension of the currently used device
    * @param dimension The dimension of the grid size. Has to be 0-2 inclusive
    * @return The max grid size of the given dimension on the device
    */
   int get_max_grid_size_dimension(int dimension);

   /**
    * A getter function to return the warp size of the currently used device
    * @return The warp size of the device
    */
   int get_warp_size();

   /**
    * A getter function to return the local memory size of the currently used device
    * @return The local memory size of the device
    */
   size_t get_local_mem();

#ifdef PV_USE_CUDNN
   void* getCudnnHandle(){return mCudnnHandle;}
#endif

protected:
   int mDeviceId;                    // device id (normally 0 for GPU, 1 for CPU)
   int mNumDevices;                  // number of computing devices
   struct cudaDeviceProp mDeviceProperties;
   cudaStream_t mStream;
   long mDeviceMemory;
   size_t mNumConvKernels;

   void* mCudnnHandle;
};

} // namespace PV

#endif /* CLDEVICE_HPP_ */
