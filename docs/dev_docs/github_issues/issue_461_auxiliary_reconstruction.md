# Replace the "LinearExtrapolator" (actually an interpolator) on the auxiliary channels with the HoltWintersFilter (configured for Zero Latency) #461

**User:**
The auxiliary channels (everything that is NOT the steering rack torque) currently use a `LinearExtrapolator` which is actually an interpolator (it introduces 10ms of delay to smooth out the 100Hz signal to 400Hz).

We should replace this with the `HoltWintersFilter` and configure it for "Zero Latency" mode (predictive extrapolation). This will remove 10ms of latency from:
- Gyro effect
- Road texture
- Stationary damping
- etc.

This is especially important for the Gyro effect as 10ms of delay can cause oscillations.

**Deliverables:**
- Replace `LinearExtrapolator` with `HoltWintersFilter` for all auxiliary channels in `FFBEngine`.
- Ensure they are configured in predictive mode (Zero Latency).
- Add a toggle in the UI to enable/disable this "Zero Latency" upsampling for auxiliary channels (default to ON).
- Update tests to reflect the changed timing/values.
