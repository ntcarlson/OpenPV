/*
 * CudaBuffer.cpp
 *
 *  Created on: Aug 6, 2014
 *      Author: Sheng Lundquist
 */

#include "CudaBuffer.hpp"
#include "cuda_util.hpp"
#include <sys/time.h>
#include <ctime>
#include <cmath>

namespace PVCuda {

CudaBuffer::CudaBuffer(size_t inSize, CudaDevice * inDevice)
{
   this->mSize = inSize;
   this->mDevice = inDevice;
   handleError(cudaMalloc(&mDevicePointer, mSize), "CudaBuffer constructor");
   if(!mDevicePointer){
      pvError().printf("Cuda Buffer allocation error\n");
   }
   // Do we need all three of these error checks?
}

CudaBuffer::~CudaBuffer()
{
   handleError(cudaFree(mDevicePointer), "Freeing device pointer");
}

int CudaBuffer::copyToDevice(const void * hostPointer)
{
   copyToDevice(hostPointer, this->mSize);
   return 0;
}

int CudaBuffer::copyToDevice(const void * hostPointer, size_t inSize)
{
   if(inSize > this->mSize){
      pvError().printf("copyToDevice, in_size of %zu is bigger than buffer size of %zu\n", inSize, this->mSize);
   }
   handleError(cudaMemcpyAsync(mDevicePointer, hostPointer, inSize, cudaMemcpyHostToDevice, mDevice->getStream()), "Copying buffer to device");
   return 0;
}

int CudaBuffer::copyFromDevice(void * hostPointer)
{
   copyFromDevice(hostPointer, this->mSize);
   return 0;
}

int CudaBuffer::copyFromDevice(void * hostPointer, size_t inSize)
{
   if(inSize > this->mSize){
      pvError().printf("copyFromDevice: in_size of %zu is bigger than buffer size of %zu\n", inSize, this->mSize);
   }
   handleError(cudaMemcpyAsync(hostPointer, mDevicePointer, inSize, cudaMemcpyDeviceToHost, mDevice->getStream()), "Copying buffer from device");
   return 0;
}

void CudaBuffer::permuteWeightsPVToCudnn(void * devicePointer, int numArbors, int numKernels, int nxp, int nyp, int nfp){
   //outFeatures is number of kernels
   int outFeatures = numKernels;

   //Rest is patch sizes
   int ny = nyp;
   int nx = nxp;
   int inFeatures = nfp;

   //Calculate grid and work size
   int numWeights = numArbors * outFeatures * ny * nx * inFeatures;
   //Ceil to get all weights
   int blockSize = mDevice->get_max_threads();
   int gridSize = std::ceil((float)numWeights/blockSize);
   //Call function
   callCudaPermuteWeightsPVToCudnn(gridSize, blockSize, devicePointer, numArbors, outFeatures, ny, nx, inFeatures);
   handleCallError("Permute weights PV to CUDNN");
}

} // namespace PV
