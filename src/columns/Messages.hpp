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

class ReadParamsMessage : public BaseMessage {
public:
   ReadParamsMessage() {
      setMessageType("ReadParams");
   }
};

class WriteParamsMessage : public BaseMessage {
public:
   WriteParamsMessage(PV_Stream * printParamsStream, PV_Stream * printLuaParamsStream, bool includeHeaderFooter) {
      setMessageType("WriteParams");
      mPrintParamsStream = printParamsStream;
      mPrintLuaParamsStream = printLuaParamsStream;
      mIncludeHeaderFooter = includeHeaderFooter;
   }
   PV_Stream * mPrintParamsStream;
   PV_Stream * mPrintLuaParamsStream;
   bool mIncludeHeaderFooter;
};

class AllocateDataMessage : public BaseMessage {
public:
   AllocateDataMessage() {
      setMessageType("AllocateDataStructures");
   }
};

class InitializeStateMessage : public BaseMessage {
public:
   InitializeStateMessage(char const * checkpointDir) {
      setMessageType("InitializeState");
      mCheckpointDir.clear();
      if (checkpointDir) { mCheckpointDir.append(checkpointDir); } // can string::append handle a null pointer?
   }
   std::string mCheckpointDir;
};

class CheckpointReadMessage : public BaseMessage {
public:
   CheckpointReadMessage(char const * checkpointDir, double const * timestampPtr) {
      setMessageType("CheckpointRead");
      mCheckpointDir.clear();
      mCheckpointDir.append(checkpointDir);
      mTimestampPtr = timestampPtr;
   }
   std::string mCheckpointDir;
   double const * mTimestampPtr;
};

class CheckpointWriteMessage : public BaseMessage {
public:
   CheckpointWriteMessage(bool suppressCheckpointIfConstant, char const * checkpointDir, double timestamp) {
      setMessageType("CheckpointRead");
      mSuppressCheckpointIfConstant = suppressCheckpointIfConstant;
      mCheckpointDir.clear();
      mCheckpointDir.append(checkpointDir);
      mTimestamp = timestamp;
   }
   bool mSuppressCheckpointIfConstant;
   std::string mCheckpointDir;
   double mTimestamp;
};

class AdaptTimestepMessage : public BaseMessage {
public:
   AdaptTimestepMessage() {
      setMessageType("AdaptTimestep");
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

// Note: the phase argument can probably be removed.
// Before the observer pattern refactor, the order in which writeTimers was called was
// connections
// layers with phase 0
// layers with phase 1
// layers with phase 2
// etc.
// Each layer and connection called its probes' writeTimer (probes have writeTimer, not writeTimers)
// Presumably the order in which writeTimers is called shouldn't matter that much, but
// the refactor aims to implement the pattern without changing behavior.
// Afterward, we can look into streamlining the interface of WriteTimersMessage
// Currently, connections respond to a negative phase argument, and
// layers respond to the phase argument agreeing with their phase.
class WriteTimersMessage : public BaseMessage {
public:
   WriteTimersMessage(std::ostream& stream, int phase) : mStream(stream), mPhase(phase) {
      setMessageType("WriteTimers");
   }
   std::ostream& mStream;
   int mPhase;
};

} /* namespace PV */

#endif /* MESSAGES_HPP_ */
