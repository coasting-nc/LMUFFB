## 2025-05-15 - [FFB Safety & Feature Decoupling]
**Issue:** [Scrub Drag Dependency & FFB Hang on Disconnect]
**Root Cause:** Scrub Drag logic was placed inside a conditional block checking if Road Texture was enabled. FFB hang occurred because the hardware UpdateForce call was only executed if the game connection was active.
**Prevention:** Always place hardware-communcation calls outside conditional connection checks, ensuring a default 'safe' state (e.g., zero force) is sent if the condition is not met. Decouple independent physics effects into their own logic blocks.
