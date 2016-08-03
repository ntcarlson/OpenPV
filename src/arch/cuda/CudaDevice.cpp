/*
 * CudaDevice.cu
 *
 *  Created on: Aug 5, 2014
 *      Author: Sheng Lundquist
 */

#include "cuda_util.hpp"
#include "CudaDevice.hpp"

#ifdef PV_USE_CUDNN
#include <cudnn.h>
#endif

namespace PVCuda{

CudaDevice* CudaDevice::instance() {
   static CudaDevice * singleton = new CudaDevice();
   return singleton;
}

CudaDevice::CudaDevice() {
   mDeviceId = -1;
   mNumDevices = 0;
   mStream = nullptr;
   mNumConvKernels = (size_t) 0;
   mCudnnHandle = nullptr;
}

CudaDevice::~CudaDevice()
{
   handleError(cudaStreamDestroy(mStream), "Cuda Device Destructor");
   //TODO set handleError to take care of this

#ifdef PV_USE_CUDNN
   if(mCudnnHandle){
      cudnnDestroy((cudnnHandle_t)mCudnnHandle);
   }
#endif
}

void CudaDevice::initialize(int deviceId)
{
   if (mDeviceId>=0 && deviceId!=mDeviceId) {
      pvError() << "CudaDevice has already been initialized to device " << mDeviceId << ".\n";
   }
   mDeviceId = deviceId;
   mCudnnHandle = nullptr;

#ifdef PV_USE_CUDA
   handleError(cudaThreadExit(), "Thread exiting in initialize");

   handleError(cudaGetDeviceCount(&mNumDevices), "Getting device count");
   handleError(cudaSetDevice(deviceId), "Setting device");

   handleError(cudaStreamCreate(&mStream), "Creating stream");

   handleError(cudaGetDeviceProperties(&mDeviceProperties, deviceId), "Getting device properties");

#endif // PV_USE_CUDA
   
#ifdef PV_USE_CUDNN
   //Testing cudnn here
   cudnnHandle_t tmpHandle;
   cudnnStatus_t cudnnStatus = cudnnCreate(&tmpHandle); 
   if(cudnnStatus != CUDNN_STATUS_SUCCESS){
      pvError(cudnnCreateError);
      switch(cudnnStatus){
         case CUDNN_STATUS_NOT_INITIALIZED:
            cudnnCreateError.printf("cuDNN Runtime API initialization failed\n");
            break;
         case CUDNN_STATUS_ALLOC_FAILED:
            cudnnCreateError.printf("cuDNN resources could not be allocated\n");
            break;
         default:
            cudnnCreateError.printf("cudnnCreate error: %s\n", cudnnGetErrorString(cudnnStatus));
            break;
      }
      exit(EXIT_FAILURE);
   }
   cudnnStatus = cudnnSetStream(tmpHandle, mStream);
   if(cudnnStatus != CUDNN_STATUS_SUCCESS){
      pvError().printf("cudnnSetStream error: %s\n", cudnnGetErrorString(cudnnStatus));
   }

   this->mCudnnHandle = (void*) tmpHandle;
#endif
}

void CudaDevice::incrementConvKernels(){
   mNumConvKernels++;
}

int CudaDevice::getNumDevices(){
   int returnVal;
   handleError(cudaGetDeviceCount(&returnVal), "Static getting device count");
   return returnVal;
}

void CudaDevice::syncDevice(){
   handleError(cudaDeviceSynchronize(), "Synchronizing device");
}

int CudaDevice::query_device_info()
{
   // query and print information about the devices found
   //
   pvInfo().printf("\n");
   pvInfo().printf("Number of Cuda devices found: %d\n", mNumDevices);
   pvInfo().printf("\n");

   for (unsigned int i = 0; i < mNumDevices; i++) {
      query_device(i);
   }
   return 0;
}

void CudaDevice::query_device(int id)
{
   struct cudaDeviceProp props;
   //Use own props if current device
   if(id == mDeviceId){
      props = mDeviceProperties;
   }
   //Otherwise, generate props
   else{
      handleError(cudaGetDeviceProperties(&props, id), "Getting device properties");
   }
   pvInfo().printf("device: %d\n", id);
   pvInfo().printf("CUDA Device # %d == %s\n", id, props.name);

   pvInfo().printf("with %d units/cores", props.multiProcessorCount);

   pvInfo().printf(" at %f MHz\n", (float)props.clockRate * .001);

   pvInfo().printf("\tMaximum threads group size == %d\n", props.maxThreadsPerBlock);
   
   pvInfo().printf("\tMaximum grid sizes == (");
   for (unsigned int i = 0; i < 3; i++) pvInfo().printf(" %d", props.maxGridSize[i]);
   pvInfo().printf(" )\n");

   pvInfo().printf("\tMaximum threads sizes == (");
   for (unsigned int i = 0; i < 3; i++) pvInfo().printf(" %d", props.maxThreadsDim[i]);
   pvInfo().printf(" )\n");

   pvInfo().printf("\tLocal mem size == %zu\n", props.sharedMemPerBlock);

   pvInfo().printf("\tGlobal mem size == %zu\n", props.totalGlobalMem);

   pvInfo().printf("\tConst mem size == %zu\n", props.totalConstMem);

   pvInfo().printf("\tRegisters per block == %d\n", props.regsPerBlock);

   pvInfo().printf("\tWarp size == %d\n", props.warpSize);

   pvInfo().printf("\n");
}

int CudaDevice::get_max_threads(){
   return mDeviceProperties.maxThreadsPerBlock;
}

int CudaDevice::get_max_block_size_dimension(int dimension){
   if(dimension < 0 || dimension >= 3) return 0;
   return mDeviceProperties.maxThreadsDim[dimension];
}

int CudaDevice::get_max_grid_size_dimension(int dimension){
   if(dimension < 0 || dimension >= 3) return 0;
   return mDeviceProperties.maxGridSize[dimension];
}

int CudaDevice::get_warp_size(){
   return mDeviceProperties.warpSize;
}

size_t CudaDevice::get_local_mem(){
   return mDeviceProperties.sharedMemPerBlock;
}

}
