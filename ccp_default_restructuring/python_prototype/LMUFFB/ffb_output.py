import os

try:
    import pyvjoy
    VJOY_AVAILABLE = True
except ImportError:
    VJOY_AVAILABLE = False

class VJoyFFBOutput:
    """
    Interface for vJoy (Virtual Joystick) output.
    Requires 'pyvjoy' library and vJoy drivers installed on Windows.
    """
    def __init__(self, device_id=1):
        self.device_id = device_id
        if not VJOY_AVAILABLE:
            raise ImportError("pyvjoy not installed. Please install it with 'pip install pyvjoy' and ensure vJoy drivers are installed.")
        
        print(f"Initializing vJoy Device {device_id}...")
        self.vj = pyvjoy.VJoyDevice(device_id)
        
        # Axis mapping (assuming X axis for steering)
        # vJoy axis range is typically 1 to 32768
        self.AXIS_MIN = 1
        self.AXIS_MAX = 32768
        
        # FFB Constant Force specific implementation depends on how pyvjoy handles FFB packets.
        # Standard pyvjoy only sets Axis position. 
        # For True FFB (Constant Force effects), one typically needs to send FFB packets which is complex via pyvjoy
        # or use the 'vJoyInterface.dll' FFB functions via ctypes.
        
        # However, many simple FFB apps (like some iRacing implementations) map the force to an axis 
        # if the wheel is set to 'spring' mode or similar, OR they use a wrapper for FFB.
        
        # For this implementation, we will assume we are driving the X-Axis as a proxy for force 
        # OR we would need a dedicated FJoyFFB wrapper. 
        # Given the scope, we will output to the X Axis, which some bridge software can interpret, 
        # OR warn that full FFB packet implementation requires a C++ wrapper or extensive ctypes work.
        
        # NOTE: Real FFB requires FfbRegisterGenCB and FfbEffect updates. pyvjoy 1.0.3+ has some FFB support but it is experimental.
        # We will implement a basic Axis update as a placeholder for the signal.
        pass

    def send_force(self, force):
        """
        Sends force as an axis value (temporary simple implementation).
        In a full FFB driver, this would update a Constant Force effect.
        """
        # Map -1.0..1.0 to 1..32768
        val = int((force + 1.0) * 0.5 * (self.AXIS_MAX - self.AXIS_MIN) + self.AXIS_MIN)
        self.vj.set_axis(pyvjoy.HID_USAGE_X, val)

    def cleanup(self):
        print("Releasing vJoy device")
        # self.vj.reset() # Optional

class MockFFBOutput:
    """
    Mock output for testing without vJoy hardware.
    """
    def __init__(self):
        print("Initializing Mock FFB Output (DirectInput placeholder)")

    def send_force(self, force):
        """
        Sends the calculated force to the virtual wheel.
        Force is a float between -1.0 and 1.0
        """
        # In a real app, this would use pyvjoy or ctypes to call DirectInput
        # For this prototype, we just verify the value is reasonable.
        # print(f"FFB Output: {force:.4f}")
        pass

    def cleanup(self):
        print("Cleaning up FFB Output")

def get_output_device(use_vjoy=False):
    if use_vjoy and VJOY_AVAILABLE:
        try:
            return VJoyFFBOutput()
        except Exception as e:
            print(f"Failed to initialize vJoy: {e}. Falling back to Mock.")
            return MockFFBOutput()
    else:
        if use_vjoy:
            print("vJoy requested but not available. Falling back to Mock.")
        return MockFFBOutput()
