#ifndef RF2DATA_H
#define RF2DATA_H

#include <cstdint>

// rFactor 2 Telemetry Data Structures
// Based on The Iron Wolf's rF2SharedMemoryMapPlugin and rFactor 2 SDK

// Ensure strict alignment if necessary, but standard rF2 SDK usually works with default packing.
// However, the Shared Memory Plugin might align things specifically.
// We will use standard alignment matching the Python definition (which used native).
// Usually in C++ on Windows x64, doubles are 8-byte aligned.

#pragma pack(push, 4) // rFactor 2 often uses 4-byte packing for some legacy reasons, or default. 
// We will assume default packing for now, but if offsets are off, we might need #pragma pack(push, 1) or 4.
// Looking at the Python struct, we didn't specify _pack_, so it used native.
// Let's use standard layout.

struct rF2Vec3 {
    double x;
    double y;
    double z;
};

struct rF2Wheel {
    double mSuspensionDeflection;
    double mRideHeight;
    double mSuspForce;
    double mBrakeTemp;
    double mBrakePressure;
    double mRotation;
    double mLateralPatchVel;
    double mLongitudinalPatchVel;
    double mLateralGroundVel;
    double mLongitudinalGroundVel;
    double mCamber;
    double mLateralForce;
    double mLongitudinalForce;
    double mTireLoad;
    double mGripFract;
    double mPressure;
    double mTemperature[3]; // Inner, Middle, Outer
    double mWear;
    char mTerrainName[16];
    unsigned char mSurfaceType;
    unsigned char mFlat;
    unsigned char mDetached;
    unsigned char mPadding[5]; // Align next double? Python handled this automatically.
                               // In Python ctypes: c_byte, c_byte, c_byte follow each other.
                               // Then c_double starts. on x64, double aligns to 8.
                               // 16 + 1 + 1 + 1 = 19. Next double at 24. Padding = 5.
    
    double mStaticCamber;
    double mToeIn;
    double mTireRadius;
    double mVerticalTireDeflection;
    double mWheelYLocation;
    double mToe;
    double mCaster;
    double mHAngle;
    double mVAngle;
    double mSlipAngle;
    double mSlipRatio;
    double mMaxSlipAngle;
    double mMaxLatGrip;
};

struct rF2Telemetry {
    double mTime;
    double mDeltaTime;
    double mElapsedTime;
    int mLapNumber;
    double mLapStartET;
    char mVehicleName[64];
    char mTrackName[64];
    rF2Vec3 mPos;
    rF2Vec3 mLocalVel;
    rF2Vec3 mLocalAccel;
    rF2Vec3 mOri[3]; // [3][3] rotation matrix rows/cols? Usually 3 vectors.
    rF2Vec3 mLocalRot;
    rF2Vec3 mLocalRotAccel;
    double mSpeed;
    double mEngineRPM;
    double mEngineWaterTemp;
    double mEngineOilTemp;
    double mClutchRPM;
    double mUnfilteredThrottle;
    double mUnfilteredBrake;
    double mUnfilteredSteering;
    double mUnfilteredClutch;
    double mSteeringArmForce;
    double mFuel;
    double mEngineMaxRPM;
    unsigned char mScheduledStops;
    unsigned char mOverheating;
    unsigned char mDetached;
    unsigned char mHeadlights;
    // Padding to align int?
    // 4 chars = 4 bytes. Next is int (4 bytes). Aligned.
    int mGear;
    int mNumGears;
    // Next is rF2Wheel which starts with double (8 bytes).
    // Current pos: int(4) + int(4) = 8. Aligned.
    
    rF2Wheel mWheels[4]; // FL, FR, RL, RR
};

#pragma pack(pop)

#endif // RF2DATA_H
