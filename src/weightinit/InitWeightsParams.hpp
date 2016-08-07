/*
 * InitWeightsParams.hpp
 *
 *  Created on: Aug 10, 2011
 *      Author: kpeterson
 */

#ifndef INITWEIGHTSPARAMS_HPP_
#define INITWEIGHTSPARAMS_HPP_

#include "../include/pv_common.h"
#include "../include/pv_types.h"
#include "../io/PVParams.hpp"
#include "../layers/HyPerLayer.hpp"
#include "../connections/HyPerConn.hpp"
#include <stdlib.h>
#include <string.h>

namespace PV {
class HyPerConn;

class InitWeightsParams : public BaseObject {
public:
   InitWeightsParams();
   InitWeightsParams(char const * name, HyPerCol * hc);
   virtual ~InitWeightsParams();

   virtual int setDescription() override;
   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag) override;
   virtual int communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) override;

   //get-set methods:
   inline HyPerLayer * getPre()                 {return pre;}
   inline HyPerLayer * getPost()                {return post;}
   inline HyPerConn * getParentConn()           {return parentConn;}
   inline ChannelType getChannel()              {return channel;}
   inline const char * getFilename()            {return filename;}
   inline bool getUseListOfArborFiles()         {return useListOfArborFiles;}
   inline bool getCombineWeightFiles()          {return combineWeightFiles;}
   inline int getNumWeightFiles()               {return numWeightFiles;}

   virtual void calcOtherParams(int patchIndex);
   float calcYDelta(int jPost);
   float calcXDelta(int iPost);
   float calcDelta(int post, float dPost, float distHeadPreUnits);

   //get/set:
   int getnfPatch();
   int getnyPatch();
   int getnxPatch();
   int getPatchSize();
   int getsx();
   int getsy();
   int getsf();

   float getWMin();     // minimum allowed weight value
   float getWMax();     // maximum allowed weight value

protected:
   int initialize_base();
   int initialize(char const * name, HyPerCol * hc);

   char           * weightInitTypeString = nullptr;
   HyPerLayer     * pre;
   HyPerLayer     * post;
   HyPerConn      * parentConn;
   ChannelType channel;    // which channel of the post to update (e.g. inhibit)
   char * filename;
   bool useListOfArborFiles;
   bool combineWeightFiles;
   int numWeightFiles;

   void getcheckdimensionsandstrides();
   int kernelIndexCalculations(int patchIndex);

   /**
    * @brief weightInitType: Specifies the initialization method of weights
    * @details Possible choices are
    * - @link InitGauss2DWeightsParams Gauss2DWeight@endlink:
    *   Initializes weights with a gaussian distribution in x and y over each f
    *
    * - @link InitCocircWeightsParams CoCircWeight@endlink:
    *   Initializes cocircular weights
    *
    * - @link InitUniformWeightsParams UniformWeight@endlink:
    *   Initializes weights with a single uniform weight
    *
    * - @link InitSmartWeights SmartWeight@endlink:
    *   TODO
    *
    * - @link InitUniformRandomWeightsParams UniformRandomWeight@endlink:
    *   Initializes weights with a uniform distribution
    *
    * - @link InitGaussianRandomWeightsParams GaussianRandomWeight@endlink:
    *   Initializes individual weights with a gaussian distribution
    *
    * - @link InitIdentWeightsParams IdentWeight@endlink:
    *   Initializes weights for ident conn (one to one with a strength to 1)
    *
    * - @link InitOneToOneWeightsParams OneToOneWeight@endlink:
    *   Initializes weights as a multiple of the identity matrix
    *
    * - @link InitOneToOneWeightsWithDelaysParams OneToOneWeightsWithDelays@endlink:
    *   Initializes weights as a multiple of the identity matrix with delays
    *
    * - @link InitSpreadOverArborsWeightsParams SpreadOverArborsWeight@endlink:
    *   Initializes weights where different part of the weights over different arbors
    *
    * - @link InitWeightsParams FileWeight@endlink:
    *   Initializes weights from a specified pvp file.
    *
    * Further parameters are needed depending on initialization type
    */
   virtual void ioParam_weightInitType(enum ParamsIOFlag ioFlag);
   virtual void ioParam_initWeightsFile(enum ParamsIOFlag ioFlag);
   virtual void ioParam_useListOfArborFiles(enum ParamsIOFlag ioFlag);
   virtual void ioParam_combineWeightFiles(enum ParamsIOFlag ioFlag);
   virtual void ioParam_numWeightFiles(enum ParamsIOFlag ioFlag);
   //more get/set
   inline float getxDistHeadPreUnits()   {return xDistHeadPreUnits;}
   inline float getyDistHeadPreUnits()   {return yDistHeadPreUnits;}
   inline float getdyPost()              {return dyPost;}
   inline float getdxPost()              {return dxPost;}

   float dxPost;
   float dyPost;
   float xDistHeadPreUnits;
   float yDistHeadPreUnits;
};

} /* namespace PV */
#endif /* INITWEIGHTSPARAMS_HPP_ */
