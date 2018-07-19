/*
 * AbstractNormProbe.hpp
 *
 *  Created on: Aug 11, 2015
 *      Author: pschultz
 */

#ifndef VISUALLAYERPROBE_HPP_
#define VISUALLAYERPROBE_HPP_

#include "LayerProbe.hpp"
#include <SDL/SDL.h>

namespace PV {

class VisualLayerProbe : public LayerProbe {
  public:
   VisualLayerProbe(const char *name, HyPerCol *hc);
   virtual ~VisualLayerProbe();

  protected:
   VisualLayerProbe();
   int initialize(const char *name, HyPerCol *hc);

   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag) override;
   void ioParam_scale(enum ParamsIOFlag ioFlag);
   void ioParam_targetName2(enum ParamsIOFlag ioFlag);
   void ioParam_targetName3(enum ParamsIOFlag ioFlag);

   virtual void calcValues(double timevalue) override;

   /**
    * Calls LayerProbe::communicateInitInfo to set up the targetLayer and
    * attach the probe; and then checks the masking layer if masking is used.
    */
   virtual Response::Status
   communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) override;

   /**
    * Implements the outputState method required by classes derived from
    * BaseProbe.
    * Prints to the outputFile the probe message, timestamp, number of neurons,
    * and norm value for
    * each batch element.
    */
   virtual Response::Status outputState(double timevalue) override;

   uint8_t rescale_color(float val, float min, float max) {
      if (val < min) return 0x00;
      if (val > max) return 0xFF;
		return (uint8_t) (((val  - min) * 255.0)/(max - min));
   }

   void visualizeLayer(HyPerLayer * layer, uint32_t *pixels, float min, float max);


  private:
   int initialize_base();

  private:
   int scale;
   int render_width;
   int render_height;
   SDL_Surface * renderer;
   uint32_t * pixels;

   char * targetName2;
   char * targetName3;

	HyPerLayer * targetLayer2;
	HyPerLayer * targetLayer3;
}; // end class VisualLayerProbe

} // end namespace PV

#endif /* VISUALLAYERPROBE_HPP_ */
