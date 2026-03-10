# GitHub Issue #316: Safety fixes for FFB Spikes (3)

## Title: Add a new Safety section in the GUI to adjust safety features for FFB spikes

**Opened on:** Mar 9, 2026 by **coasting-nc**

### Description:
Add a new section in the GUI for "Safety".
The purpose is to make the new safety features adjustable.
Currently we are getting too many false positives from these safety mechanisms, where the FFB turns off unnecessarily, particularly in online sessions when other drivers join or leave the session, and this causes stuttering (frames lost) which in turns triggers the safety mechanism of turning off the FFB for 2 seconds.
We need to be able to customize the settings of these safety mechanisms, in other to find the sweet spots so that they prevent dangerous FFB spikes but don't turn off FFB unnecessarily.

### Settings to include in the GUI to adjust safety features:
- Include settings about slew rate limitation when in "safe mode" (eg. because stuttering / lost frames were detected).
- Make adjustable in the gui (Safety section): `SAFETY_SLEW_WINDOW` (mapped to `SAFETY_SLEW_FULL_SCALE_TIME_S`), `SAFETY_GAIN_REDUCTION`, `SAFETY_SMOOTHING_TAU`, `IMMEDIATE_SPIKE_THRESHOLD`.
- Add GUI Checkbox (also in Safety section) to turn off FFB disabling when stuttering is detected.
- A slider to determine the sensitivity to stuttering and the resulting FFB disabling.

### Additional Implementation Considerations:
- A gating or other complementary mechanism that, when stuttering is detected, it does not automatically disable FFB, but rather caps the slew rate. So we still have FFB in case of stuttering, but if spikes start to happen, they get reduced significantly (to the point of not being dangerous).

---
*This file is included for review and transparency purposes as part of the fix implementation.*
