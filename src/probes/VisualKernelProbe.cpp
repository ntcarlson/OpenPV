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
   SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
   renderer = SDL_SetVideoMode(render_width, render_height, 32, SDL_DOUBLEBUF);
   pixels = (uint32_t *) renderer->pixels;


   return Response::SUCCESS;
}

Response::Status VisualKernelProbe::outputState(double timed) {
   if (mOutputStreams.empty()) {
      return Response::NO_ACTION;
   }
   Communicator *icComm  = parent->getCommunicator();
   const int rank        = icComm->commRank();
   auto *targetHyPerConn = getTargetHyPerConn();
   assert(targetHyPerConn != nullptr);
   int numKernels = getTargetHyPerConn()->getNumDataPatches();
   int nxp        = targetHyPerConn->getPatchSizeX();
   int nyp        = targetHyPerConn->getPatchSizeY();
   int nfp        = targetHyPerConn->getPatchSizeF();
   int patchSize  = nxp * nyp * nfp;

   int kernelIndex;
   for (kernelIndex = 0; kernelIndex < numKernels; kernelIndex++) {
       const float *wdata = targetHyPerConn->getWeightsDataStart(arborID) + patchSize * kernelIndex;

       int start_x = nxp*featureScale*(kernelIndex % numFeatureX);
       int start_y = nyp*featureScale*(kernelIndex / numFeatureX);

       float min_w = wdata[0];
       float max_w = wdata[0];
       for (int f = 0; f < nfp; f++) {
          for (int y = 0; y < nyp; y++) {
             for (int x = 0; x < nxp; x++) {
                int k = kIndex(x, y, f, nxp, nyp, nfp);
                min_w = std::min(min_w, wdata[k]);
                max_w = std::max(max_w, wdata[k]);
             }
          }
       }

       for (int y = 0; y < nyp; y++) {
          for (int x = 0; x < nxp; x++) {
             uint32_t rgb = 0;
             for (int f = 0; f < nfp; f++) {
                int k = kIndex(x, y, f, nxp, nyp, nfp);
                uint8_t color_val = (uint8_t) (((wdata[k] - min_w) * 255.0)/(max_w - min_w));
                rgb |= (color_val << 8*f);
                output(0) << (int) color_val << ", ";
             }
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
