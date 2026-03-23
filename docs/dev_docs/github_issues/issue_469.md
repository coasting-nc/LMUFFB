# DSP Mathematical Upgrades & Time-Aware Telemetry Upsampling #469

**Opened by coasting-nc on Mar 22, 2026**

## Description

DSP Mathematical Upgrades & Time-Aware Telemetry Upsampling

Follow this implementation plan:
docs\dev_docs\implementation_plans\plan_dsp_math_upgrades(patch 3).md

## Context

Following the structural categorization of auxiliary telemetry channels in Patch 2 (Issue #466), this patch (Patch 3) implements critical mathematical corrections to the HoltWintersFilter and the channel tuning parameters. Based on deep research into vehicle dynamics DSP, the current implementation is vulnerable to telemetry jitter (variable 100Hz frame times) and catastrophic extrapolation overshoot during dropped frames. This patch upgrades the filter to be "Time-Aware", introduces "Trend Damping" to safely arrest runaway extrapolation, and corrects the Alpha/Beta tuning parameters to respect Nyquist-Shannon sampling limits.

## Design Rationale

Why upgrade the math?
1. Time-Awareness: Game telemetry does not arrive at a perfect 10ms cadence due to OS scheduling and game engine rendering fluctuations. Dividing a change in signal by a hardcoded 0.01s when the actual elapsed time was 0.015s calculates an artificially steep, incorrect derivative. The filter must measure the actual time between frames.
2. Trend Damping: If a telemetry frame drops while the driver is rapidly counter-steering, standard Holt-Winters will extrapolate that high velocity into infinity, causing a violent FFB spike. We must decay the trend to zero during starvation events so the signal safely plateaus.
3. Nyquist Limits: Extrapolating a 30Hz tire vibration sampled at 100Hz is mathematically volatile. We must dynamically force the trend tracker (Beta) to 0.0 for Group 2 signals when "Zero Latency" is selected to prevent harmonic ringing and metallic grinding noises.
