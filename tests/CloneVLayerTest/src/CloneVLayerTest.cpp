/*
 * pv.cpp
 *
 */


#include <columns/buildandrun.hpp>

int customexit(HyPerCol * hc, int argc, char * argv[]);

int main(int argc, char * argv[]) {
   PV_Init initObj(&argc, &argv, false/*allowUnrecognizedArguments*/);
   if (initObj.getParamsFile() == NULL) {
      initObj.setParams("input/CloneVLayerTest.params");
   }

   int status;
   status = rebuildandrun(&initObj, NULL, &customexit);
   return status==PV_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

int customexit(HyPerCol * hc, int argc, char * argv[]) {
   int N;

   HyPerLayer * check_clone_layer = hc->getLayerFromName("CheckClone");
   pvErrorIf(!(check_clone_layer!=NULL), "No layer named CheckClone.\n");
   const pvdata_t * check_clone_layer_data = check_clone_layer->getLayerData();
   N = check_clone_layer->getNumExtended();

   for (int k=0; k<N; k++) {
      pvErrorIf(!(fabsf(check_clone_layer_data[k])<1e-6), "Test failed.\n");
   }

   HyPerLayer * check_sigmoid_layer = hc->getLayerFromName("CheckSigmoid");
   pvErrorIf(!(check_sigmoid_layer!=NULL), "No layer named CheckSigmoid.\n");
   const pvdata_t * check_sigmoid_layer_data = check_sigmoid_layer->getLayerData();
   N = check_sigmoid_layer->getNumExtended();

   for (int k=0; k<N; k++) {
      pvErrorIf(!(fabsf(check_sigmoid_layer_data[k])<1e-6), "Test failed.\n");
   }

   if (hc->columnId()==0) {
      pvInfo().printf("%s passed.\n", argv[0]);
   }
   return PV_SUCCESS;
}
