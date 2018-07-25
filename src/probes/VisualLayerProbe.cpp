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
}

int VisualLayerProbe::initialize(const char *name, HyPerCol *hc) {
   return LayerProbe::initialize(name, hc);
}

int VisualLayerProbe::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = LayerProbe::ioParamsFillGroup(ioFlag);
   ioParam_extraTargets(ioFlag);
   return status;
}

void VisualLayerProbe::ioParam_extraTargets(enum ParamsIOFlag ioFlag) {
	// Multiple layer names are given in a single string separated by spaces
	// TODO: Implement an ioParamStringArray function to do this properly
   parent->parameters()->ioParamString(ioFlag, name, "extraTargets", &namesString, NULL);
	
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

	char *tmp;
	char *name = strtok_r(namesString, " ", &tmp);
	while (name != NULL) {
		extraNames.push_back(name);
		name = strtok_r(NULL, " ", &tmp);
	}

	for (char * name : extraNames) {
      auto layer = message->lookup<HyPerLayer>(std::string(name));
      if (layer == NULL) {
         if (parent->columnId() == 0) {
            ErrorLog().printf(
                  "%s: layer \"%s\" is not a layer in the column.\n",
                  getDescription_c(),
                  name);
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }


      const PVLayerLoc *mainLoc = getTargetLayer()->getLayerLoc();
      const PVLayerLoc *loc    = layer->getLayerLoc();
      assert(mainLoc != NULL && loc != NULL);
      if (mainLoc->nxGlobal != loc->nxGlobal || mainLoc->nyGlobal != loc->nyGlobal
            || mainLoc->nf != loc->nf) {
         if (parent->columnId() == 0) {
            ErrorLog(errorMessage);
            errorMessage.printf(
                  "%s: Extra layer \"%s\" does not have the same dimensions as targetLayer.\n",
                  getDescription_c(),
                  name);
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }

		extraTargets.push_back(layer);
      layer->insertProbe(this);
      render_height += loc->ny;
	}

   // TODO: Assert that targetLayer has 3 features (RGB)


   SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
   renderer = SDL_SetVideoMode(render_width, render_height, 32, SDL_DOUBLEBUF);
   pixels = (uint32_t *) renderer->pixels;

   return Response::SUCCESS;
}

void VisualLayerProbe::visualizeLayer(HyPerLayer * layer, uint32_t *pixels) {
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


   float max_a = aBuffer[0];
   float min_a = aBuffer[0];

   // Find max/min values to rescale float array into 24 bit RGB image
#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for reduction(max: max_a), reduction(min: min_a)
#endif
   for (int k = 0; k < layer->getNumNeurons(); k++) {
      int kex = kIndexExtended(k, nx, ny, nf, lt, rt, dn, up);
      float a = aBuffer[kex];
      if (a > max_a) max_a = a;
      if (a < min_a) min_a = a;
   }

#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for
#endif
   for (int k = 0; k < getTargetLayer()->getNumNeurons(); k += 3) {
      int kex = kIndexExtended(k, nx, ny, nf, lt, rt, dn, up);
      uint8_t red   = rescale_color(aBuffer[kex],   min_a, max_a);
      uint8_t green = rescale_color(aBuffer[kex+1], min_a, max_a);
      uint8_t blue  = rescale_color(aBuffer[kex+2], min_a, max_a);
      uint32_t rgb = (red << 16) | (green << 8) | (blue);

      int x = kxPos(k, nx, ny, nf);
      int y = kyPos(k, nx, ny, nf);
      pixels[x + render_width*y] = rgb;
   }
}

void VisualLayerProbe::calcValues(double timevalue) {
   PVLayerLoc const *loc = getTargetLayer()->getLayerLoc();
   int const nx          = loc->nx;
   int const ny          = loc->ny;
   int const nf          = loc->nf;
   PVHalo const *halo    = &loc->halo;
   int const lt          = halo->lt;
   int const rt          = halo->rt;
   int const dn          = halo->dn;
   int const up          = halo->up;

   uint32_t *canvas = pixels;
   visualizeLayer(getTargetLayer(), canvas);

	for (auto layer : extraTargets) {
		canvas += render_width * ny;
		visualizeLayer(layer, canvas);
	}
}

Response::Status VisualLayerProbe::outputState(double timevalue) {
   calcValues(timevalue);
   SDL_UpdateRect(renderer, 0, 0, render_width, render_height);
   return Response::SUCCESS;
}


} // end namespace PV
