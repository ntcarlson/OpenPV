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

  private:
   int initialize_base();

  private:
   int scale;
   int render_width;
   int render_height;
   SDL_Surface * renderer;
   uint32_t * pixels;
}; // end class VisualLayerProbe

} // end namespace PV

#endif /* VISUALLAYERPROBE_HPP_ */
