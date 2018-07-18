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
   ioParam_scale(ioFlag);
   return status;
}

void VisualLayerProbe::ioParam_scale(enum ParamsIOFlag ioFlag) {
   parent->parameters()->ioParamValue(
         ioFlag, name, "scale", &scale, 1 /*default*/);
}


Response::Status
VisualLayerProbe::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
   auto status = LayerProbe::communicateInitInfo(message);
   if (!Response::completed(status)) {
      return status;
   }
   assert(targetLayer);

   // TODO: Assert that targetLayer has 3 features (RGB)

   render_width  = getTargetLayer()->getLayerLoc()->nx;
   render_height = getTargetLayer()->getLayerLoc()->ny;
   SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
   renderer = SDL_SetVideoMode(render_width, render_height, 32, SDL_DOUBLEBUF);
   pixels = (uint32_t *) renderer->pixels;

   return Response::SUCCESS;
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

   float const *aBuffer = getTargetLayer()->getLayerData();

   float max_a = aBuffer[0];
   float min_a = aBuffer[0];

#ifdef PV_USE_OPENMP_THREADS
#pragma omp parallel for reduction(max: max_a), reduction(min: min_a)
#endif
   for (int k = 0; k < getTargetLayer()->getNumNeurons(); k++) {
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
      uint8_t red   = ((aBuffer[kex]   - min_a) * 255.0)/(max_a - min_a);
      uint8_t green = ((aBuffer[kex+1] - min_a) * 255.0)/(max_a - min_a);
      uint8_t blue  = ((aBuffer[kex+2] - min_a) * 255.0)/(max_a - min_a);
      uint32_t rgb = (red << 16) | (green << 8) | (blue);

      int x = kxPos(k, nx, ny, nf);
      int y = kyPos(k, nx, ny, nf);
      pixels[x + render_width*y] = rgb;
   }

}

Response::Status VisualLayerProbe::outputState(double timevalue) {
   calcValues(timevalue);
   SDL_UpdateRect(renderer, 0, 0, render_width, render_height);
   return Response::SUCCESS;
}


} // end namespace PV
