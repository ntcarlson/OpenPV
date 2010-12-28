#ifndef RETINA_PARAMS_H_
#define RETINA_PARAMS_H_

typedef struct LIF_params_ {
   float probStim;
   float probBase;
   float beginStim;
   float endStim;
   float burstFreq;          // frequency of bursts
   float burstDuration;      // duration of each burst, <=0 -> sinusoidal

   float refactory_period;
   float abs_refactory_period;
} Retina_params;

#endif /* RETINA_PARAMS_H_ */

