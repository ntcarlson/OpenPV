/*
 * fileio.hpp
 *
 *  Created on: Oct 21, 2009
 *      Author: rasmussn
 */

#ifndef FILEIO_HPP_
#define FILEIO_HPP_

#include "io.hpp"
#include "arch/mpi/mpi.h"
#include "include/pv_types.h"
#include "include/PVLayerLoc.h"
#include "columns/Communicator.hpp"
#include "columns/DataStore.hpp"
#include "io/FileStream.hpp"
#include "utils/PVLog.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits>

namespace PV {

// index/value pairs used by writeActivitySparseNonspiking()
typedef struct indexvaluepair_ {
    unsigned int index;
    pvdata_t value;
} indexvaluepair;

void timeToParams(double time, void * params);
double timeFromParams(void * params);

size_t pv_sizeof(int datatype);

PV_Stream * PV_fopen(const char * path, const char * mode, bool verifyWrites);
int PV_stat(const char * path, struct stat * buf);
long int getPV_StreamFilepos(PV_Stream * pvstream);
long int updatePV_StreamFilepos(PV_Stream * pvstream);
long int PV_ftell(PV_Stream * pvstream);
int PV_fseek(PV_Stream * pvstream, long int offset, int whence);
size_t PV_fwrite(const void * RESTRICT ptr, size_t size, size_t nitems, PV_Stream * RESTRICT pvstream);
size_t PV_fread(void * RESTRICT ptr, size_t size, size_t nitems, PV_Stream * RESTRICT pvstream);
int PV_fclose(PV_Stream * pvstream);
int ensureDirExists(Communicator * comm, const char* dirname);

PV_Stream * pvp_open_read_file(const char * filename, Communicator * comm);

PV_Stream * pvp_open_write_file(const char * filename, Communicator * comm, bool append);

int pvp_close_file(PV_Stream * pvstream, Communicator * comm);

int pvp_read_header(PV_Stream * pvstream, Communicator * comm, int * params, int * numParams);
int pvp_read_header(const char * filename, Communicator * comm, double * time,
                    int * filetype, int * datatype, int params[], int * numParams);
void read_header_err(const char * filename, Communicator * comm, int returned_num_params, int * params);
int pvp_write_header(PV_Stream * pvstream, Communicator * comm, int * params, int numParams);

// The pvp_write_header below will go away in favor of the pvp_write_header above.
int pvp_write_header(PV_Stream * pvstream, Communicator * comm, double time, const PVLayerLoc * loc,
                     int filetype, int datatype, int numbands,
                     bool extended, bool contiguous, unsigned int numParams, size_t recordSize);

int * pvp_set_file_params(Communicator * comm, double timed, const PVLayerLoc * loc, int datatype, int numbands);
int * pvp_set_activity_params(Communicator * comm, double timed, const PVLayerLoc * loc, int datatype, int numbands);
int * pvp_set_weight_params(Communicator * comm, double timed, const PVLayerLoc * loc, int datatype, int numbands, int nxp, int nyp, int nfp, float min, float max, int numPatches);
int * pvp_set_nonspiking_act_params(Communicator * comm, double timed, const PVLayerLoc * loc, int datatype, int numbands);
int * pvp_set_kernel_params(Communicator * comm, double timed, const PVLayerLoc * loc, int datatype, int numbands, int nxp, int nyp, int nfp, float min, float max, int numPatches);
int * alloc_params(int numParams);
int set_weight_params(int * params, int nxp, int nyp, int nfp, float min, float max, int numPatches);

int pvp_read_time(PV_Stream * pvstream, Communicator * comm, int root_process, double * timed);

int writeActivity(PV_Stream * pvstream, Communicator * comm, double timed, DataStore * store, const PVLayerLoc* loc);

int writeActivitySparse(PV_Stream * pvstream, Communicator * comm, double timed, DataStore * store, const PVLayerLoc* loc, bool includeValues);

//This function is not defined anywhere?
//int writeActivitySparseValues(PV_Stream * pvstream, PV_Stream * posstream, Communicator * comm, double time, PVLayer * l);

int readWeights(PVPatch *** patches, pvwdata_t ** dataStart, int numArbors, int numPatches, int nxp, int nyp, int nfp, const char * filename,
                Communicator * comm, double * timed, const PVLayerLoc * loc);

#ifdef OBSOLETE // Marked obsolete June 27, 2016.
// The old readWeights, now readWeightsDeprecated, was deprecated Nov 20, 2014.
// readWeights() reads weights that were saved in an MPI-independent manner (the current writeWeights)
// readWeightsDeprecated() reads weights saved in the old MPI-dependent manner.
int readWeightsDeprecated(PVPatch *** patches, pvwdata_t ** dataStart, int numArbors, int numPatches, int nxp, int nyp, int nfp, const char * filename,
                Communicator * comm, double * timed, const PVLayerLoc * loc);
#endif // OBSOLETE // Marked obsolete June 27, 2016.

int pv_text_write_patch(OutStream * pvstream, PVPatch * patch, pvwdata_t * data, int nf, int sx, int sy, int sf);

int writeWeights(const char * filename, Communicator * comm, double timed, bool append,
                 const PVLayerLoc * preLoc, const PVLayerLoc * postLoc, int nxp, int nyp, int nfp, float minVal, float maxVal,
                 PVPatch *** patches, pvwdata_t ** dataStart, int numPatches, int numArbors, bool compress=true, int file_type=PVP_WGT_FILE_TYPE);

int pvp_check_file_header(Communicator * comm, const PVLayerLoc * loc, int params[], int numParams);

int writeRandState(const char * filename, Communicator * comm, taus_uint4 * randState, const PVLayerLoc * loc, bool isExtended, bool verifyWrites);

int readRandState(const char * filename, Communicator * comm, taus_uint4 * randState, const PVLayerLoc * loc, bool isExtended);

template <typename T> int gatherActivity(PV_Stream * pvstream, Communicator * comm, int rootproc, T * buffer, const PVLayerLoc * layerLoc, bool extended);
template <typename T> int scatterActivity(PV_Stream * pvstream, Communicator * comm, int rootproc, T * buffer, const PVLayerLoc * layerLoc, bool extended, const PVLayerLoc * fileLoc=NULL, int offsetX=0, int offsetY=0, int filetype=PVP_NONSPIKING_ACT_FILE_TYPE, int numActive=0);


/**
 * Uses the arguments directory, objectName, and suffix to create a path of the form
 * [cpDir]/[objectName]_[suffix].[extension]
 * (the brackets are not in the created path, but the separators "/", "_", and "." are).
 * cpDir, objectName, and suffix should not be null.
 * If extension is null, the result does not contain the period.
 * If extension is the empty string, the result does contain the period as the last character.
 * The string returned is allocated with new, and the calling routine is responsible
 * for deleting the string.
 */
std::string * pathInCheckpoint(char const * cpDir, char const * objectName, char const * suffix, char const * extension);

template <typename T>
int readArrayFromFile(const char * directory, const char* groupName, const char* arrayName, Communicator * comm, T * val, size_t count, T defaultValue=(T) 0) {
   std::string * filename = pathInCheckpoint(directory, groupName, arrayName, "bin");
   return readArrayFromFile(filename->c_str(), comm, val, count, defaultValue);
   delete filename;
}

template <typename T>
int readArrayFromFile(char const * filename, Communicator * comm, T * val, size_t count, T defaultValue=(T) 0) {
   if (comm->commRank()==0)  {
      PV_Stream * pvstream = PV_fopen(filename, "r", false/*verifyWrites not used when reading*/);
      if (pvstream==nullptr) {
         pvError() << "readArrayFromFile unable to open path \"" << filename << "\" for reading.\n";
      }
      int num_written = PV_fread(val, sizeof(T), count, pvstream);
      if (num_written != count) {
         pvError() << "readArrayFromFile unable to read from \"" << filename << "\".\n";
      }
      PV_fclose(pvstream);
   }
   MPI_Bcast(val, sizeof(T)*count, MPI_CHAR, 0, comm->communicator());
   return PV_SUCCESS;
}

template <typename T>
int readScalarFromFile(char const * directory, char const * groupName, char const * arrayName, Communicator * comm, T * val, T defaultValue=(T) 0) {
   return readArrayFromFile(directory, groupName, arrayName, comm, val, (size_t) 1, defaultValue);
}

template <typename T>
int writeArrayToFile(char const * directory, char const * groupName, char const * arrayName, Communicator * comm, T * val, size_t count, bool verifyWrites) {
   std::string * filenamebin = pathInCheckpoint(directory, groupName, arrayName, "bin");
   std::string * filenametxt = pathInCheckpoint(directory, groupName, arrayName, "txt");
   return writeArrayToFile(filenamebin->c_str(), filenametxt->c_str(), comm, val, count, verifyWrites);
   delete filenamebin;
   delete filenametxt;
}

template <typename T>
int writeArrayToFile(char const * filenamebin, char const * filenametxt, Communicator * comm, T *  val, size_t count, bool verifyWrites) {
   if (comm->commRank()==0)  {
      PV_Stream * pvstream = PV_fopen(filenamebin, "w", verifyWrites);
      if (pvstream==nullptr) {
         pvError().printf("writeArrayToFile unable to open path %s for writing.\n", filenamebin);
      }
      int num_written = PV_fwrite(val, sizeof(T), count, pvstream);
      if (num_written != count) {
         pvError().printf("writeArrayToFile unable to write to %s.\n", filenamebin);
      }
      PV_fclose(pvstream);

      if (filenametxt == nullptr) { return PV_SUCCESS; }
      std::ofstream fs;
      fs.open(filenametxt);
      if (!fs) {
         pvError() << "writeArrayToFile unable to open path \"" << filenametxt << "\" for writing.\n";
      }
      for(int i = 0; i < count; i++){
         fs << val[i] << "\n";
      }
      fs.close();
   }
   return PV_SUCCESS;
}

template <typename T>
int writeScalarToFile(char const * directory, char const * groupName, char const * arrayName, Communicator * comm, T val, bool verifyWrites) {
   return writeArrayToFile(directory, groupName, arrayName, comm, &val, (size_t) 1, verifyWrites);
}

template <typename T>
void appendParamValueToString(T value, std::string * vstr) {
   if (std::numeric_limits<T>::has_infinity) {
      if (value<=-FLT_MAX*0.9999) {
         vstr->append("-infinity");
      }
      else if (value>=FLT_MAX*1.0001) {
         vstr->append("infinity");
      }
      else {
         vstr->append(std::to_string(value));
      }
   }
   else {
      vstr->append(std::to_string(value));
   }
}

template <typename T>
void writeFormattedParamValue(const char * paramName, T value, PV_Stream * printParamsStream, PV_Stream * printLuaParamsStream) {
   if (!printParamsStream && !printLuaParamsStream) { return; }
   std::string vstr("");
   vstr.reserve(50);
   vstr.append("    ").append(paramName);
   if (vstr.length()<39) { vstr.resize(39, ' ');}
   vstr.append(" = ");
   appendParamValueToString(value, &vstr);
   vstr.append(";\n");
   if (printParamsStream && printParamsStream->fp) {
      fwrite(vstr.c_str(), 1, vstr.size(), printParamsStream->fp);
   }
   if (printLuaParamsStream && printLuaParamsStream->fp) {
      fwrite(vstr.c_str(), 1, vstr.size(), printLuaParamsStream->fp);
   }
}

} // namespace PV

#endif /* FILEIO_HPP_ */
