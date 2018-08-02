/*
 * KernelPactchProbe.cpp
 *
 *  Created on: Oct 21, 2011
 *      Author: pschultz
 */

#include "VisualKernelProbe.hpp"

namespace PV {

VisualKernelProbe::VisualKernelProbe() { initialize_base(); }

VisualKernelProbe::VisualKernelProbe(const char *probename, HyPerCol *hc) {
   initialize_base();
   int status = initialize(probename, hc);
   assert(status == PV_SUCCESS);
}

VisualKernelProbe::~VisualKernelProbe() {}

int VisualKernelProbe::initialize_base() { return PV_SUCCESS; }

int VisualKernelProbe::initialize(const char *probename, HyPerCol *hc) {
   int status = BaseConnectionProbe::initialize(probename, hc);
   assert(name && parent);

   return status;
}

int VisualKernelProbe::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = BaseConnectionProbe::ioParamsFillGroup(ioFlag);
   ioParam_arborId(ioFlag);
   ioParam_numFeatureX(ioFlag);
   ioParam_numFeatureY(ioFlag);
   ioParam_featureScale(ioFlag);
   return status;
}

void VisualKernelProbe::ioParam_arborId(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "arborId", &arborID, 0);
}

void VisualKernelProbe::ioParam_numFeatureX(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "numFeatureX", &numFeatureX, 0);
}

void VisualKernelProbe::ioParam_numFeatureY(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "numFeatureY", &numFeatureY, 0);
}

void VisualKernelProbe::ioParam_featureScale(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(ioFlag, name, "featureScale", &featureScale, 0);
}

void VisualKernelProbe::initNumValues() { setNumValues(-1); }

Response::Status
VisualKernelProbe::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   auto status = BaseHyPerConnProbe::communicateInitInfo(message);
   if (!Response::completed(status)) {
      return status;
   }
   auto *targetHyPerConn = getTargetHyPerConn();
   assert(targetHyPerConn);
   if (targetHyPerConn->getSharedWeights() == false) {
      if (parent->getCommunicator()->globalCommRank() == 0) {
         ErrorLog().printf(
               "%s: %s is not using shared weights.\n",
               getDescription_c(),
               targetHyPerConn->getDescription_c());
      }
      MPI_Barrier(parent->getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
   return Response::SUCCESS;
}

Response::Status VisualKernelProbe::allocateDataStructures() {
   auto status = BaseHyPerConnProbe::allocateDataStructures();
   if (!Response::completed(status)) {
      return status;
   }
   auto *targetHyPerConn = getTargetHyPerConn();
   assert(targetHyPerConn);
   if (numFeatureX * numFeatureY != targetHyPerConn->getNumDataPatches()) {
      Fatal().printf("VisualKernelProbe \"%s\": feaure dimensions (%d by %d)"
              " do not match expected total (%d)\n",
              name, numFeatureX, numFeatureY,
              targetHyPerConn->getNumDataPatches());
   }
   if (getArbor() < 0 || getArbor() >= getTargetHyPerConn()->getNumAxonalArbors()) {
      Fatal().printf(
            "KernelProbe \"%s\": arborId %d is out of bounds. (min 0, max %d)\n",
            name,
            getArbor(),
            getTargetHyPerConn()->getNumAxonalArbors() - 1);
   }
   if (featureScale <= 0) {
      Fatal().printf(
            "KernelProbe \"%s\": featureScale is out of bounds. (must be > 0)\n",
            name);
   }

   int nxp        = targetHyPerConn->getPatchSizeX();
   int nyp        = targetHyPerConn->getPatchSizeY();
   render_width = numFeatureX * featureScale * nxp ;
   render_height = numFeatureY * featureScale * nyp;


   Communicator *icComm  = parent->getCommunicator();
   const int rank = icComm->globalCommRank();
   if (rank == 0) {
      SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
      renderer = SDL_SetVideoMode(render_width, render_height, 32, SDL_DOUBLEBUF);
      pixels = (uint32_t *) renderer->pixels;
   }
   return Response::SUCCESS;
}

Response::Status VisualKernelProbe::outputState(double timed) {
   Communicator *icComm  = parent->getCommunicator();
   const int rank = icComm->globalCommRank();
   if (rank != 0) {
      return Response::NO_ACTION;
   }
   if (mOutputStreams.empty()) {
      return Response::NO_ACTION;
   }
   auto *targetHyPerConn = getTargetHyPerConn();
   assert(targetHyPerConn != nullptr);
   int numKernels = getTargetHyPerConn()->getNumDataPatches();
   int nxp        = targetHyPerConn->getPatchSizeX();
   int nyp        = targetHyPerConn->getPatchSizeY();
   int nfp        = targetHyPerConn->getPatchSizeF();
   int patchSize  = nxp * nyp * nfp;
   int numWeights = patchSize * numKernels;
   
   const float *wdata = targetHyPerConn->getWeightsDataStart(arborID);

   // Find max/min values needed to rescale the kernels into 24 bit RGB images
    float min_w = wdata[0];
    float max_w = wdata[0];
#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for reduction(max: max_w), reduction(min: min_w)
#endif
   for (int k = 0; k < numWeights; k++) {
      if (wdata[k] > max_w) max_w = wdata[k];
      if (wdata[k] < min_w) min_w = wdata[k];
   }

#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for
#endif
   for (int kernelIndex = 0; kernelIndex < numKernels; kernelIndex++) {
      const float *kernel_data = wdata + patchSize * kernelIndex;

      // Start x,y cordinates on the canvas
      int start_x = nxp*featureScale*(kernelIndex % numFeatureX);
      int start_y = nyp*featureScale*(kernelIndex / numFeatureX);

      for (int y = 0; y < nyp; y++) {
         for (int x = 0; x < nxp; x++) {
            int k = kIndex(x, y, 0, nxp, nyp, nfp);
            // Rescale floats to 24 bit RGB
            uint8_t red   = ((kernel_data[k]   - min_w) * 255.0)/(max_w - min_w);
            uint8_t green = ((kernel_data[k+1] - min_w) * 255.0)/(max_w - min_w);
            uint8_t blue  = ((kernel_data[k+2] - min_w) * 255.0)/(max_w - min_w);
            uint32_t rgb = (red << 16) | (green << 8) | (blue);

            // Draw scaled pixel to canvas
            for (int sx = 0; sx < featureScale; sx++) {
               for (int sy = 0; sy < featureScale; sy++) {
                  int canvas_x = start_x + featureScale*x + sx;
                  int canvas_y = start_y + featureScale*y + sy;
                  pixels[canvas_x + canvas_y*render_width] = rgb;
               }
            }
         }
      }
   }

   SDL_UpdateRect(renderer, 0, 0, render_width, render_height);

   return Response::SUCCESS;
}

} // End namespace PV