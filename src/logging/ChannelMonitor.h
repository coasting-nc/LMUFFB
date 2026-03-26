#ifndef CHANNELMONITOR_H
#define CHANNELMONITOR_H

#include "RateMonitor.h"
#include "io/lmu_sm_interface/LmuSharedMemoryWrapper.h"

namespace LMUFFB {

// Extended monitors for Issue #133
struct ChannelMonitor {
    RateMonitor monitor;
    double lastValue = -1e18;
    void Update(double newValue) {
        if (newValue != lastValue) {
            monitor.RecordEvent();
            lastValue = newValue;
        }
    }
};

struct ChannelMonitors {
    ChannelMonitor mAccX, mAccY, mAccZ;
    ChannelMonitor mVelX, mVelY, mVelZ;
    ChannelMonitor mRotX, mRotY, mRotZ;
    ChannelMonitor mRotAccX, mRotAccY, mRotAccZ;
    ChannelMonitor mUnfSteer, mFilSteer;
    ChannelMonitor mRPM;
    ChannelMonitor mLoadFL, mLoadFR, mLoadRL, mLoadRR;
    ChannelMonitor mLatFL, mLatFR, mLatRL, mLatRR;
    ChannelMonitor mPosX, mPosY, mPosZ;
    ChannelMonitor mDtMon;

    void UpdateAll(const TelemInfoV01* pPlayerTelemetry) {
        mAccX.Update(pPlayerTelemetry->mLocalAccel.x);
        mAccY.Update(pPlayerTelemetry->mLocalAccel.y);
        mAccZ.Update(pPlayerTelemetry->mLocalAccel.z);
        mVelX.Update(pPlayerTelemetry->mLocalVel.x);
        mVelY.Update(pPlayerTelemetry->mLocalVel.y);
        mVelZ.Update(pPlayerTelemetry->mLocalVel.z);
        mRotX.Update(pPlayerTelemetry->mLocalRot.x);
        mRotY.Update(pPlayerTelemetry->mLocalRot.y);
        mRotZ.Update(pPlayerTelemetry->mLocalRot.z);
        mRotAccX.Update(pPlayerTelemetry->mLocalRotAccel.x);
        mRotAccY.Update(pPlayerTelemetry->mLocalRotAccel.y);
        mRotAccZ.Update(pPlayerTelemetry->mLocalRotAccel.z);
        mUnfSteer.Update(pPlayerTelemetry->mUnfilteredSteering);
        mFilSteer.Update(pPlayerTelemetry->mFilteredSteering);
        mRPM.Update(pPlayerTelemetry->mEngineRPM);
        mLoadFL.Update(pPlayerTelemetry->mWheel[0].mTireLoad);
        mLoadFR.Update(pPlayerTelemetry->mWheel[1].mTireLoad);
        mLoadRL.Update(pPlayerTelemetry->mWheel[2].mTireLoad);
        mLoadRR.Update(pPlayerTelemetry->mWheel[3].mTireLoad);
        mLatFL.Update(pPlayerTelemetry->mWheel[0].mLateralForce);
        mLatFR.Update(pPlayerTelemetry->mWheel[1].mLateralForce);
        mLatRL.Update(pPlayerTelemetry->mWheel[2].mLateralForce);
        mLatRR.Update(pPlayerTelemetry->mWheel[3].mLateralForce);
        mPosX.Update(pPlayerTelemetry->mPos.x);
        mPosY.Update(pPlayerTelemetry->mPos.y);
        mPosZ.Update(pPlayerTelemetry->mPos.z);
        mDtMon.Update(pPlayerTelemetry->mDeltaTime);
    }
};

} // namespace LMUFFB

#endif // CHANNELMONITOR_H
