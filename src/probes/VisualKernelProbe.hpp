/*
 * VisualKernelPatchProbe.hpp
 *
 *  Created on: Jun 25, 2018
 *      Author: ncarlson
 */

#ifndef VISUALKERNELPROBE_HPP_
#define VISUALKERNELPROBE_HPP_

#include "BaseHyPerConnProbe.hpp"
#include <SDL/SDL.h>

namespace PV {

class VisualKernelProbe : public BaseHyPerConnProbe {

   // Methods
  public:
   VisualKernelProbe(const char *probename, HyPerCol *hc);
   virtual ~VisualKernelProbe();
   virtual Response::Status
   communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) override;
   virtual Response::Status allocateDataStructures() override;
   virtual Response::Status outputState(double timef) override;

  protected:
   VisualKernelProbe(); // Default constructor, can only be called by derived classes
   int initialize(const char *probename, HyPerCol *hc);
   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag) override;
   virtual void ioParam_arborId(enum ParamsIOFlag ioFlag);
   virtual void ioParam_featureScale(enum ParamsIOFlag ioFlag);
   virtual void ioParam_numFeatureX(enum ParamsIOFlag ioFlag);
   virtual void ioParam_numFeatureY(enum ParamsIOFlag ioFlag);

   virtual void initNumValues() override;

   virtual void calcValues(double timevalue) override {
      Fatal().printf("%s does not use calcValues.\n", getDescription_c());
   };

   int getArbor() { return arborID; }

  private:
   int initialize_base();
   int render_width;
   int render_height;
   int numFeatureX;
   int numFeatureY;
   int featureScale;
   SDL_Surface * renderer;
   uint32_t * pixels;
   int arborID; // which arbor to investigate

}; // end of class VisualKernelProbe block

} // end of namespace PV block

#endif /* VISUALKERNELPROBE_HPP_ */
