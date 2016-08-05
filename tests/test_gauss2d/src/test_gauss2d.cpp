/**
 * This file tests weight initialization to a 2D Gaussian with sigma = 1.0 and normalized to 1.0
 * Test compares HyPerConn to KernelConn,
 * assumes kernelConn produces correct 2D Gaussian weights
 *
 */

#undef DEBUG_PRINT

#include "layers/HyPerLayer.hpp"
#include "connections/HyPerConn.hpp"
#include "io/io.hpp"
#include "utils/PVAssert.hpp"
#include <assert.h>

#include "Example.hpp"

using namespace PV;

int check_kernel_vs_hyper(HyPerConn * cHyPer, HyPerConn * cKernel, int kPre,
		int axonID);

template <typename T>
T * createObject(char const * name, HyPerCol * hc, ObserverTable& hierarchy) {
   T * newObject = new T(name, hc);
   hierarchy.addObject(name, newObject);
   return newObject;
}

int main(int argc, char * argv[])
{
   PV_Init* initObj = new PV_Init(&argc, &argv, false/*allowUnrecognizedArguments*/);
   if (initObj->getParamsFile()==NULL) {
      int rank = 0;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      if (rank==0) {
         pvErrorNoExit().printf("%s does not take a -p argument; the necessary param file is hardcoded.\n", argv[0]);
      }
      MPI_Barrier(MPI_COMM_WORLD);
      exit(EXIT_FAILURE);
   }

   initObj->setParams("input/test_gauss2d.params");
   const char * pre_layer_name = "test_gauss2d pre";
   const char * post_layer_name = "test_gauss2d post";
   const char * pre2_layer_name = "test_gauss2d pre 2";
   const char * post2_layer_name = "test_gauss2d post 2";

   PV::HyPerCol * hc = new PV::HyPerCol("test_gauss2d column", initObj);
   ObserverTable hierarchy;
   PV::Example * pre = createObject<PV::Example>(pre_layer_name, hc, hierarchy);
   PV::Example * post = createObject<PV::Example>(post_layer_name, hc, hierarchy);

   PV::HyPerConn * cHyPer = createObject<PV::HyPerConn>("test_gauss2d hyperconn", hc, hierarchy);
   PV::HyPerConn * cKernel = createObject<PV::HyPerConn>("test_gauss2d kernelconn", hc, hierarchy);

   PV::Example * pre2 = createObject<PV::Example>(pre2_layer_name, hc, hierarchy);
   PV::Example * post2 = createObject<PV::Example>(post2_layer_name, hc, hierarchy);

   PV::HyPerConn * cHyPer1to2 = createObject<PV::HyPerConn>("test_gauss2d hyperconn 1 to 2", hc, hierarchy);
   PV::HyPerConn * cKernel1to2 = createObject<PV::HyPerConn>("test_gauss2d kernelconn 1 to 2", hc, hierarchy);

   PV::HyPerConn * cHyPer2to1 = createObject<PV::HyPerConn>("test_gauss2d hyperconn 2 to 1", hc, hierarchy);
   PV::HyPerConn * cKernel2to1 = createObject<PV::HyPerConn>("test_gauss2d kernelconn 2 to 1", hc, hierarchy);
   
   int status = 0;

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
   pvErrorIf(num_pre_extended != cHyPer->getNumWeightPatches(), "Test failed.\n");

   for (int kPre = 0; kPre < num_pre_extended; kPre++) {
     status = check_kernel_vs_hyper(cHyPer, cKernel, kPre, axonID);
     pvErrorIf(status!=0, "Test failed.\n");
     status = check_kernel_vs_hyper(cHyPer1to2, cKernel1to2, kPre, axonID);
     pvErrorIf(status!=0, "Test failed.\n");
     status = check_kernel_vs_hyper(cHyPer2to1, cKernel2to1, kPre, axonID);
     pvErrorIf(status!=0, "Test failed.\n");
   }

   delete hc;
   delete initObj;
   return 0;
}

int check_kernel_vs_hyper(HyPerConn * cHyPer, HyPerConn * cKernel, int kPre, int axonID)
{
   assert(cKernel->usingSharedWeights()==true);
   assert(cHyPer->usingSharedWeights()==false);
   int status = 0;
   PVPatch * hyperPatch = cHyPer->getWeights(kPre, axonID);
   PVPatch * kernelPatch = cKernel->getWeights(kPre, axonID);
   int hyPerDataIndex = cHyPer->patchIndexToDataIndex(kPre);
   int kernelDataIndex = cKernel->patchIndexToDataIndex(kPre);

   int nk = cHyPer->fPatchSize() * (int) hyperPatch->nx; // hyperPatch->nf * hyperPatch->nx;
   assert(nk == (cKernel->fPatchSize() * (int) kernelPatch->nx));// assert(nk == (kernelPatch->nf * kernelPatch->nx));
   int ny = hyperPatch->ny;
   assert(ny == kernelPatch->ny);
   int sy = cHyPer->yPatchStride(); // hyperPatch->sy;
   assert(sy == cKernel->yPatchStride()); // assert(sy == kernelPatch->sy);
   pvwdata_t * hyperWeights = cHyPer->get_wData(axonID, hyPerDataIndex); // hyperPatch->data;
   pvwdata_t * kernelWeights = cKernel->get_wDataHead(axonID, kernelDataIndex)+hyperPatch->offset; // kernelPatch->data;
   float test_cond = 0.0f;
   for (int y = 0; y < ny; y++) {
      for (int k = 0; k < nk; k++) {
         test_cond = kernelWeights[k] - hyperWeights[k];
         if (fabs(test_cond) > 0.001f) {
            pvError(errorMessage);
            errorMessage.printf("y %d\n", y);
            errorMessage.printf("k %d\n", k);
            errorMessage.printf("kernelweight %f\n", kernelWeights[k]);
            errorMessage.printf("hyperWeights %f\n", hyperWeights[k]);
            const char * cHyper_filename = "gauss2d_hyper.txt";
            cHyPer->writeTextWeights(cHyper_filename, kPre);
            const char * cKernel_filename = "gauss2d_kernel.txt";
            cKernel->writeTextWeights(cKernel_filename, kPre);
            status=1;
         }
      }
      // advance pointers in y
      hyperWeights += sy;
      kernelWeights += sy;
   }
   return status;
}

