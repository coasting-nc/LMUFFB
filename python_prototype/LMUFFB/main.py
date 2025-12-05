import time
import sys
import ctypes
import os

# Adjust path to handle direct execution or module execution
if __name__ == "__main__" and __package__ is None:
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from LMUFFB.shared_memory import TelemetryReader, rF2Telemetry, RF2_SHARED_MEMORY_NAME
from LMUFFB.ffb_engine import FFBEngine
from LMUFFB.ffb_output import get_output_device

def main():
    print("Starting LMU FFB App...")
    
    # 1. Initialize Telemetry
    # In Mock mode, we need to ensure the shared memory file exists if we are on Linux/Mock
    import os
    reader = None
    if not os.path.exists(RF2_SHARED_MEMORY_NAME):
        print(f"Creating mock shared memory file at {RF2_SHARED_MEMORY_NAME}")
        try:
             # Ensure directory exists for /dev/shm or local
            if "/" in RF2_SHARED_MEMORY_NAME:
                os.makedirs(os.path.dirname(RF2_SHARED_MEMORY_NAME), exist_ok=True)
            with open(RF2_SHARED_MEMORY_NAME, "wb") as f:
                f.write(bytearray(ctypes.sizeof(rF2Telemetry)))
            reader = TelemetryReader(RF2_SHARED_MEMORY_NAME)
        except OSError:
             # Fallback to local file for testing
             print("Could not create in /dev/shm, using local file.")
             # We can't change the imported global constant directly if it's imported via 'from module import X'
             # It's better to just pass the new name to TelemetryReader
             local_map_name = "rFactor2SMMP_Telemetry_Mock"
             with open(local_map_name, "wb") as f:
                f.write(bytearray(ctypes.sizeof(rF2Telemetry)))
             reader = TelemetryReader(local_map_name)
    else:
         reader = TelemetryReader(RF2_SHARED_MEMORY_NAME)

    if not reader.connected and not reader.connect():
        print("Could not connect to shared memory.")
        sys.exit(1)

    # 2. Initialize Engine and Output
    engine = FFBEngine()
    # Try to use vJoy if available, else Mock
    output = get_output_device(use_vjoy=True)

    print("App running. Press Ctrl+C to stop.")
    try:
        while True:
            # Read Telemetry
            data = reader.read()
            
            if data:
                # Calculate Force
                force = engine.calculate_force(data)
                
                # Send to Output
                output.send_force(force)
                
                # Debug output every ~1 second (assuming 100hz loop for mock)
                if int(time.time() * 10) % 10 == 0:
                   # print(f"Time: {data.mTime:.2f} | Force: {force:.4f}")
                   pass
            
            time.sleep(0.01) # 100 Hz

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        output.cleanup()
        reader.close()

if __name__ == "__main__":
    main()
