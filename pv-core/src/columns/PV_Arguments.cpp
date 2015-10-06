/*
 * PVArguments.cpp
 *
 *  Created on: Sep 21, 2015
 *      Author: pschultz
 */

#include <cstdlib>
#include <omp.h>
#include "PV_Arguments.hpp"
#include "../io/io.h"

namespace PV {

PV_Arguments::PV_Arguments(int argc, char * argv[], bool allowUnrecognizedArguments) {
   initialize_base();
   initialize(argc, argv, allowUnrecognizedArguments);
}

int PV_Arguments::initialize_base() {
   initializeState();
   numArgs = 0;
   args = NULL;
   return PV_SUCCESS;
}

int PV_Arguments::initializeState() {
   requireReturnFlag = false;
   outputPath = NULL;
   paramsFile = NULL;
   logFile = NULL;
   gpuDevices = NULL;
   randomSeed = 0U;
   workingDir = NULL;
   restartFlag = false;
   checkpointReadDir = NULL;
   numThreads = 0;
   numRows = 0;
   numColumns = 0;
   batchWidth = 0;
   return PV_SUCCESS;
}

int PV_Arguments::initialize(int argc, char * argv[], bool allowUnrecognizedArguments) {
   args = (char **) malloc((size_t) (argc+1) * sizeof(char *));
   numArgs = argc;
   args = copyArgs(argc, argv);
   return setStateFromCmdLineArgs(allowUnrecognizedArguments);
}

char ** PV_Arguments::copyArgs(int argc, char const * const * argv) {
   char ** argumentArray = (char **) malloc((size_t) (argc+1) * sizeof(char *));
   if (argumentArray==NULL) {
      fprintf(stderr, "PV_Arguments error: unable to allocate memory for %d arguments: %s\n",
            argc, strerror(errno));
      exit(EXIT_FAILURE);
   }
   for (int a=0; a<argc; a++) {
      char const * arga = argv[a];
      if (arga) {
         char * copied = strdup(arga);
         if (!copied) {
            fprintf(stderr, "PV_Arguments unable to store argument %d: %s\n", a, strerror(errno));
            fprintf(stderr, "Argument was \"%s\".\n", arga);
            exit(EXIT_FAILURE);
         }
         argumentArray[a] = copied;
      }
      else {
         argumentArray[a] = NULL;
      }
   }
   argumentArray[argc] = NULL;
   return argumentArray;
}

void PV_Arguments::freeArgs(int argc, char ** argv) {
   for (int k=0; k<argc; k++) { free(argv[k]); }
   free(argv);
   return;
}

char ** PV_Arguments::getArgsCopy() {
   return copyArgs(numArgs, args);
}

bool PV_Arguments::setRequireReturnFlag(bool val) {
   requireReturnFlag = val;
   return requireReturnFlag;
}
char const * PV_Arguments::setOutputPath(char const * val) {
   return setString(&outputPath, val, "output path");
}
char const * PV_Arguments::setParamsFile(char const * val) {
   return setString(&paramsFile, val, "params file");
}
char const * PV_Arguments::setLogFile(char const * val) {
   return setString(&logFile, val, "log file");
}
char const * PV_Arguments::setGPUDevices(char const * val) {
   return setString(&gpuDevices, val, "GPU devices string");
}
unsigned int PV_Arguments::setRandomSeed(unsigned int val) {
   randomSeed = val;
   return randomSeed;
}
char const * PV_Arguments::setWorkingDir(char const * val) {
   return setString(&workingDir, val, "working directory");
}
bool PV_Arguments::setRestartFlag(bool val) {
   requireReturnFlag = val;
   return requireReturnFlag;
}
char const * PV_Arguments::setCheckpointReadDir(char const * val) {
   return setString(&checkpointReadDir, val, "checkpointRead directory");
}
int PV_Arguments::setNumThreads(int val) {
   numThreads = val;
   return numThreads;
}
int PV_Arguments::setNumRows(int val) {
   numRows = val;
   return numRows;
}
int PV_Arguments::setNumColumns(int val) {
   numColumns = val;
   return numColumns;
}
int PV_Arguments::setBatchWidth(int val) {
   batchWidth = val;
   return batchWidth;
}

char const * PV_Arguments::setString(char ** parameter, char const * string, char const * parameterName) {
   int status = PV_SUCCESS;
   char * newParameter = NULL;
   if (string!=NULL) {
      newParameter = strdup(string);
      if (newParameter==NULL) {
         errorSettingString(parameterName, string);
         status = PV_FAILURE;
      }
   }
   if (status==PV_SUCCESS) {
      free(*parameter);
      *parameter = newParameter;
   }
   return newParameter;
}

int PV_Arguments::errorSettingString(char const * parameterName, char const * value) {
   fprintf(stderr, "PV_Arguments error setting %s to \"%s\": %s\n",
         parameterName, value, strerror(errno));
   exit(EXIT_FAILURE);
}

int PV_Arguments::resetState(int argc, char * argv[], bool allowUnrecognizedArguments) {
   int status = clearState();
   assert(status == PV_SUCCESS);
   freeArgs(numArgs, args); args = NULL;
   return initialize(argc, argv, allowUnrecognizedArguments);
}

int PV_Arguments::resetState() {
   int status = clearState();
   assert(status == PV_SUCCESS);
   return setStateFromCmdLineArgs(true);
   /* If unrecognized arguments were not allowed in the constructor and there were unrecognized args in argv,
    * the error would have taken place during the constructor. */
}

int PV_Arguments::clearState() {
   requireReturnFlag = false;
   free(outputPath); outputPath = NULL;
   free(paramsFile); paramsFile = NULL;
   free(logFile); logFile = NULL;
   free(gpuDevices); gpuDevices = NULL;
   randomSeed = 0U;
   free(workingDir); workingDir = NULL;
   restartFlag = false;
   free(checkpointReadDir); checkpointReadDir = NULL;
   numThreads = 0;
   numRows = 0;
   numColumns = 0;
   batchWidth = 0;
   return PV_SUCCESS;
}

int PV_Arguments::setStateFromCmdLineArgs(bool allowUnrecognizedArguments) {
   bool * usedArgArray = (bool *) calloc((size_t) numArgs, sizeof(bool)); // Because, C++ expert, I'm *trying* to cause you pain.
   if (usedArgArray==NULL) {
      fprintf(stderr, "PV_Arguments::setStateFromCmdLineArgs unable to allocate memory for usedArgArray: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   int restart = (int) restartFlag;
   int status = parse_options(numArgs, args,
         usedArgArray, &requireReturnFlag, &outputPath, &paramsFile, &logFile,
         &gpuDevices, &randomSeed, &workingDir,
         &restart, &checkpointReadDir, &numThreads,
         &numRows, &numColumns, &batchWidth);
   restartFlag = restart!=0;

#ifdef PV_USE_OPENMP_THREADS
   // If "-t" appeared as the last argument, set threads to the max possible.
   if (numThreads==0 && pv_getopt(numArgs, args, "-t", usedArgArray)==0) {
      numThreads = omp_get_max_threads();
   }
#else // PV_USE_OPENMP_THREADS
   numThreads = 1;
#endif // PV_USE_OPENMP_THREADS

   // Error out if both -r and -c are used.
   if (errorChecking()) {
      exit(EXIT_FAILURE);
   }

   if (!allowUnrecognizedArguments) {
      bool anyUnusedArgs = false;
      for (int a=0; a<numArgs; a++) {
         if(!usedArgArray[a]) {
            fprintf(stderr, "%s: argument %d, \"%s\", is not recognized.\n",
                  getProgramName(), a, args[a]);
            anyUnusedArgs = true;
         }
      }
      if (anyUnusedArgs) {
         exit(EXIT_FAILURE);
      }
   }
   free(usedArgArray);
   return PV_SUCCESS;
}

int PV_Arguments::printState() {
   printf("%s", getProgramName());
   if (requireReturnFlag) { printf(" --require-return"); }
   if (outputPath) { printf(" -o %s", outputPath); }
   if (paramsFile) { printf(" -p %s", paramsFile); }
   if (logFile) { printf(" -l %s", logFile); }
   if (gpuDevices) { printf(" -d %s", gpuDevices); }
   if (randomSeed) { printf(" -s %u", randomSeed); }
   if (workingDir) { printf(" -w %s", workingDir); }
   assert(!(restartFlag && checkpointReadDir));
   if (restartFlag) { printf(" -r"); }
   if (checkpointReadDir) { printf(" -c %s", checkpointReadDir); }
   if (numThreads) { printf(" -t %d", numThreads); }
   if (numRows) { printf(" -rows %d", numRows); }
   if (numColumns) { printf(" -columns %d", numColumns); }
   if (batchWidth) { printf(" -batchwidth %d", batchWidth); }
   printf("\n");
   return PV_SUCCESS;
}

int PV_Arguments::errorChecking() {
   int status = PV_SUCCESS;
   if (restartFlag && checkpointReadDir) {
      fprintf(stderr, "PV_Arguments error: cannot set both the restart flag and the checkpoint read directory.\n");
      status = PV_FAILURE;
   }
   return status;
}

PV_Arguments::~PV_Arguments() {
   for(int a=0; a<numArgs; a++) {
      free(args[a]);
   }
   free(args);
   clearState();
}

} /* namespace PV */