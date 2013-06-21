/*
 * ANNErrorLayer.cpp
 *
 *  Created on: Jun 21, 2013
 *      Author: gkenyon
 */

#include "ANNErrorLayer.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void ANNErrorLayer_update_state(
    const int numNeurons,
    const int nx,
    const int ny,
    const int nf,
    const int nb,

    float * V,
    const float Vth,
    const float VMax,
    const float VMin,
    const float VShift,
    float * GSynHead,
    float * activity);


#ifdef __cplusplus
}
#endif

namespace PV {

ANNErrorLayer::ANNErrorLayer()
{
   initialize_base();
}

ANNErrorLayer::ANNErrorLayer(const char * name, HyPerCol * hc, int num_channels)
{
   initialize_base();
   assert(num_channels == 2);
   initialize(name, hc, num_channels);
}

ANNErrorLayer::ANNErrorLayer(const char * name, HyPerCol * hc)
{
   initialize_base();
   initialize(name, hc, 2);
}

ANNErrorLayer::~ANNErrorLayer()
{
}

int ANNErrorLayer::initialize_base()
{
   return PV_SUCCESS;
}

int ANNErrorLayer::initialize(const char * name, HyPerCol * hc, int num_channels)
{
   ANNLayer::initialize(name, hc, num_channels);
   return PV_SUCCESS;
}

int ANNErrorLayer::doUpdateState(double time, double dt, const PVLayerLoc * loc, pvdata_t * A,
      pvdata_t * V, int num_channels, pvdata_t * gSynHead, bool spiking,
      unsigned int * active_indices, unsigned int * num_active)
{
   update_timer->start();
#ifdef PV_USE_OPENCL
   if(gpuAccelerateFlag) {
      updateStateOpenCL(time, dt);
      //HyPerLayer::updateState(time, dt);
   }
   else {
#endif
      int nx = loc->nx;
      int ny = loc->ny;
      int nf = loc->nf;
      int num_neurons = nx*ny*nf;
    	  ANNErrorLayer_update_state(num_neurons, nx, ny, nf, loc->nb, V, VThresh,
    			  VMax, VMin, VShift, gSynHead, A);
      if (this->writeSparseActivity){
         updateActiveIndices();  // added by GTK to allow for sparse output, can this be made an inline function???
      }
#ifdef PV_USE_OPENCL
   }
#endif

   update_timer->stop();
   return PV_SUCCESS;
}

} /* namespace PV */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef PV_USE_OPENCL
#  include "../kernels/ANNErrorLayer_update_state.cl"
#else
#  undef PV_USE_OPENCL
#  include "../kernels/ANNErrorLayer_update_state.cl"
#  define PV_USE_OPENCL
#endif

#ifdef __cplusplus
}
#endif
