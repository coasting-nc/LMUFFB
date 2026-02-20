### Code Review Iteration 2

This patch implements **Stage 3 of the FFB Strength Normalization** project, focusing on anchoring tactile haptics (road texture, slip rumble, and lockup vibrations) to a static mechanical load baseline rather than a dynamic peak.

#### Core Functionality
- **Latching Logic:** Correctly implements speed-based latching in `update_static_load_reference`.
- **Giannoulis Soft-Knee:** Mathematically sound implementation.
- **Smoothing:** Appropriate EMA smoothing added.
- **Bottoming Trigger:** Updated for consistency.

#### Safety & Side Effects
- **Regression Management:** Existing tests updated successfully.
- **Safety Clamp:** Added `(max)(0.0, ...)` clamp to load factor to ensure robustness.
- **Thread Safety:** Follows existing patterns.

#### Completeness
- Core logic and tests are complete.
- **Missing Metadata:** Still need to update `VERSION`, `src/Version.h`, and `Changelog`.
- **Missing Documentation:** Still need to finalize `plan_154.md`.

### Merge Assessment (Blocking vs. Non-Blocking)
- **Blocking:** Missing `VERSION` and `Changelog` updates.

### Final Rating: #Mostly Correct#
