import unittest
import ctypes
import os
import time
from LMUFFB.shared_memory import TelemetryReader, rF2Telemetry
from LMUFFB.ffb_engine import FFBEngine

class TestLMUFFB(unittest.TestCase):
    def setUp(self):
        self.test_map_name = "test_shm_unittest"
        with open(self.test_map_name, "wb") as f:
            f.write(bytearray(ctypes.sizeof(rF2Telemetry)))
        self.reader = TelemetryReader(self.test_map_name)
        self.reader.connect()
        self.engine = FFBEngine()

    def tearDown(self):
        self.reader.close()
        if os.path.exists(self.test_map_name):
            os.remove(self.test_map_name)

    def test_read_telemetry(self):
        data = self.reader.read()
        self.assertIsNotNone(data)
        self.assertEqual(data.mTime, 0.0)

    def test_calculate_force_zero(self):
        data = rF2Telemetry()
        force = self.engine.calculate_force(data)
        self.assertEqual(force, 0.0)

    def test_calculate_force_with_values(self):
        data = rF2Telemetry()
        # Mock some values
        data.mSteeringArmForce = 2000.0
        data.mWheels[0].mLateralForce = 1000.0
        data.mWheels[1].mLateralForce = 1000.0
        data.mWheels[0].mGripFract = 1.0 # Full grip
        data.mWheels[1].mGripFract = 1.0
        data.mLocalAccel.x = 5.0 # ~0.5 G
        
        force = self.engine.calculate_force(data)
        # 2000 * 1.0 + (0.509 * 0.5 * 1000) = 2254.5 approx (5.0 / 9.81 = 0.5096)
        # 2254.8 / 4000 = 0.5637
        self.assertAlmostEqual(force, 0.5637, places=3) # approx check

    def test_calculate_force_grip_loss(self):
        data = rF2Telemetry()
        # Mock some values
        data.mSteeringArmForce = 2000.0
        data.mWheels[0].mGripFract = 0.5 # Grip loss
        data.mWheels[1].mGripFract = 0.5
        data.mLocalAccel.x = 0.0 
        
        force = self.engine.calculate_force(data)
        # 2000 * 0.5 = 1000
        # 1000 / 4000 = 0.25
        self.assertAlmostEqual(force, 0.25, places=3)

if __name__ == '__main__':
    unittest.main()
