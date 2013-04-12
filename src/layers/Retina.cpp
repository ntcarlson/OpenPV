/*
 * Retina.cpp
 *
 *  Created on: Jul 29, 2008
 *
 */

#include "HyPerLayer.hpp"
#include "Retina.hpp"
#include "../io/io.h"
#include "../include/default_params.h"
#include "../utils/cl_random.h"
#include "../utils/pv_random.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void Retina_spiking_update_state (
    const int numNeurons,
    const double timed,
    const double dt,
    const int nx,
    const int ny,
    const int nf,
    const int nb,
    Retina_params * params,
    uint4 * rnd,
    float * GSynHead,
//    float * phiExc,
//    float * phiInh,
    float * activity,
    float * prevTime);

void Retina_nonspiking_update_state (
    const int numNeurons,
    const double timed,
    const double dt,
    const int nx,
    const int ny,
    const int nf,
    const int nb,
    Retina_params * params,
    float * GSynHead,
//    float * phiExc,
//    float * phiInh,
    float * activity);

#ifdef __cplusplus
}
#endif


namespace PV {

Retina::Retina() {
   initialize_base();
   // Default constructor to be called by derived classes.
   // It doesn't call Retina::initialize; instead, the derived class
   // should explicitly call Retina::initialize in its own initialization,
   // the way that Retina::initialize itself calls HyPerLayer::initialization.
   // This way, virtual methods called by initialize will be overridden
   // as expected.
}

Retina::Retina(const char * name, HyPerCol * hc) {
   initialize_base();
   initialize(name, hc, TypeRetina);
#ifdef PV_USE_OPENCL
   if(gpuAccelerateFlag)
      initializeGPU();
#endif
}

Retina::~Retina()
{
   free(rand_state);
 #ifdef PV_USE_OPENCL
   if((gpuAccelerateFlag)&&(spikingFlag)) {
      delete clRand;
   }
   // Moved to HyPerLayer since evList is a HyPerLayer member variable
//    free(evList);
 #endif
}

int Retina::initialize_base() {
   rand_state = NULL;
   spikingFlag = true;
   rParams.abs_refractory_period = 0.0f;
   rParams.refractory_period = 0.0f;
   rParams.beginStim = 0.0f;
   rParams.endStim = -1.0;
   rParams.burstDuration = 1000.0;
   rParams.burstFreq = 1.0f;
   rParams.probBase = 0.0f;
   rParams.probStim = 1.0f;
   return PV_SUCCESS;
}

int Retina::initialize(const char * name, HyPerCol * hc, PVLayerType type) {
   int status = HyPerLayer::initialize(name, hc, NUM_RETINA_CHANNELS);

   free(clayer->V);
   clayer->V = NULL;

   clayer->layerType = type;

   PVLayer * l = clayer;

   setParams(parent->parameters());

   // the size of the Retina may have changed due to size of image
   //
   const int nx = l->loc.nx;
   const int ny = l->loc.ny;
   const int nf = l->loc.nf;
   const int nb = l->loc.nb;
   l->numNeurons  = nx * ny * nf;
   l->numExtended = (nx + 2*nb) * (ny + 2*nb) * nf;

   // Commented out Nov. 29, 2012
   // // random seed should be different for different layers
   // unsigned int seed = (unsigned int) (parent->getRandomSeed() + getLayerId());

   // // a random state variable is needed for every neuron/clthread
   // rand_state = cl_random_init(numNeurons, seed);
   numGlobalRNGs = getNumGlobalNeurons();
   rand_state = (uint4 *) malloc(getNumNeurons() * sizeof(uint4));
   if (rand_state == NULL) {
      fprintf(stderr, "Retina::initialize error.  Layer \"%s\" unable to allocate memory for random states.\n", getName());
      exit(EXIT_FAILURE);
   }
   unsigned int seed = parent->getObjectSeed(getNumGlobalRNGs());
   const PVLayerLoc * loc = getLayerLoc();
   for (int y = 0; y<loc->ny; y++) {
      int k_local = kIndex(0, y, 0, loc->nx, loc->ny, loc->nf);
      int k_global = kIndex(loc->kx0, y+loc->ky0, 0, loc->nxGlobal, loc->nyGlobal, loc->nf);
      cl_random_init(&rand_state[k_local], loc->nx * loc->nf, seed + k_global);
   }

#ifdef PV_USE_OPENCL
   numEvents=NUM_RETINA_EVENTS;
//this code was moved to Hyperlayer:initializeGPU():
//   CLDevice * device = parent->getCLDevice();
//
//   numWait = 0;
//   numEvents = NUM_RETINA_EVENTS;
//   evList = (cl_event *) malloc(numEvents*sizeof(cl_event));
//   assert(evList != NULL);
//
//   // TODO - fix to use device and layer parameters
//   if (device->id() == 1) {
//      nxl = 1;  nyl = 1;
//   }
//   else {
//      nxl = 16; nyl = 8;
//   }
//
//   const char * kernel_name;
//   if (spikingFlag) {
//      kernel_name = "Retina_spiking_update_state";
//   }
//   else {
//      kernel_name = "Retina_nonspiking_update_state";
//   }
//
//   initializeThreadBuffers(kernel_name);
//   initializeThreadKernels(kernel_name);
#endif

   return status;
}

#ifdef PV_USE_OPENCL
/**
 * Initialize OpenCL buffers.  This must be called after PVLayer data have
 * been allocated.
 */
int Retina::initializeThreadBuffers(const char * kernel_name)
{
   int status = HyPerLayer::initializeThreadBuffers(kernel_name);

   CLDevice * device = parent->getCLDevice();

   // TODO - use constant memory --done!
   clParams = device->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(rParams), &rParams);
//   clParams->copyToDevice(&evUpdate);
//   status |= clWaitForEvents(1, &evUpdate);
//   clReleaseEvent(evUpdate);

   if (spikingFlag) {
      clRand   = device->createBuffer(CL_MEM_COPY_HOST_PTR, getNumNeurons()*sizeof(uint4), rand_state);
//      clRand->copyToDevice(&evUpdate);
//      status |= clWaitForEvents(1, &evUpdate);
//      clReleaseEvent(evUpdate);
   }

   return status;
}

int Retina::initializeThreadKernels(const char * kernel_name)
{
   char kernelPath[256];
   char kernelFlags[256];

   int status = CL_SUCCESS;
   CLDevice * device = parent->getCLDevice();

   const char * pvRelPath = "../PetaVision";
   sprintf(kernelPath,  "%s/%s/src/kernels/Retina_update_state.cl", parent->getPath(), pvRelPath);
   sprintf(kernelFlags, "-D PV_USE_OPENCL -cl-fast-relaxed-math -I %s/%s/src/kernels/", parent->getPath(), pvRelPath);

   // create kernels
   //
   krUpdate = device->createKernel(kernelPath, kernel_name, kernelFlags);
//kernel name should already be set correctly!
//   if (spikingFlag) {
//      krUpdate = device->createKernel(kernelPath, kernel_name, kernelFlags);
//   }
//   else {
//      krUpdate = device->createKernel(kernelPath, "Retina_nonspiking_update_state", kernelFlags);
//   }

   int argid = 0;

   status |= krUpdate->setKernelArg(argid++, getNumNeurons());
   status |= krUpdate->setKernelArg(argid++, parent->simulationTime());
   status |= krUpdate->setKernelArg(argid++, parent->getDeltaTime());

   status |= krUpdate->setKernelArg(argid++, clayer->loc.nx);
   status |= krUpdate->setKernelArg(argid++, clayer->loc.ny);
   status |= krUpdate->setKernelArg(argid++, clayer->loc.nf);
   status |= krUpdate->setKernelArg(argid++, clayer->loc.nb);

   status |= krUpdate->setKernelArg(argid++, clParams);
   if (spikingFlag) {
      status |= krUpdate->setKernelArg(argid++, clRand);
   }

   status |= krUpdate->setKernelArg(argid++, getChannelCLBuffer());
//   status |= krUpdate->setKernelArg(argid++, getChannelCLBuffer(CHANNEL_EXC));
//   status |= krUpdate->setKernelArg(argid++, getChannelCLBuffer(CHANNEL_INH));
   status |= krUpdate->setKernelArg(argid++, clActivity);
   if (spikingFlag) {
      status |= krUpdate->setKernelArg(argid++, clPrevTime);
   }

   return status;
}
#endif

int Retina::initializeState() {

   PVParams * params = parent->parameters();
   bool restart_flag = params->value(name, "restart", 0.0f) != 0.0f;
   if( restart_flag ) {
      double timef;
      readState(&timef);
   }
   else {
      updateState(parent->simulationTime(), parent->getDeltaTime());
   }

   return PV_SUCCESS;
}

int Retina::setParams(PVParams * p)
{
   double dt_sec = parent->getDeltaTime() * .001;  // seconds

   //clayer->loc.nf = 1;  // WTF???  who put this here???
   clayer->loc.nb = (int) p->value(name, "marginWidth", 0.0);

   clayer->params = &rParams;

   spikingFlag = (int) p->value(name, "spikingFlag", 1);

   float probStim = 1.0f;
   float probBase = 0.0f;
   if (p->present(name, "noiseOnFreq")) {
      fprintf(stderr, "Warning for retina layer \"%s\": parameter noiseOnFreq has been replaced by foregroundRate.\n", name);
      probStim = p->value(name, "noiseOnFreq");
   }
   else if (p->present(name, "poissonEdgeProb")) {
      fprintf(stderr, "Error in retina layer \"%s\": parameter poissonEdgeProb has been replaced by foregroundRate.\n", name);
      exit(EXIT_FAILURE);
      probStim = p->value(name, "poissonEdgeProb");
   }
   else {
      probStim = p->value(name, "foregroundRate", probStim);
   }
   if (p->present(name, "noiseOffFreq")) {
      fprintf(stderr, "Warning for retina layer \"%s\": parameter noiseOffFreq has been replaced by backgroundRate.\n", name);
      probBase = p->value(name, "noiseOffFreq");
   }
   else if (p->present(name, "poissonBlankProb")) {
      fprintf(stderr, "Error in retina layer \"%s\": parameter poissonBlankProb has been replaced by backgroundRate.\n", name);
      exit(EXIT_FAILURE);
      probBase = p->value(name, "poissonBlankProb");
   }
   else {
      probBase = p->value(name, "backgroundRate", probBase);
   }

   if (probBase > probStim) {
      fprintf(stderr, "Error in retina layer \"%s\": background rate should not be greater than foreground rate.\n", name);
      exit(EXIT_FAILURE);
   }
   probStim *= dt_sec;
   if (probStim > 1.0) probStim = 1.0f;
   probBase *= dt_sec;
   if (probBase > 1.0) probBase = 1.0f;

   maxRate = probStim/dt_sec;

   // default parameters
   //
   rParams.probStim  = probStim;
   rParams.probBase  = probBase;
   rParams.beginStim = p->value(name, "beginStim", 0.0f);
   rParams.endStim   = p->value(name, "endStim"  , FLT_MAX);
   if (rParams.endStim < 0) rParams.endStim = FLT_MAX;
   rParams.burstFreq = p->value(name, "burstFreq", 1);         // frequency of bursts
   rParams.burstDuration = p->value(name, "burstDuration", 1000); // duration of each burst, <=0 -> sinusoidal
   if (spikingFlag){
      rParams.refractory_period = p->value(name, "refractoryPeriod", REFRACTORY_PERIOD);
      rParams.abs_refractory_period = p->value(name, "absRefractoryPeriod",
            ABS_REFRACTORY_PERIOD);
   }

   return 0;
}

int Retina::checkpointRead(const char * cpDir, double * timef) {
   int status = HyPerLayer::checkpointRead(cpDir, timef);

   // Restore rand_state
   char filename[PV_PATH_MAX];
   int chars_needed = snprintf(filename, PV_PATH_MAX, "%s/%s_rand_state.bin", cpDir, name);
   if(chars_needed >= PV_PATH_MAX) {
      if (parent->icCommunicator()->commRank()==0) {
         fprintf(stderr, "HyPerLayer::checkpointRead error in layer \"%s\".  Base pathname \"%s/%s_rand_state.bin\" too long.\n", name, cpDir, name);
      }
      abort();
   }
   int rand_state_status = readRandState(filename, parent->icCommunicator(), rand_state, getLayerLoc());
   if (rand_state_status != PV_SUCCESS) status = rand_state_status;
   return status;
}

int Retina::checkpointWrite(const char * cpDir) {
   int status = HyPerLayer::checkpointWrite(cpDir);

   // Save rand_state
   char filename[PV_PATH_MAX];
   int chars_needed = snprintf(filename, PV_PATH_MAX, "%s/%s_rand_state.bin", cpDir, name);
   if(chars_needed >= PV_PATH_MAX) {
      if (parent->icCommunicator()->commRank()==0) {
         fprintf(stderr, "HyPerLayer::checkpointWrite error in layer \"%s\".  Base pathname \"%s/%s_rand_state.bin\" too long.\n", name, cpDir, name);
      }
      abort();
   }
   int rand_state_status = writeRandState(filename, parent->icCommunicator(), rand_state, getLayerLoc());
   if (rand_state_status != PV_SUCCESS) status = rand_state_status;
   return status;
}


int Retina::updateStateOpenCL(double time, double dt)
{
   int status = CL_SUCCESS;

#ifdef PV_USE_OPENCL
   // wait for memory to be copied to device
   if (numWait > 0) {
       status |= clWaitForEvents(numWait, evList);
   }
   for (int i = 0; i < numWait; i++) {
      clReleaseEvent(evList[i]);
   }
   numWait = 0;

   status |= krUpdate->setKernelArg(1, time);
   status |= krUpdate->setKernelArg(2, dt);
   status |= krUpdate->run(getNumNeurons(), nxl*nyl, 0, NULL, &evUpdate);
   krUpdate->finish();

   status |= getChannelCLBuffer()->copyFromDevice(1, &evUpdate, &evList[getEVGSyn()]);
//   status |= getChannelCLBuffer(CHANNEL_EXC)->copyFromDevice(1, &evUpdate, &evList[getEVGSynE()]);
//   status |= getChannelCLBuffer(CHANNEL_INH)->copyFromDevice(1, &evUpdate, &evList[getEVGSynI()]);
   status |= clActivity->copyFromDevice(1, &evUpdate, &evList[getEVActivity()]);
   numWait += 2;

#if PV_CL_COPY_BUFFERS
   status |= clPhiE    ->copyFromDevice(1, &evUpdate, &evList[EV_R_PHI_E]);
   status |= clPhiI    ->copyFromDevice(1, &evUpdate, &evList[EV_R_PHI_I]);
   status |= clActivity->copyFromDevice(1, &evUpdate, &evList[EV_R_ACTIVITY]);
   numWait += 3;
#endif
#endif

   return status;
}

int Retina::triggerReceive(InterColComm* comm)
{
   int status = HyPerLayer::triggerReceive(comm);


   // copy data to device
   //
#ifdef PV_USE_OPENCL
#if PV_CL_COPY_BUFFERS
   status |= clPhiE->copyToDevice(&evList[EV_R_PHI_E]);
   status |= clPhiI->copyToDevice(&evList[EV_R_PHI_I]);
   numWait += 2;
#endif
#endif

   return status;
}

int Retina::waitOnPublish(InterColComm* comm)
{
   int status = HyPerLayer::waitOnPublish(comm);

   // copy activity to device
   //
#ifdef PV_USE_OPENCL
#if PV_CL_COPY_BUFFERS
   status |= clActivity->copyToDevice(&evList[EV_R_ACTIVITY]);
   numWait += 1;
#endif
#endif

   return status;
}

//! Updates the state of the Retina
/*!
 * REMARKS:
 *      - prevActivity[] buffer holds the time when a neuron last spiked.
 *      - not used if nonspiking
 *      - it sets the probStim and probBase.
 *              - probStim = noiseOnFreq * dt_sec * (phiExc - phiInh); the last ()  is V[k];
 *              - probBase = noiseOffFreq * dt_sec;
 *              .
 *      - activity[] is set to 0 or 1 depending on the return of spike()
 *      - this depends on the last time a neuron spiked as well as on V[]
 *      at the location of the neuron. This V[] is set by calling updateImage().
 *      - V points to the same memory space as data in the Image so that when Image
 *      is updated, V gets updated too.
 *      .
 *      .
 *
 *
 */
int Retina::updateState(double timed, double dt)
{
   update_timer->start();
#ifdef PV_USE_OPENCL
   if((gpuAccelerateFlag)&&(true)) {
      updateStateOpenCL(timed, dt);
   }
   else {
#endif // PV_USE_OPENCL
      const int nx = clayer->loc.nx;
      const int ny = clayer->loc.ny;
      const int nf = clayer->loc.nf;
      const int nb = clayer->loc.nb;

      pvdata_t * GSynHead   = GSyn[0];
      pvdata_t * activity = clayer->activity->data;

      if (spikingFlag == 1) {
         Retina_spiking_update_state(getNumNeurons(), timed, dt, nx, ny, nf, nb,
                                     &rParams, rand_state,
                                     GSynHead, activity, clayer->prevActivity);
      }
      else {
         Retina_nonspiking_update_state(getNumNeurons(), timed, dt, nx, ny, nf, nb,
                                        &rParams, GSynHead, activity);
      }
#ifdef PV_USE_OPENCL
   }
#endif // PV_USE_OPENCL



#ifdef DEBUG_PRINT
   char filename[132];
   sprintf(filename, "r_%d.tiff", (int)(2*timed));
   this->writeActivity(filename, timed);

   printf("----------------\n");
   for (int k = 0; k < 6; k++) {
      printf("host:: k==%d h_exc==%f h_inh==%f\n", k, phiExc[k], phiInh[k]);
   }
   printf("----------------\n");

#endif // DEBUG_PRINT
   update_timer->stop();
   updateActiveIndices();
   return 0;
}

int Retina::updateBorder(double time, double dt)
{
   // wait for OpenCL data transfers to finish
   HyPerLayer::updateBorder(time, dt);

   // Data has arrived from OpenCL device now safe to add background
   // activity to border regions.  Update all of the regions regions
   // even if using MPI and the border may be from an adjacent processor.
   // TODO - check that MPI will overwrite the border regions after this
   // function has been called.

   const PVLayerLoc * loc = getLayerLoc();

   const int nb = loc->nb;
   if (nb == 0) return 0;

   const int nx = loc->nx;
   const int ny = loc->ny;
   const int nf = loc->nf;

   const int nx_ex = nx + 2*nb;
   const int sy = nf*nx_ex;

   pvdata_t * activity_top = &clayer->activity->data[0];
   pvdata_t * activity_bot = &clayer->activity->data[(nb+ny)*sy];

   pvdata_t * activity_l = &clayer->activity->data[nb*sy];
   pvdata_t * activity_r = &clayer->activity->data[nb*sy + nb+nx];

   // top and bottom borders (including corners)
   for (int kex = 0; kex < nx_ex*nf*nb; kex++) {
      activity_top[kex] = (pv_random_prob() < rParams.probBase) ? 1.0 : 0.0;
      activity_bot[kex] = (pv_random_prob() < rParams.probBase) ? 1.0 : 0.0;
   }

   // left and right borders
   for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nf*nb; x++) {
         activity_l[x] = (pv_random_prob() < rParams.probBase) ? 1.0 : 0.0;
         activity_r[x] = (pv_random_prob() < rParams.probBase) ? 1.0 : 0.0;
      }
      activity_l += sy;
      activity_r += sy;
   }

   return 0;
}

#ifdef OBSOLETE // Marked obsolete Jul 13, 2012.  Dumping the state is now done by CheckpointWrite.
int Retina::writeState(double timef, bool last)
{
   int status = HyPerLayer::writeState(timef, last);

   // print activity at center of image

#ifdef DEBUG_OUTPUT
   int sx = clayer->loc.nf;
   int sy = sx*clayer->loc.nx;
   pvdata_t * a = clayer->activity->data;

   for (int k = 0; k < clayer->numExtended; k++) {
      if (a[k] == 1.0) printf("a[%d] == 1\n", k);
   }

  int n = (int) (sy*(clayer->loc.ny/2 - 1) + sx*(clayer->loc.nx/2));
  for (int f = 0; f < clayer->loc.nf; f++) {
     printf("a[%d] = %f\n", n, a[n]);
     n += 1;
  }
#endif

   return status;
}
#endif // OBSOLETE

int Retina::outputState(double time, bool last)
{
   // if( spikingFlag ) updateActiveIndices(); // updateActiveIndices moved back into updateState.
   return HyPerLayer::outputState(time, last);
}

} // namespace PV

///////////////////////////////////////////////////////
//
// implementation of Retina kernels
//

#ifdef __cplusplus
extern "C" {
#endif

//#ifndef PV_USE_OPENCL
//#  include "../kernels/Retina_update_state.cl"
//#  include "../kernels/Retina_update_state.c"
//#endif
#ifndef PV_USE_OPENCL
#  include "../kernels/Retina_update_state.cl"
#else
#  undef PV_USE_OPENCL
#  include "../kernels/Retina_update_state.cl"
#  define PV_USE_OPENCL
#endif

#ifdef __cplusplus
}
#endif
