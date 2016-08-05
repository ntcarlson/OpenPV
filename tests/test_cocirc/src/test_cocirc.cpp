/**
 * This file tests weight initialization based on cocircularity
 * Test compares HyPerConn to CocircConn,
 * assumes CocircConn produces correct weights
 *
 */

#undef DEBUG_PRINT

#include "Example.hpp"
#include "layers/HyPerLayer.hpp"
#include "connections/HyPerConn.hpp"
#include "io/io.hpp"
#include <utils/PVLog.hpp>

using namespace PV;

template <typename T>
T * createObject(char const * name, HyPerCol * hc, ObserverTable& hierarchy) {
   T * newObject = new T(name, hc);
   hierarchy.addObject(name, newObject);
   return newObject;
}

// First argument to check_cocirc_vs_hyper should have sharedWeights = false
// Second argument should have sharedWeights = true
int check_cocirc_vs_hyper(HyPerConn * cHyPer, HyPerConn * cKernel, int kPre,
      int axonID);

int main(int argc, char * argv[])
{
   PV_Init * initObj = new PV_Init(&argc, &argv, false/*allowUnrecognizedArguments*/);
   PV::HyPerCol * hc = new PV::HyPerCol("test_cocirc column", initObj);
   ObserverTable hierarchy;
   
   const char * preLayerName = "test_cocirc pre";
   const char * postLayerName = "test_cocirc post";
   PV::Example * pre = createObject<PV::Example>(preLayerName, hc, hierarchy);
   PV::Example * post = createObject<PV::Example>(postLayerName, hc, hierarchy);
   PV::HyPerConn * cHyPer = createObject<PV::HyPerConn>("test_cocirc hyperconn", hc, hierarchy);
   PV::HyPerConn * cCocirc = createObject<PV::HyPerConn>("test_cocirc cocircconn", hc, hierarchy);
   
   PV::Example * pre2 = createObject<PV::Example>("test_cocirc pre 2", hc, hierarchy);
   PV::Example * post2 = createObject<PV::Example>("test_cocirc post 2", hc, hierarchy);
   PV::HyPerConn * cHyPer1to2 = createObject<PV::HyPerConn>("test_cocirc hyperconn 1 to 2", hc, hierarchy);
   PV::HyPerConn * cCocirc1to2 = createObject<PV::HyPerConn>("test_cocirc cocircconn 1 to 2", hc, hierarchy);
   PV::HyPerConn * cHyPer2to1 = createObject<PV::HyPerConn>("test_cocirc hyperconn 2 to 1", hc, hierarchy);
   PV::HyPerConn * cCocirc2to1 = createObject<PV::HyPerConn>("test_cocirc cocircconn 2 to 1", hc, hierarchy);

   ensureDirExists(hc->getCommunicator(), hc->getOutputPath());
   
   auto commMessagePtr = std::make_shared<CommunicateInitInfoMessage >(hierarchy);
   for (auto obj : hierarchy.getObjectVector()) {
      int status = obj->respond(commMessagePtr);
      pvErrorIf(status!=PV_SUCCESS, "Test failed.\n");
   }

   auto allocateMessagePtr = std::make_shared<AllocateDataMessage>();
   for (auto obj : hierarchy.getObjectVector()) {
      int status = obj->respond(allocateMessagePtr);
      pvErrorIf(status!=PV_SUCCESS, "Test failed.\n");
   }

   const int axonID = 0;
   int num_pre_extended = pre->clayer->numExtended;
   pvErrorIf(!(num_pre_extended == cHyPer->getNumWeightPatches()), "Test failed.\n");

   int status = 0;
   for (int kPre = 0; kPre < num_pre_extended; kPre++) {
      status = check_cocirc_vs_hyper(cHyPer, cCocirc, kPre, axonID);
      pvErrorIf(!(status==0), "Test failed.\n");
      status = check_cocirc_vs_hyper(cHyPer1to2, cCocirc1to2, kPre, axonID);
      pvErrorIf(!(status==0), "Test failed.\n");
      status = check_cocirc_vs_hyper(cHyPer2to1, cCocirc2to1, kPre, axonID);
      pvErrorIf(!(status==0), "Test failed.\n");
   }

   delete hc;
   delete initObj;
   return 0;
}

int check_cocirc_vs_hyper(HyPerConn * cHyPer, HyPerConn * cKernel, int kPre, int axonID)
{
   pvErrorIf(!(cKernel->usingSharedWeights()==true), "Test failed.\n");
   pvErrorIf(!(cHyPer->usingSharedWeights()==false), "Test failed.\n");
   int status = 0;
   PVPatch * hyperPatch = cHyPer->getWeights(kPre, axonID);
   PVPatch * cocircPatch = cKernel->getWeights(kPre, axonID);
   int hyPerDataIndex = cHyPer->patchIndexToDataIndex(kPre);
   int kernelDataIndex = cKernel->patchIndexToDataIndex(kPre);
   
   int nk = cHyPer->fPatchSize() * (int) hyperPatch->nx;
   pvErrorIf(!(nk == (cKernel->fPatchSize() * (int) cocircPatch->nx)), "Test failed.\n");
   int ny = hyperPatch->ny;
   pvErrorIf(!(ny == cocircPatch->ny), "Test failed.\n");
   int sy = cHyPer->yPatchStride();
   pvErrorIf(!(sy == cKernel->yPatchStride()), "Test failed.\n");
   pvwdata_t * hyperWeights = cHyPer->get_wData(axonID, hyPerDataIndex);
   pvwdata_t * cocircWeights = cKernel->get_wDataHead(axonID, kernelDataIndex)+hyperPatch->offset;
   float test_cond = 0.0f;
   for (int y = 0; y < ny; y++) {
      for (int k = 0; k < nk; k++) {
         test_cond = cocircWeights[k] - hyperWeights[k];
         if (fabs(test_cond) > 0.001f) {
            const char * cHyper_filename = "cocirc_hyper.txt";
            cHyPer->writeTextWeights(cHyper_filename, kPre);
            const char * cKernel_filename = "cocirc_cocirc.txt";
            cKernel->writeTextWeights(cKernel_filename, kPre);
         }
         pvErrorIf(!(fabs(test_cond) <= 0.001f), "Test failed.\n");
      }
      // advance pointers in y
      hyperWeights += sy;
      cocircWeights += sy;
   }
   return status;
}

