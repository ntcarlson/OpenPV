/*
 * BaseMessage.hpp
 *
 *  Created on: Jul 21, 2016
 *      Author: pschultz
 */

#ifndef MESSAGES_HPP_
#define MESSAGES_HPP_

#include "observerpattern/BaseMessage.hpp"
#include "cMakeHeader.h"
#include "utils/Timer.hpp"
#include "include/pv_types.h"
#include <map>
#include <string>

namespace PV {

class AllocateDataMessage : public BaseMessage {
public:
   AllocateDataMessage() {
      setMessageType("AllocateDataStructures");
   }
};

class InitializeStateMessage : public BaseMessage {
public:
   InitializeStateMessage() {
      setMessageType("InitializeState");
   }
};

class ConnectionUpdateMessage : public BaseMessage {
public:
   ConnectionUpdateMessage(double simTime, double deltaTime) {
      setMessageType("ConnectionUpdate");
      mTime = simTime;
      mDeltaT = deltaTime;
   }
   double mTime;
   double mDeltaT; // TODO: this should be the nbatch-sized vector of adaptive timesteps
};

class ConnectionNormalizeMessage : public BaseMessage {
public:
   ConnectionNormalizeMessage() {
      setMessageType("ConnectionNormalize");
   }
};

class ConnectionFinalizeUpdateMessage : public BaseMessage {
public:
   ConnectionFinalizeUpdateMessage(double simTime, double deltaTime) {
      setMessageType("ConnectionFinalizeUpdate");
      mTime = simTime;
      mDeltaT = deltaTime;
   }
   double mTime;
   double mDeltaT; // TODO: this should be the nbatch-sized vector of adaptive timesteps
};

class ConnectionOutputMessage : public BaseMessage {
public:
   ConnectionOutputMessage(double simTime) {
      setMessageType("ConnectionOutput");
      mTime = simTime;
   }
   double mTime;
};

class LayerRecvSynapticInputMessage : public BaseMessage {
public:
   LayerRecvSynapticInputMessage(int phase, Timer * timer,
#ifdef PV_USE_CUDA
         bool recvOnGpuFlag,
#endif // PV_USE_CUDA
         double simTime, double deltaTime) {
      setMessageType("LayerRecvSynapticInput");
      mPhase = phase;
      mTimer = timer;
#ifdef PV_USE_CUDA
      mRecvOnGpuFlag = recvOnGpuFlag;
#endif // PV_USE_CUDA
      mTime = simTime;
      mDeltaT = deltaTime;
   }
   int mPhase;
   Timer * mTimer;
#ifdef PV_USE_CUDA
   bool mRecvOnGpuFlag;
#endif // PV_USE_CUDA
   float mTime;
   float mDeltaT; // TODO: this should be the nbatch-sized vector of adaptive timesteps
};

class LayerUpdateStateMessage : public BaseMessage {
public:
   LayerUpdateStateMessage(int phase,
#ifdef PV_USE_CUDA
         bool recvOnGpuFlag, bool updateOnGpuFlag, // updateState needs recvOnGpuFlag because correct order of updating depends on it.
#endif // PV_USE_CUDA
         double simTime, double deltaTime) {
      setMessageType("LayerUpdateState");
      mPhase = phase;
#ifdef PV_USE_CUDA
      mRecvOnGpuFlag = recvOnGpuFlag;
      mUpdateOnGpuFlag = updateOnGpuFlag;
#endif // PV_USE_CUDA
      mTime = simTime;
      mDeltaT = deltaTime;
   }
   int mPhase;
#ifdef PV_USE_CUDA
   bool mRecvOnGpuFlag;
   bool mUpdateOnGpuFlag;
#endif // PV_USE_CUDA
   float mTime;
   float mDeltaT; // TODO: this should be the nbatch-sized vector of adaptive timesteps
};

#ifdef PV_USE_CUDA
class LayerCopyFromGpuMessage : public BaseMessage {
public:
   LayerCopyFromGpuMessage(int phase, Timer* timer) {
      setMessageType("LayerCopyFromGpu");
      mPhase = phase;
      mTimer = timer;
   }
   int mPhase;
   Timer * mTimer;
};
#endif // PV_USE_CUDA

class LayerPublishMessage : public BaseMessage {
public:
   LayerPublishMessage(int phase, double simTime) {
      setMessageType("LayerPublish");
      mPhase = phase;
      mTime = simTime;
   }
   int mPhase;
   double mTime;
};

class LayerUpdateActiveIndicesMessage : public BaseMessage {
public:
   LayerUpdateActiveIndicesMessage(int phase) {
      setMessageType("LayerUpdateActiveIndices");
      mPhase = phase;
   }
   int mPhase;
};

class LayerOutputStateMessage : public BaseMessage {
public:
   LayerOutputStateMessage(int phase, double simTime) {
      setMessageType("LayerOutputState");
      mPhase = phase;
      mTime = simTime;
   }
   int mPhase;
   double mTime;
};

class LayerCheckNotANumberMessage : public BaseMessage {
public:
   LayerCheckNotANumberMessage(int phase) {
      setMessageType("LayerCheckNotANumber");
      mPhase = phase;
   }
   int mPhase;
};

class LayerAddInputSourceMessage : public BaseMessage {
public:
   LayerAddInputSourceMessage(ChannelType channel, bool gpuFlag) {
      setMessageType("LayerDeliverInput");
      mChannel = channel;
      mGPUFlag = gpuFlag;
   }
   ChannelType mChannel;
   bool mGPUFlag;
};

class DeliverInputMessage : public BaseMessage {
public:
   DeliverInputMessage() {
      setMessageType("DeliverInput");
   }
};

class ConnectionAddNormalizerMessage : public BaseMessage {
public:
   ConnectionAddNormalizerMessage() {
      setMessageType("ConnectionAddNormalizer");
   }
};

} /* namespace PV */

#endif /* MESSAGES_HPP_ */
