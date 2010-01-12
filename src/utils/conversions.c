/*
 * transformations.c
 *
 *  Created on: Jan 6, 2010
 *      Author: rasmussn
 */

#include "conversions.h"

/**
 * Return the leading index in z direction (either x or y) of a patch in postsynaptic layer
 * @kzPre is the pre-synaptic index in z direction (can be either local or global)
 * @zScaleLog2Pre is log2 zScale of presynaptic layer
 * @zScaleLog2Post is log2 zScale of postsynaptic layer
 * @nzPatch is the size of patch in z direction
 */
int zPatchHead(int kzPre, int nzPatch, int zScaleLog2Pre, int zScaleLog2Post)
{
   int shift = 0;

   if (nzPatch % 2 == 0 && (zScaleLog2Post < zScaleLog2Pre)) {
      // if even, can't shift evenly (at least for scale < 0)
      // the later choice alternates direction so not always to left
      shift = kzPre % 2;
   }
   shift -= (int) (0.5 * (float) nzPatch);
   return shift + nearby_neighbor(kzPre, zScaleLog2Pre, zScaleLog2Post);
}

/**
 * compute distance from kzPre to the nearest kzPost, i.e.
 *    (xPost - xPre) or (yPost - yPre)
 * in units of both pre- and post-synaptic dx (or dy).
 *
 * distance can be positive or negative
 *
 * also computes  kzPost, which is local x (or y) index of nearest cell in post layer
 *
 * @log2ScalePre
 * @log2ScalePost
 * @kzPost
 * @distPre
 * @distPost
 */
int dist2NearestCell(int kzPre, int log2ScalePre, int log2ScalePost,
      float * distPre, float * distPost)
{
   // scaleFac == 1
   int kzPost = kzPre;
   *distPost = 0.0;
   *distPre = 0.0;
   if (log2ScalePre > log2ScalePost) {
      // post-synaptic layer has smaller size scale (is denser)
      int scaleFac = pow(2, log2ScalePre) / pow(2, log2ScalePost);
      *distPost = 0.5;
      *distPre = 0.5 / scaleFac;
      kzPost = (int) ((kzPre + 0.5) * scaleFac); // - 1;  // 2 neighbors, subtract 1 for other
   }
   else if (log2ScalePre < log2ScalePost) {
      // post-synaptic layer has larger size scale (is less dense), scaleFac > 1
      int scaleFac = pow(2, log2ScalePost) / pow(2, log2ScalePre);
      *distPre = 0.5 * (scaleFac - 2 * (kzPre % scaleFac) - 1);
      *distPost = *distPre / scaleFac;
      kzPost = kzPre / scaleFac;
   }
   return kzPre;
}

/*
 * returns global x,y position of patchhead and of presynaptic cell
 */
int posPatchHead(const int kPre, const int xScaleLog2Pre,
      const int yScaleLog2Pre, const PVLayerLoc locPre, float * xPreGlobal,
      float * yPreGlobal, const int xScaleLog2Post, const int yScaleLog2Post,
      const PVLayerLoc locPost, const PVPatch * wp, float * xPatchHeadGlobal,
      float * yPatchHeadGlobal)
{
   // get global index and location of presynaptic cell
   const int nxPre = locPre.nx;
   const int nyPre = locPre.ny;
   const int nfPre = locPre.nBands;
   const int nxPreGlobal = locPre.nxGlobal;
   const int nyPreGlobal = locPre.nyGlobal;
   const int kPreGlobal = globalIndexFromLocal(kPre, locPre, nfPre);
   *xPreGlobal = xPosGlobal(kPreGlobal, xScaleLog2Pre, nxPreGlobal,
         nyPreGlobal, nfPre);
   *yPreGlobal = xPosGlobal(kPreGlobal, yScaleLog2Pre, nxPreGlobal,
         nyPreGlobal, nfPre);

   // get global index of postsynaptic patchhead
   const int kxPre = (int) kxPos(kPre, nxPre, nyPre, nfPre);
   const int kyPre = (int) kyPos(kPre, nxPre, nyPre, nfPre);
   const int kxPatchHead = zPatchHead(kxPre, wp->nx, xScaleLog2Pre, xScaleLog2Post);
   const int kyPatchHead = zPatchHead(kyPre, wp->ny, yScaleLog2Pre, yScaleLog2Post);
   const int nxPost = locPost.nx;
   const int nyPost = locPost.ny;
   const int nfPost = locPost.nBands;
   const int kPatchHead = kIndex(kxPatchHead, kyPatchHead, 0, nxPost, nyPost, nfPost);
   const int kPatchHeadGlobal = globalIndexFromLocal(kPatchHead, locPost,
         nfPost);

   // get global x,y position of patchhead
   const float nxPostGlobal = locPost.nxGlobal;
   const float nyPostGlobal = locPost.nyGlobal;
   *xPatchHeadGlobal = xPosGlobal(kPatchHeadGlobal, xScaleLog2Post,
         nxPostGlobal, nyPostGlobal, nfPost);
   *yPatchHeadGlobal = xPosGlobal(kPatchHeadGlobal, yScaleLog2Post,
         nxPostGlobal, nyPostGlobal, nfPost);

   return 0;
}
