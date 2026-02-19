## 2026-02-19 - [Fix Tire Load Normalization (#120)]
Issue: Hardcoded tire load constants (4000N, 8000N) caused clipping and false bottoming in high-downforce cars.
Root Cause: FFB engine effects were designed around GT3 load levels without accounting for Hypercar/LMP2 aero loads.
Prevention: Use class-aware dynamic thresholds (multiples of `m_auto_peak_load`) instead of absolute Newton values.
