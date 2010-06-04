/*
 * Image.cpp
 *
 *  Created on: Sep 8, 2009
 *      Author: rasmussn
 */

#include "Image.hpp"
#include "../io/imageio.hpp"

#include <assert.h>
#include <string.h>

namespace PV {

Image::Image(const char * name, HyPerCol * hc)
     : HyPerLayer(name, hc)
{
   initialize(TypeImage);
   initializeImage(NULL);
}

Image::Image(const char * name, HyPerCol * hc, const char * filename)
     : HyPerLayer(name, hc)
{
   initialize(TypeImage);
   initializeImage(filename);
}

Image::~Image()
{
   if (filename != NULL) free(filename);
#ifdef OBSOLETE
   if (data != NULL) {
      free(data);
      data = NULL;
   }
#endif
}

#ifdef OBSOLETE
int Image::initialize_base(const char * name, HyPerCol * hc)
{
   this->comm = hc->icCommunicator();

   PVParams * params = hc->parameters();

   PVLayerLoc * loc = & clayer->loc;
   loc->nBands = 1;
   loc->nPad = (int) params->value(name, "marginWidth", 0);

   // create mpi_datatypes for border transfer
   mpi_datatypes = Communicator::newDatatypes(getLayerLoc());

   return 0;
}
#endif

#ifdef OBSOLETE
int Image::initGlobal(int colId, int colRow, int colCol, int nRows, int nCols)
{
   int status = HyPerLayer::initGlobal(colId, colRow, colCol, nRows, nCols);

   // need all image bands until converted to gray scale
   PVLayerLoc * loc = & clayer->loc;
   clayer->loc.nBands = getImageLoc().nBands;

   initialize_data(loc);

   read(filename);

   // for now convert images to grayscale
   if (loc->nBands > 1) {
      this->toGrayScale();
   }

   // exchange border information
   exchange();
   return 0;
}
#endif

/**
 * data lives in an extended frame of size
 * (nx+2*nPad)*(ny+2*nPad)*nBands
 */
int Image::initializeImage(const char * filename)
{
   int status = 0;

   if (filename != NULL) {
      this->filename = strdup(filename);
      status = getImageInfo(filename, parent->icCommunicator(), &imageLoc);
   }
   else {
      this->filename = NULL;
      this->imageLoc = * getLayerLoc();
   }
   this->lastUpdateTime = 0.0;

   // get size info from image so that data buffer can be allocated
// TODO - must make image conform to layer size

#ifdef OBSOLETE
   // allocate storage for actual image
   //
//   int N = imageLoc.nx * imageLoc.ny * imageLoc.nBands;
//   imageData = (pvdata_t *) calloc(sizeof(pvdata_t), N);
//   assert(imageData != NULL);

   // allocate storage for layer data buffer
   //
   const int N = (dataLoc->nx + 2*dataLoc->nPad) * (dataLoc->ny + 2*dataLoc->nPad)
               * dataLoc->nBands;
   data = (pvdata_t *) calloc(sizeof(pvdata_t), N);
   assert(data != NULL);
#endif
   data = clayer->activity->data;

   // create mpi_datatypes for border transfer
   mpi_datatypes = Communicator::newDatatypes(getLayerLoc());

   if (filename != NULL) {
      read(filename);
   }

   // for now convert images to grayscale
   if (getLayerLoc()->nBands > 1) {
      this->toGrayScale();
   }

   // exchange border information
   exchange();

   return status;
}

pvdata_t * Image::getImageBuffer()
{
   return data;
}

PVLayerLoc Image::getImageLoc()
{
   return imageLoc;
}

/**
 * return some useful information about the image
 */
int Image::tag()
{
   return 0;
}

//pvdata_t * Image::getDataBuffer()
//{
//   return data;
//}

//PVLayerLoc Image::getDataLoc()
//{
//   return loc;
//}

int Image::recvSynapticInput(HyPerConn * conn, PVLayerCube * cube, int neighbor)
{
   // this should never be called as an image shouldn't have an incoming connection
   return 0;
}

/**
 * update the image buffers
 */
int Image::updateState(float time, float dt)
{
   // this should replace updateImage
   // make sure image is copied to activity buffer
   //
   return 0;
}

int Image::outputState(float time, bool last)
{
   // this could probably use Marion's update time interval
   // for some classes
   //
   return 0;
}

#ifdef OBSOLETE
/**
 * update the image buffers
 *
 * return true if buffers have changed
 */
bool Image::updateImage(float time, float dt)
{
   // default is to do nothing for now
   // eventually could go through a list of images
   return false;
}
#endif

//! CLEAR IMAGE
/*!
 * this is Image specific.
 * NOTE:
 *      - Shall I modify this method to return a bool as
 * updateImage() does?
 *      - If I do so, I should also modify clearImage() in ImagrCreator().
 *      .
 */
int Image::clearImage()
{
   // default is to do nothing for now
   // it could, for example, set the data buffer to zero.

   return 0;
}

int Image::read(const char * filename)
{
   int status = 0;
   PVLayerLoc * loc = & clayer->loc;

   const int n = loc->nx * loc->ny * loc->nBands;
   unsigned char * buf = new unsigned char[n];
   assert(buf != NULL);

   // read the image and scatter the local portions
   status = scatterImageFile(filename, parent->icCommunicator(), loc, buf);

   if (status == 0) {
      status = copyFromInteriorBuffer(buf);
   }
   delete buf;

   return status;
}

int Image::write(const char * filename)
{
   int status = 0;
   const PVLayerLoc * loc = getLayerLoc();

   const int n = loc->nx * loc->ny * loc->nBands;
   unsigned char * buf = new unsigned char[n];
   assert(buf != NULL);

   status = copyToInteriorBuffer(buf, 255.0);

   // gather the local portions and write the image
   status = gatherImageFile(filename, parent->icCommunicator(), loc, buf);

   delete buf;

   return status;
}

int Image::exchange()
{
   return parent->icCommunicator()->exchange(data, mpi_datatypes, getLayerLoc());
}

int Image::gatherToInteriorBuffer(unsigned char * buf)
{
   return HyPerLayer::gatherToInteriorBuffer(buf);
#ifdef OBSOLETE
   const PVLayerLoc * loc = getLayerLoc();

   assert(loc->nBands == 1);

   const int nx = loc->nx;
   const int ny = loc->ny;

   const int nxBorder = loc->nPad;
   const int nyBorder = loc->nPad;

   const int sy = nx + 2*nxBorder;
   const int sb = sy * (ny + 2*nyBorder);

   // only interior portion of local data needed
   //
   unsigned char * srcBuf = (unsigned char *) malloc(nx * ny * sizeof(unsigned char));
   assert(srcBuf != NULL);

   int ii = 0;
   for (int b = 0; b < loc->nBands; b++) {
      for (int j = 0; j < ny; j++) {
         int jex = j + nyBorder;
         for (int i = 0; i < nx; i++) {
            int iex = i + nxBorder;
            srcBuf[ii++] = (unsigned char) (255.0f * data[iex + jex*sy + b*sb]);
         }
      }
   }

   gather(parent->icCommunicator(), loc, buf, srcBuf);

   free(srcBuf);

   return 0;
#endif
}

int Image::copyToInteriorBuffer(unsigned char * buf, float fac)
{
   const PVLayerLoc * loc = getLayerLoc();
   const int nx = loc->nx;
   const int ny = loc->ny;

   const int nxBorder = loc->nPad;
   const int nyBorder = loc->nPad;

   const int sy = nx + 2*nxBorder;
   const int sb = sy * (ny + 2*nyBorder);

   int ii = 0;
   for (int b = 0; b < loc->nBands; b++) {
      for (int j = 0; j < ny; j++) {
         int jex = j + nyBorder;
         for (int i = 0; i < nx; i++) {
            int iex = i + nxBorder;
            buf[ii++] = (unsigned char) (fac * data[iex + jex*sy + b*sb]);
         }
      }
   }
   return 0;
}

int Image::copyFromInteriorBuffer(const unsigned char * buf)
{
   const PVLayerLoc * loc = getLayerLoc();
   const int nx = loc->nx;
   const int ny = loc->ny;

   const int nxBorder = loc->nPad;
   const int nyBorder = loc->nPad;

   const int sy = nx + 2*nxBorder;
   const int sb = sy * (ny + 2*nyBorder);

   int ii = 0;
   for (int b = 0; b < loc->nBands; b++) {
      for (int j = 0; j < ny; j++) {
         int jex = j + nyBorder;
         for (int i = 0; i < nx; i++) {
            int iex = i + nxBorder;
            data[iex + jex*sy + b*sb] = (pvdata_t) buf[ii++];
         }
      }
   }
   return 0;
}

int Image::toGrayScale()
{
   const PVLayerLoc * loc = getLayerLoc();
   const int nx_ex = loc->nx + 2*loc->nPad;
   const int ny_ex = loc->ny + 2*loc->nPad;

   const int numBands = loc->nBands;

   const int sx = 1;
   const int sy = nx_ex;
   const int sb = nx_ex * ny_ex;

   if (numBands < 2) return 0;

   for (int j = 0; j < ny_ex; j++) {
      for (int i = 0; i < nx_ex; i++) {
         float val = 0;
         for (int b = 0; b < numBands; b++) {
            float d = data[i*sx + j*sy + b*sb];
            val += d*d;
//            val += d;
         }
         // store the converted image in the first color band
         data[i*sx + j*sy + 0*sb] = sqrtf(val/numBands);
//         data[i*sx + j*sy + 0*sb] = val/numBands;
      }
   }

   // turn off the color
   clayer->loc.nBands = 1;

   return 0;
}

int Image::convertToGrayScale(PVLayerLoc * loc, unsigned char * buf)
{
   const int nx = loc->nx;
   const int ny = loc->ny;

   const int numBands = loc->nBands;

   const int sx = 1;
   const int sy = nx;
   const int sb = nx * ny;

   if (numBands < 2) return 0;

   for (int j = 0; j < ny; j++) {
      for (int i = 0; i < nx; i++) {
         float val = 0;
         for (int b = 0; b < numBands; b++) {
            float d = buf[i*sx + j*sy + b*sb];
            val += d*d;
         }
         // store the converted image in the first color band
         buf[i*sx + j*sy + 0*sb] = (unsigned char) sqrtf(val/numBands);
      }
   }

   // turn off the color
   loc->nBands = 1;

   return 0;
}

int Image::convolve(int width)
{
   const PVLayerLoc * loc = getLayerLoc();
   const int nx_ex = loc->nx + 2*loc->nPad;
   const int ny_ex = loc->ny + 2*loc->nPad;
   //const int nb = loc->nBands;

   const int size_ex = nx_ex * ny_ex;

   // an image is different from normal layers as features (bands) vary last
   const int sx = 1;
   const int sy = nx_ex;
   //const int sb = nx_ex * ny_ex;

   const int npx = width;
   const int npy = width;
   const int npx_2 = width/2;
   const int npy_2 = width/2;

   assert(npx <= loc->nPad);
   assert(npy <= loc->nPad);

   float * buf = new float[size_ex];
   //for (int i = 0; i < size_ex; i++) buf[i] = 0;

   float max = -1.0e9;
   float min = -max;

   // ignore image bands for now
   for (int jex = npy_2; jex < ny_ex - npy_2; jex++) {
      for (int iex = npx_2; iex < nx_ex - npx_2; iex++) {
         float av = 0;
         float sq = 0;
         for (int jp = 0; jp < npy; jp++) {
            for (int ip = 0; ip < npx; ip++) {
   //            int ix = i + ip - npx_2;
   //            int iy = j + jp - npy_2;
   //            float val = data[ix*sx + iy*sy];
   //            av += val;
   //            sq += val * val;
            }
         }
         av = av / (npx*npy);
         min = (av < min) ? av : min;
         max = (av > max) ? av : max;
//         sq  = sqrt( sq/(nPad*nPad) - av*av ) + tau;
//         buf[i*sx + j*sy] = data[i*sx + j*sy] + mid - av;
         buf[iex*sx + jex*sy] = .95f * 255.0f * (data[iex*sx + jex*sy] - .95f * av) / sq;
      }
   }

   printf("min==%f max==%f\n", min, max);

   for (int k = 0; k < size_ex; k++) data[k] = buf[k];

   return 0;
}

} // namespace PV
