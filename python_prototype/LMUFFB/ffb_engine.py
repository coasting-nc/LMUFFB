import math

class FFBEngine:
    """
    Core Logic for calculating Force Feedback signals.
    Takes raw telemetry input and produces a normalized force value (-1.0 to 1.0).
    """
    def __init__(self):
        self.smoothing = 0.5
        self.gain = 1.0
        self.sop_factor = 0.5 # Seat of Pants factor (Grip loss feel)

    def calculate_force(self, telemetry):
        """
        Calculates the FFB force based on telemetry data.
        Returns a float between -1.0 and 1.0.
        
        Logic:
        1. Base Force: Takes the native SteeringArmForce from the game.
        2. Grip Modulation: Scales the base force by the tire Grip Fraction.
           - If tires are slipping (Grip < 1.0), the force is reduced (wheel feels lighter).
        3. SoP (Seat of Pants): Adds a lateral G-force component to simulate body load.
        """
        if not telemetry:
            return 0.0

        # Extract relevant data
        # Use Front Left and Front Right wheels for steering feedback
        fl_wheel = telemetry.mWheels[0]
        fr_wheel = telemetry.mWheels[1]
        
        # Pneumatic trail calculation simplified
        # Torque = Lateral Force * Trail
        # We don't have trail explicitly, but we have Lateral Force and Aligning Torque usually comes from game physics.
        # But here we want to calculate it ourselves or enhance it.
        
        # Simple "Magic Formula" style approximation or enhancement
        # We want to feel the grip drop off.
        
        # Calculate lateral force average for front axle
        lat_force = (fl_wheel.mLateralForce + fr_wheel.mLateralForce) / 2.0
        
        # Steering Arm Force from game (if valid)
        game_force = telemetry.mSteeringArmForce
        
        # Calculate localized slip angle average
        slip_angle = (fl_wheel.mSlipAngle + fr_wheel.mSlipAngle) / 2.0
        
        # Calculate Grip Fraction (0 to 1, where < 1 means sliding)
        # Assuming mGripFract is available and correct. 
        # mGripFract: Fraction of tire grip being used? Or grip remaining?
        # rF2 usually gives grip fraction.
        grip_l = fl_wheel.mGripFract
        grip_r = fr_wheel.mGripFract
        avg_grip = (grip_l + grip_r) / 2.0
        
        # SoP Effect: When grip is lost (avg_grip drops), we want to change the force.
        # Typically, aligning torque increases with slip angle up to a peak, then drops.
        # If the game FFB is good, it does this. If not, we simulated it.
        
        # Synthetic Force:
        # F = Load * sin(SlipAngle) * Friction
        # But we can just use the Lateral Force provided by game physics which is accurate,
        # and modulate it based on our preference.
        
        # Pure replacement mode (like irFFB often does):
        # Re-calculate torque from physics.
        # Torque = Sum(LateralForce_i * PneumaticTrail_i + LongitudinalForce_i * ScrubRadius)
        
        # For this prototype, let's assume we want to output the Lateral Force scaled, 
        # but emphasized by the Grip Fraction to simulate the "lightening" of the wheel.
        
        # If we use game_force as base:
        # Modulate the game force by the grip fraction. 
        # When grip is lost (grip_fract < 1.0), the force should feel lighter or different.
        # This is a simple implementation: lighten the force as grip is lost.
        # However, aligning torque often drops off, so if game_force is aligning torque, it might already do this.
        # But we can accentuate it.
        
        # We can also mix in a "Grip Factor" component.
        grip_mod = avg_grip # 0.0 to 1.0
        
        output_force = game_force * grip_mod
        
        # Add SoP effect (Yaw/G-force feel)
        # Using Lateral G from local acceleration
        lat_g = telemetry.mLocalAccel.x / 9.81
        sop_force = lat_g * self.sop_factor * 1000 # Scaling factor guess
        
        # Combine
        total_force = output_force + sop_force
        
        # Normalize (assuming max force is around 5000-10000 depending on car)
        max_force_ref = 4000.0 
        norm_force = total_force / max_force_ref
        
        # Clip
        norm_force = max(-1.0, min(1.0, norm_force))
        
        return norm_force

    def update_settings(self, gain, smoothing, sop_factor):
        self.gain = gain
        self.smoothing = smoothing
        self.sop_factor = sop_factor
