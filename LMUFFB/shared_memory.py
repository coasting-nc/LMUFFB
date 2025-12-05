import ctypes
import mmap
import struct
import platform

# Define rF2 constants
RF2_SHARED_MEMORY_NAME = "$rFactor2SMMP_Telemetry$"
if platform.system() != "Windows":
    # On Linux/Wine, the shared memory might be a file or different name. 
    # For this mock environment (likely Linux), we will simulate with a file.
    RF2_SHARED_MEMORY_NAME = "/dev/shm/$rFactor2SMMP_Telemetry$"

class rF2Vec3(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_double),
        ("y", ctypes.c_double),
        ("z", ctypes.c_double),
    ]

class rF2Wheel(ctypes.Structure):
    _fields_ = [
        ("mSuspensionDeflection", ctypes.c_double),
        ("mRideHeight", ctypes.c_double),
        ("mSuspForce", ctypes.c_double),
        ("mBrakeTemp", ctypes.c_double),
        ("mBrakePressure", ctypes.c_double),
        ("mRotation", ctypes.c_double),
        ("mLateralPatchVel", ctypes.c_double),
        ("mLongitudinalPatchVel", ctypes.c_double),
        ("mLateralGroundVel", ctypes.c_double),
        ("mLongitudinalGroundVel", ctypes.c_double),
        ("mCamber", ctypes.c_double),
        ("mLateralForce", ctypes.c_double),
        ("mLongitudinalForce", ctypes.c_double),
        ("mTireLoad", ctypes.c_double),
        ("mGripFract", ctypes.c_double),
        ("mPressure", ctypes.c_double),
        ("mTemperature", ctypes.c_double * 3),
        ("mWear", ctypes.c_double),
        ("mTerrainName", ctypes.c_char * 16),
        ("mSurfaceType", ctypes.c_byte),
        ("mFlat", ctypes.c_byte),
        ("mDetached", ctypes.c_byte),
        ("mStaticCamber", ctypes.c_double),
        ("mToeIn", ctypes.c_double),
        ("mTireRadius", ctypes.c_double),
        ("mVerticalTireDeflection", ctypes.c_double),
        ("mWheelYLocation", ctypes.c_double),
        ("mToe", ctypes.c_double),
        ("mCaster", ctypes.c_double),
        ("mHAngle", ctypes.c_double),
        ("mVAngle", ctypes.c_double),
        ("mSlipAngle", ctypes.c_double),
        ("mSlipRatio", ctypes.c_double),
        ("mMaxSlipAngle", ctypes.c_double),
        ("mMaxLatGrip", ctypes.c_double),
    ]

class rF2Telemetry(ctypes.Structure):
    _fields_ = [
        ("mTime", ctypes.c_double),
        ("mDeltaTime", ctypes.c_double),
        ("mElapsedTime", ctypes.c_double),
        ("mLapNumber", ctypes.c_int),
        ("mLapStartET", ctypes.c_double),
        ("mVehicleName", ctypes.c_char * 64),
        ("mTrackName", ctypes.c_char * 64),
        ("mPos", rF2Vec3),
        ("mLocalVel", rF2Vec3),
        ("mLocalAccel", rF2Vec3),
        ("mOri", rF2Vec3 * 3), # 3 vectors for orientation matrix
        ("mLocalRot", rF2Vec3),
        ("mLocalRotAccel", rF2Vec3),
        ("mSpeed", ctypes.c_double),
        ("mEngineRPM", ctypes.c_double),
        ("mEngineWaterTemp", ctypes.c_double),
        ("mEngineOilTemp", ctypes.c_double),
        ("mClutchRPM", ctypes.c_double),
        ("mUnfilteredThrottle", ctypes.c_double),
        ("mUnfilteredBrake", ctypes.c_double),
        ("mUnfilteredSteering", ctypes.c_double),
        ("mUnfilteredClutch", ctypes.c_double),
        ("mSteeringArmForce", ctypes.c_double),
        ("mFuel", ctypes.c_double),
        ("mEngineMaxRPM", ctypes.c_double),
        ("mScheduledStops", ctypes.c_byte),
        ("mOverheating", ctypes.c_byte),
        ("mDetached", ctypes.c_byte),
        ("mHeadlights", ctypes.c_byte),
        ("mGear", ctypes.c_int),
        ("mNumGears", ctypes.c_int),
        ("mWheels", rF2Wheel * 4),
    ]

class TelemetryReader:
    """
    Handles connection to the rFactor 2 / LMU Shared Memory Map.
    Uses 'mmap' to read the memory buffer exposed by the shared memory plugin.
    """
    def __init__(self, map_name=RF2_SHARED_MEMORY_NAME):
        self.map_name = map_name
        self.mm = None
        self.connected = False

    def connect(self):
        """
        Attempts to connect to the shared memory buffer.
        Returns True if successful, False otherwise.
        """
        try:
            # In a real windows environment:
            # self.mm = mmap.mmap(-1, ctypes.sizeof(rF2Telemetry), self.map_name)
            
            # For this mock environment, we try to open a file
            try:
                f = open(self.map_name, "r+b")
                self.mm = mmap.mmap(f.fileno(), ctypes.sizeof(rF2Telemetry))
                self.connected = True
            except FileNotFoundError:
                print(f"Shared memory file {self.map_name} not found. Waiting for game...")
                return False
        except Exception as e:
            print(f"Failed to connect to shared memory: {e}")
            return False
        return True

    def read(self):
        """
        Reads the current state from the shared memory buffer.
        Returns an instance of rF2Telemetry, or None if not connected.
        """
        if not self.connected:
            if not self.connect():
                return None
        
        self.mm.seek(0)
        buf = self.mm.read(ctypes.sizeof(rF2Telemetry))
        data = rF2Telemetry.from_buffer_copy(buf)
        return data

    def close(self):
        """Closes the mmap connection."""
        if self.mm:
            self.mm.close()

if __name__ == "__main__":
    # Test creation of file for mocking
    with open("test_shm", "wb") as f:
        f.write(bytearray(ctypes.sizeof(rF2Telemetry)))
    
    reader = TelemetryReader("test_shm")
    reader.connect()
    data = reader.read()
    print(f"Read data size: {ctypes.sizeof(data)}")
    reader.close()
