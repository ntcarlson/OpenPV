/*
 * pv.cpp
 *
 */
#include "HyPerConnCheckpointerTestProbe.hpp"
#include <columns/PV_Init.hpp>
#include <columns/buildandrun.hpp>

void checkConfiguration(PV::PV_Init &pv_initObj, char const *programName);
int checkProbe(PV::HyPerCol *hc, int argc, char **argv);

int main(int argc, char *argv[]) {
   int status = PV_SUCCESS;
   PV::PV_Init pv_initObj(&argc, &argv, false /*do not allow unrecognized arguments*/);
   char const *programName = argv[0];
   checkConfiguration(pv_initObj, programName);
   pv_initObj.registerKeyword(
         "HyPerConnCheckpointerTestProbe", Factory::create<HyPerConnCheckpointerTestProbe>);

   pv_initObj.setParams("input/HyPerConnCheckpointerTest_freshstart_triggering.params");
   status = buildandrun(&pv_initObj, nullptr, checkProbe);
   FatalIf(
         status != PV_SUCCESS,
         "%s failed on triggering connection, with run started from scratch.\n",
         programName);

   pv_initObj.setStringArgument(
         "CheckpointReadDirectory", "output_freshstart_triggering/checkpoints/Checkpoint08");
   status = buildandrun(&pv_initObj, nullptr, checkProbe);
   FatalIf(
         status != PV_SUCCESS,
         "%s failed on triggering connection, with run started from Checkpoint08.\n",
         programName);

   pv_initObj.resetState();

   pv_initObj.setParams("input/HyPerConnCheckpointerTest_initfromCP_triggering.params");
   status = buildandrun(&pv_initObj, nullptr, checkProbe);
   FatalIf(
         status != PV_SUCCESS,
         "%s failed on triggering connection, with run initialized from Checkpoint08.\n",
         programName);

   pv_initObj.resetState();

   pv_initObj.setParams("input/HyPerConnCheckpointerTest_freshstart_updateperiod.params");
   status = buildandrun(&pv_initObj, nullptr, checkProbe);
   FatalIf(
         status != PV_SUCCESS,
         "%s failed on update period connection, with run started from scratch.\n",
         programName);

   pv_initObj.setStringArgument(
         "CheckpointReadDirectory", "output_freshstart_updateperiod/checkpoints/Checkpoint08");
   status = buildandrun(&pv_initObj, nullptr, checkProbe);
   FatalIf(
         status != PV_SUCCESS,
         "%s failed on update period connection, with run started from Checkpoint08.\n",
         programName);

   pv_initObj.resetState();

   pv_initObj.setParams("input/HyPerConnCheckpointerTest_initfromCP_updateperiod.params");
   status = buildandrun(&pv_initObj, nullptr, checkProbe);
   FatalIf(
         status != PV_SUCCESS,
         "%s failed on update period connection, with run initialized from Checkpoint08.\n",
         programName);

   return PV_SUCCESS;
}

void checkConfiguration(PV::PV_Init &pv_initObj, char const *programName) {
   int status = PV_SUCCESS;
   int rank   = pv_initObj.getCommunicator()->globalCommRank();
   if (pv_initObj.getParams() != nullptr) {
      if (rank == 0) {
         ErrorLog().printf("%s should be run without the params file argument.\n", programName);
      }
      status = PV_FAILURE;
   }
   if (pv_initObj.getBooleanArgument("Restart")) {
      if (rank == 0) {
         ErrorLog().printf("%s should be run without the restart flag.\n", programName);
      }
      status = PV_FAILURE;
   }
   if (!pv_initObj.getStringArgument("CheckpointReadDirectory").empty()) {
      if (rank == 0) {
         ErrorLog().printf(
               "%s should be run without the checkpoint read directory.\n", programName);
      }
      status = PV_FAILURE;
   }
   if (status != PV_SUCCESS) {
      if (rank == 0) {
         ErrorLog().printf(
               "This test uses hard-coded params files and includes running from checkpoint; "
               "therefore it needs to be run without params file or checkpoint arguments "
               "on the command line.\n");
      }
      MPI_Barrier(pv_initObj.getCommunicator()->globalCommunicator());
      exit(EXIT_FAILURE);
   }
}

int checkProbe(PV::HyPerCol *hc, int argc, char **argv) {
   auto probe = dynamic_cast<HyPerConnCheckpointerTestProbe *>(hc->getColProbeFromName("probe"));
   FatalIf(
         probe == nullptr,
         "Column does not have a HyPerConnCheckpointerTestProbe named \"probe\".\n");
   return probe->getTestFailed() ? PV_FAILURE : PV_SUCCESS;
}
