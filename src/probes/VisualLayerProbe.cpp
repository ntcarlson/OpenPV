/*
 * VisualLayerProbe.cpp
 */

#include "VisualLayerProbe.hpp"
#include "columns/HyPerCol.hpp"
#include "layers/HyPerLayer.hpp"

namespace PV {

VisualLayerProbe::VisualLayerProbe() {
   initialize_base();
}

VisualLayerProbe::VisualLayerProbe(const char *name, HyPerCol *hc) {
   initialize_base();
   initialize(name, hc);
}

VisualLayerProbe::~VisualLayerProbe() {}

int VisualLayerProbe::initialize_base() {
   return PV_SUCCESS;
	targetLayer2 = NULL;
	targetLayer3 = NULL;
}

int VisualLayerProbe::initialize(const char *name, HyPerCol *hc) {
   return LayerProbe::initialize(name, hc);
}

int VisualLayerProbe::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = LayerProbe::ioParamsFillGroup(ioFlag);
   ioParam_scale(ioFlag);
   ioParam_targetName2(ioFlag);
   ioParam_targetName3(ioFlag);
   return status;
}

void VisualLayerProbe::ioParam_scale(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(
         ioFlag, name, "scale", &scale, 1 /*default*/);
}

void VisualLayerProbe::ioParam_targetName2(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamString(ioFlag, name, "targetName2", &targetName2, NULL);
}

void VisualLayerProbe::ioParam_targetName3(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamString(ioFlag, name, "targetName3", &targetName3, NULL);
}


Response::Status
VisualLayerProbe::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   auto status = LayerProbe::communicateInitInfo(message);
   if (!Response::completed(status)) {
      return status;
   }
   assert(targetLayer);

   render_width  = getTargetLayer()->getLayerLoc()->nx;
   render_height = getTargetLayer()->getLayerLoc()->ny;

   if (targetName2 != NULL && strcmp(targetName2, "") != 0) {
      targetLayer2 = message->lookup<HyPerLayer>(std::string(targetName2));
      if (targetLayer2 == NULL) {
         if (parent->columnId() == 0) {
            ErrorLog().printf(
                  "%s: targetLayer2 \"%s\" is not a layer in the column.\n",
                  getDescription_c(),
                  targetName2);
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }


      const PVLayerLoc *mainLoc = targetLayer->getLayerLoc();
      const PVLayerLoc *loc    = targetLayer2->getLayerLoc();
      assert(mainLoc != NULL && loc != NULL);
      if (mainLoc->nxGlobal != loc->nxGlobal || mainLoc->nyGlobal != loc->nyGlobal
            || mainLoc->nf != loc->nf) {
         if (parent->columnId() == 0) {
            ErrorLog(errorMessage);
            errorMessage.printf(
                  "%s: targetLayer2 \"%s\" does not have the same dimensions as targetLayer.\n",
                  getDescription_c(),
                  targetName2);
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }

      targetLayer2->insertProbe(this);
      render_height += loc->ny;
	}


   if (targetName3 != NULL && strcmp(targetName3, "") != 0) {
      targetLayer3 = message->lookup<HyPerLayer>(std::string(targetName3));
      if (targetLayer3 == NULL) {
         if (parent->columnId() == 0) {
            ErrorLog().printf(
                  "%s: targetLayer2 \"%s\" is not a layer in the column.\n",
                  getDescription_c(),
                  targetName3);
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
		}

      const PVLayerLoc *mainLoc = targetLayer->getLayerLoc();
      const PVLayerLoc *loc    = targetLayer3->getLayerLoc();
      assert(mainLoc != NULL && loc != NULL);
      if (mainLoc->nxGlobal != loc->nxGlobal || mainLoc->nyGlobal != loc->nyGlobal
            || mainLoc->nf != loc->nf) {
         if (parent->columnId() == 0) {
            ErrorLog(errorMessage);
            errorMessage.printf(
                  "%s: targetLayer3 \"%s\" does not have the same dimensions as targetLayer.\n",
                  getDescription_c(),
                  targetName3);
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }

      targetLayer3->insertProbe(this);
      render_height += loc->ny;
   }

   // TODO: Assert that targetLayer has 3 features (RGB)

	printf("%d\n", render_height);

   SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
   renderer = SDL_SetVideoMode(render_width, render_height, 32, SDL_DOUBLEBUF);
   pixels = (uint32_t *) renderer->pixels;

   return Response::SUCCESS;
}

void VisualLayerProbe::visualizeLayer(HyPerLayer * layer, uint32_t *pixels, float min, float max) {
   PVLayerLoc const *loc = layer->getLayerLoc();
   int const nx          = loc->nx;
   int const ny          = loc->ny;
   int const nf          = loc->nf;
   PVHalo const *halo    = &loc->halo;
   int const lt          = halo->lt;
   int const rt          = halo->rt;
   int const dn          = halo->dn;
   int const up          = halo->up;
   float const *aBuffer = layer->getLayerData();

#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for
#endif
   for (int k = 0; k < getTargetLayer()->getNumNeurons(); k += 3) {
      int kex = kIndexExtended(k, nx, ny, nf, lt, rt, dn, up);
      uint8_t red   = rescale_color(aBuffer[kex],   min, max);
      uint8_t green = rescale_color(aBuffer[kex+1], min, max);
      uint8_t blue  = rescale_color(aBuffer[kex+2], min, max);
      uint32_t rgb = (red << 16) | (green << 8) | (blue);

      int x = kxPos(k, nx, ny, nf);
      int y = kyPos(k, nx, ny, nf);
      pixels[x + render_width*y] = rgb;
   }
}

void VisualLayerProbe::calcValues(double timevalue) {
   PVLayerLoc const *loc = targetLayer->getLayerLoc();
   int const nx          = loc->nx;
   int const ny          = loc->ny;
   int const nf          = loc->nf;
   PVHalo const *halo    = &loc->halo;
   int const lt          = halo->lt;
   int const rt          = halo->rt;
   int const dn          = halo->dn;
   int const up          = halo->up;

   float const *aBuffer = targetLayer->getLayerData();

   float max_a = aBuffer[0];
   float min_a = aBuffer[0];

   // Find max/min values to rescale float array into 24 bit RGB image
#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for reduction(max: max_a), reduction(min: min_a)
#endif
   for (int k = 0; k < targetLayer->getNumNeurons(); k++) {
      int kex = kIndexExtended(k, nx, ny, nf, lt, rt, dn, up);
      float a = aBuffer[kex];
      if (a > max_a) max_a = a;
      if (a < min_a) min_a = a;
   }


   uint32_t *canvas = pixels;
   visualizeLayer(targetLayer, canvas, min_a, max_a);
   if (targetLayer2 != NULL) {
      canvas += render_width*ny;
      visualizeLayer(targetLayer2, canvas, min_a, max_a);
   }
   if (targetLayer3 != NULL) {
      canvas += render_width*ny;
      visualizeLayer(targetLayer3, canvas, min_a, max_a);
   }
}

Response::Status VisualLayerProbe::outputState(double timevalue) {
   calcValues(timevalue);
   SDL_UpdateRect(renderer, 0, 0, render_width, render_height);
   return Response::SUCCESS;
}


} // end namespace PV
