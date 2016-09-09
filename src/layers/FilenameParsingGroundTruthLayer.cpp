/*
 * FilenameParsingGroundTruthLayer.cpp
 *
 *  Created on: Nov 10, 2014
 *      Author: wchavez
 */

#include "FilenameParsingGroundTruthLayer.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdlib>

namespace PV {
   FilenameParsingGroundTruthLayer::FilenameParsingGroundTruthLayer(const char *name, HyPerCol *hc) {
      initialize(name, hc);
   }

   FilenameParsingGroundTruthLayer::~FilenameParsingGroundTruthLayer()
   {
      free(mInputLayerName);
   }

   int FilenameParsingGroundTruthLayer::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
      int status = ANNLayer::ioParamsFillGroup(ioFlag);
      ioParam_classes(ioFlag);
      ioParam_inputLayerName(ioFlag);
      ioParam_gtClassTrueValue(ioFlag);
      ioParam_gtClassFalseValue(ioFlag);
      return status;
   }

   void FilenameParsingGroundTruthLayer::ioParam_gtClassTrueValue(enum ParamsIOFlag ioFlag) {
         ioParamValue(ioFlag, name, "gtClassTrueValue", &mGtClassTrueValue, 1.0f, false);
   }

   void FilenameParsingGroundTruthLayer::ioParam_gtClassFalseValue(enum ParamsIOFlag ioFlag) {
         parent->ioParamValue(ioFlag, name, "gtClassFalseValue", &mGtClassFalseValue, -1.0f, false);
   }

   void FilenameParsingGroundTruthLayer::ioParam_inputLayerName(enum ParamsIOFlag ioFlag) {
         ioParamStringRequired(ioFlag, name, "inputLayerName", &mInputLayerName);
   }

   void FilenameParsingGroundTruthLayer::ioParam_classes(enum ParamsIOFlag ioFlag) {
      std::string outPath = parent->getOutputPath();
      outPath = outPath + "/classes.txt";
      mInputFile.open(outPath.c_str(), std::ifstream::in);
      pvErrorIf(!mInputFile.is_open(), "%s: Unable to open file %s\n", getName(), outPath.c_str());

      mClasses.clear();
      std::string line;
      while(getline(mInputFile, line)) {
         mClasses.push_back(line);
      }
      mInputFile.close();
   }

   int FilenameParsingGroundTruthLayer::communicateInitInfo(std::shared_ptr<CommunicateInitInfoMessage const> message) {
      mInputLayer = message->mTable->lookup<InputLayer>(mInputLayerName);
      pvErrorIf(mInputLayer == nullptr && parent->columnId() == 0,
            "%s: movieLayerName \"%s\" is not a layer in the HyPerCol.\n",
            getDescription_c(), mInputLayerName);
      return PV_SUCCESS;
   }

   bool FilenameParsingGroundTruthLayer::needUpdate(double time, double dt){
      return mInputLayer->needUpdate(time, dt);
   }

   int FilenameParsingGroundTruthLayer::updateState(double time, double dt)
   {
      update_timer->start();
      pvdata_t * A = getCLayer()->activity->data;
      const PVLayerLoc * loc = getLayerLoc();
      int num_neurons = getNumNeurons();
      pvErrorIf(num_neurons != mClasses.size(),
         "The number of neurons in %s is not equal to the number of classes specified in %s/classes.txt\n",
         getName(), parent->getOutputPath());

      for (int b = 0; b < loc->nbatch; ++b) {
         char * currentFilename = nullptr;
         int filenameLen = 0;
         if (getCommunicator()->commRank() == 0) {
            currentFilename = strdup(mInputLayer->getFileName(b).c_str());
            int filenameLen = (int) strlen(currentFilename) + 1; // +1 for the null terminator
            MPI_Bcast(&filenameLen, 1, MPI_INT, 0, getCommunicator()->communicator());
            //Broadcast filename to all other local processes
            MPI_Bcast(currentFilename, filenameLen, MPI_CHAR, 0, getCommunicator()->communicator());
         }
         else {
            //Receive broadcast about length of filename
            MPI_Bcast(&filenameLen, 1, MPI_INT, 0, getCommunicator()->communicator());
            currentFilename = (char*)calloc(sizeof(char), filenameLen);
            //Receive filename
            MPI_Bcast(currentFilename, filenameLen, MPI_CHAR, 0, getCommunicator()->communicator());
         }

         std::string fil = currentFilename;
         pvdata_t * ABatch = A + b * getNumExtended();
         for(int i = 0; i < num_neurons; i++){
            int nExt = kIndexExtended(i, loc->nx, loc->ny, loc->nf, loc->halo.lt, loc->halo.rt, loc->halo.dn, loc->halo.up);
            int fi = featureIndex(nExt, loc->nx+loc->halo.rt+loc->halo.lt, loc->ny+loc->halo.dn+loc->halo.up, loc->nf);
            int match = fil.find(mClasses.at(i));
            if(0 <= match){
               ABatch[nExt] = mGtClassTrueValue;
            }
            else{
               ABatch[nExt] = mGtClassFalseValue;
            }
         }
         free(currentFilename);
      }
      update_timer->stop();
      return PV_SUCCESS;
   }
}
