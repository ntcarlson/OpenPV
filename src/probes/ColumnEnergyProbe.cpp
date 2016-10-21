/*
 * ColumnEnergyProbe.cpp
 *
 *  Created on: Aug 12, 2015
 *      Author: pschultz
 */

#include "ColumnEnergyProbe.hpp"
#include "columns/HyPerCol.hpp"
#include <limits>

namespace PV {

ColumnEnergyProbe::ColumnEnergyProbe()
      : ColProbe() { // Default constructor to be called by derived classes.
   // They should call ColumnEnergyProbe::initialize from their own
   // initialization routine
   // instead of calling a non-default constructor.
   initialize_base();
} // end ColumnEnergyProbe::ColumnEnergyProbe(const char *)

ColumnEnergyProbe::ColumnEnergyProbe(const char *probename, HyPerCol *hc) : ColProbe() {
   initialize_base();
   initializeColumnEnergyProbe(probename, hc);
} // end ColumnEnergyProbe::ColumnEnergyProbe(const char *, HyPerCol *)

ColumnEnergyProbe::~ColumnEnergyProbe() {
   // Don't delete terms[k]; the BaseProbes belong to the layer or connection.
   free(terms);
} // end ColumnEnergyProbe::~ColumnEnergyProbe()

int ColumnEnergyProbe::initialize_base() {
   numTerms = 0;
   terms    = NULL;

   return PV_SUCCESS;
}

int ColumnEnergyProbe::initializeColumnEnergyProbe(const char *probename, HyPerCol *hc) {
   return ColProbe::initialize(probename, hc);
}

int ColumnEnergyProbe::outputHeader() {
   if (outputStream) {
      if (!isWritingToFile()) {
         output() << "Probe_name,"; // lack of \n is deliberate
      }
      output() << "time,index,energy\n";
   }
   return PV_SUCCESS;
}

int ColumnEnergyProbe::addTerm(BaseProbe *probe) {
   if (probe == NULL) {
      return PV_FAILURE;
   }
   int status = PV_SUCCESS;
   if (numTerms == 0) {
      int newNumValues = probe->getNumValues();
      if (newNumValues < 0) {
         status = PV_FAILURE;
         if (parent->columnId() == 0) {
            pvErrorNoExit().printf(
                  "%s: %s cannot be used as a term of the energy "
                  "probe (getNumValue() returned a "
                  "negative number).\n",
                  getDescription_c(),
                  probe->getDescription_c());
         }
         MPI_Barrier(parent->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }
      if (newNumValues != this->getNumValues()) {
         status = setNumValues(newNumValues);
         if (status != PV_SUCCESS) {
            pvErrorNoExit().printf(
                  "%s: unable to allocate memory for %d probe values: %s\n",
                  getDescription_c(),
                  newNumValues,
                  strerror(errno));
            exit(EXIT_FAILURE);
         }
      }
   }
   else {
      if (probe->getNumValues() != this->getNumValues()) {
         if (this->getParent()->columnId() == 0) {
            pvErrorNoExit().printf(
                  "Failed to add terms to %s:  new probe \"%s\" "
                  "returns %d values, but previous "
                  "probes return %d values\n",
                  getDescription_c(),
                  probe->getName(),
                  probe->getNumValues(),
                  this->getNumValues());
         }
         MPI_Barrier(this->getParent()->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }
   }
   assert(probe->getNumValues() == getNumValues());
   int newNumTerms = numTerms + (size_t)1;
   if (newNumTerms <= numTerms) {
      if (this->getParent()->columnId() == 0) {
         pvErrorNoExit().printf(
               "How did you manage to add %zu terms to %s?  "
               "Unable to add any more!\n",
               numTerms,
               getDescription_c());
      }
      MPI_Barrier(this->getParent()->getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
   BaseProbe **newTermsArray =
         (BaseProbe **)realloc(terms, (numTerms + (size_t)1) * sizeof(BaseProbe *));
   if (newTermsArray == NULL) {
      pvErrorNoExit().printf(
            "%s: unable to add term %zu (\"%s\"): %s\n",
            getDescription_c(),
            numTerms + (size_t)1,
            probe->getName(),
            strerror(errno));
      exit(EXIT_FAILURE);
   }
   terms           = newTermsArray;
   terms[numTerms] = probe;
   numTerms        = newNumTerms;
   return PV_SUCCESS;
} // end ColumnEnergyProbe::addTerm(BaseProbe *, double)

bool ColumnEnergyProbe::needRecalc(double timevalue) { return true; }

double ColumnEnergyProbe::referenceUpdateTime() const { return parent->simulationTime(); }

int ColumnEnergyProbe::calcValues(double timevalue) {
   double *valuesBuffer = getValuesBuffer();
   int numValues        = this->getNumValues();
   memset(valuesBuffer, 0, numValues * sizeof(*valuesBuffer));
   double energy1[numValues];
   for (int n = 0; n < numTerms; n++) {
      BaseProbe *p = terms[n];
      p->getValues(timevalue, energy1);
      double coeff = p->getCoefficient();
      for (int b = 0; b < numValues; b++) {
         valuesBuffer[b] += coeff * energy1[b];
      }
   }
   return PV_SUCCESS;
}

int ColumnEnergyProbe::outputState(double timevalue) {
   getValues(timevalue);
   if (this->getParent()->getCommunicator()->commRank() != 0) {
      return PV_SUCCESS;
   }

   // Root process should have outputStream defined.
   // Non-root process should have outputStream==nullptr
   pvAssert(outputStream);
   double *valuesBuffer = getValuesBuffer();
   int nbatch           = this->getNumValues();
   for (int b = 0; b < nbatch; b++) {
      if (!isWritingToFile()) {
         output() << "\"" << name << "\","; // lack of \n is deliberate
      }
      output().printf("%10f, %d, %10.9f\n", timevalue, b, valuesBuffer[b]);
   }
   output().flush();
   return PV_SUCCESS;
} // end ColumnEnergyProbe::outputState(float)

} // end namespace PV