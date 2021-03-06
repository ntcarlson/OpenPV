/*
 * MPIBlockTest.cpp
 *
 */

#include "structures/MPIBlock.hpp"
#include "columns/Arguments.hpp"
#include "columns/Communicator.hpp"
#include "utils/PVLog.hpp"
#include "utils/conversions.h"
#include <sstream>
#include <string>

void runNoBatching();
void runWithBatching();
void run(std::string configString);

int main(int argc, char *argv[]) {
   MPI_Init(&argc, &argv);
   runNoBatching();
   runWithBatching();
   MPI_Finalize();
   return 0;
}

void runNoBatching() {
   std::string configString;
   configString.append("NumRows:4\n");
   configString.append("NumColumns:6\n");
   configString.append("BatchWidth:1\n");
   configString.append("CheckpointCellNumRows:2\n");
   configString.append("CheckpointCellNumColumns:2\n");
   configString.append("CheckpointCellBatchDimension:1\n");
   configString.append("ParamsFile:input/CheckpointCellTest.params\n");
   configString.append("LogFile:CheckpointCellTest.log\n");
   run(configString);
}

void runWithBatching() {
   std::string configString;
   configString.append("NumRows:1\n");
   configString.append("NumColumns:6\n");
   configString.append("BatchWidth:4\n");
   configString.append("CheckpointCellNumRows:1\n");
   configString.append("CheckpointCellNumColumns:2\n");
   configString.append("CheckpointCellBatchDimension:2\n");
   configString.append("ParamsFile:input/CheckpointCellTest.params\n");
   configString.append("LogFile:CheckpointCellTest.log\n");
   run(configString);
}

void run(std::string configString) {
   std::istringstream configStream{configString};
   PV::Arguments arguments{configStream, false /* do not allow unrecognized arguments */};
   PV::Communicator pvCommunicator{&arguments};

   int const numRows            = arguments.getIntegerArgument("NumRows");
   int const numColumns         = arguments.getIntegerArgument("NumColumns");
   int const batchWidth         = arguments.getIntegerArgument("BatchWidth");
   int const numNeededProcesses = numRows * numColumns * batchWidth;

   int const cellNumRows        = arguments.getIntegerArgument("CheckpointCellNumRows");
   int const cellNumColumns     = arguments.getIntegerArgument("CheckpointCellNumColumns");
   int const cellBatchDimension = arguments.getIntegerArgument("CheckpointCellBatchDimension");

   PV::MPIBlock mpiBlock{pvCommunicator.globalCommunicator(),
                         pvCommunicator.numCommRows(),
                         pvCommunicator.numCommColumns(),
                         pvCommunicator.numCommBatches(),
                         cellNumRows,
                         cellNumColumns,
                         cellBatchDimension};

   int globalRank;
   MPI_Comm_rank(pvCommunicator.globalCommunicator(), &globalRank);

   int const globalRowIndex      = pvCommunicator.commRow();
   int const globalColumnIndex   = pvCommunicator.commColumn();
   int const globalBatchIndex    = pvCommunicator.commBatch();
   int const blockNumRows        = cellNumRows > 0 ? cellNumRows : 1;
   int const blockNumColumns     = cellNumColumns > 0 ? cellNumColumns : 1;
   int const blockBatchDimension = cellBatchDimension > 0 ? cellBatchDimension : 1;

   // Verify MPIBlock rank
   int const blockRowIndex    = globalRowIndex % blockNumRows;
   int const blockColumnIndex = globalColumnIndex % blockNumColumns;
   int const blockBatchIndex  = globalBatchIndex % blockBatchDimension;
   int blockRank =
         (blockBatchIndex * blockNumRows + blockRowIndex) * blockNumColumns + blockColumnIndex;
   FatalIf(mpiBlock.getRank() != blockRank, "Rank incorrect on global process %d\n", globalRank);

   // Verify SizeRow, SizeColumn, SizeBatch
   FatalIf(
         mpiBlock.getNumRows() != blockNumRows,
         "SizeRow incorrect on global process %d\n",
         globalRank);
   FatalIf(
         mpiBlock.getNumColumns() != blockNumColumns,
         "SizeColumn incorrect on global process %d\n",
         globalRank);
   FatalIf(
         mpiBlock.getBatchDimension() != blockBatchDimension,
         "SizeBatch incorrect on global process %d\n",
         globalRank);

   // Verify RowIndex, ColumnIndex, BatchIndex
   FatalIf(
         mpiBlock.getRowIndex() != blockRowIndex,
         "RowIndex incorrect on global process %d\n",
         globalRank);
   FatalIf(
         mpiBlock.getColumnIndex() != blockColumnIndex,
         "ColumnIndex incorrect on global process %d\n",
         globalRank);
   FatalIf(
         mpiBlock.getBatchIndex() != blockBatchIndex,
         "BatchIndex incorrect on global process %d\n",
         globalRank);

   // Verify StartRow, StartColumn, StartBatch
   FatalIf(
         mpiBlock.getStartRow() != globalRowIndex - blockRowIndex,
         "StartRow incorrect on global process %d\n",
         globalRank);
   FatalIf(
         mpiBlock.getStartColumn() != globalColumnIndex - blockColumnIndex,
         "StartColumn incorrect on global process %d\n",
         globalRank);
   FatalIf(
         mpiBlock.getStartBatch() != globalBatchIndex - blockBatchIndex,
         "StartBatch incorrect on global process %d (%d versus %d)\n",
         globalRank,
         mpiBlock.getStartBatch(),
         globalBatchIndex - blockBatchIndex);
}
