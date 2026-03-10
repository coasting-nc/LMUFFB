The proposed code change is a comprehensive and well-engineered solution to decouple the longitudinal steering weight effect from aerodynamic downforce. By shifting the input from tire load telemetry to velocity-derived G-force, it resolves the issue where high-downforce cars would experience artificially heavy steering at high speeds.

### **Analysis and Reasoning:**

1.  **User's Goal:** Replace tire-load-based steering weight modulation with G-force-based modulation to prevent aerodynamic downforce from interfering with steering feel, while maintaining support for high-dynamic-range braking (up to 5G) and mathematical stability for non-linear transformations.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch correctly implements the transition to G-force using `m_accel_z_smoothed`. It introduces "Domain Scaling," which is a sophisticated way to apply non-linear transformations (Cubic, Quadratic, Hermite) designed for a $[-1, 1]$ domain over a much larger physical range ($[-5G, 5G]$). This ensures that the S-curves provide smooth feedback without "folding back" or becoming non-monotonic at high G-loads.
    *   **Safety & Side Effects:** The implementation is safe. It reuses existing configuration variables (`m_long_load_effect`, etc.), ensuring that users do not lose their settings upon upgrading. It maintains the internal clamping logic to prevent dangerous FFB spikes during crashes. The decoupling from tire load is a significant physical improvement that eliminates noise from track bumps and aero-load.
    *   **Completeness:** The patch is exceptionally thorough. It includes:
        *   Core physics updates in `FFBEngine.cpp`.
        *   Clear UI renaming in `GuiLayer_Common.cpp` for user transparency.
        *   Updated tooltips in `Tooltips.h` explaining the new logic.
        *   A new dedicated test suite (`test_issue_325_longitudinal_g.cpp`) verifying the domain scaling math and aero-independence.
        *   Updates to existing regression tests.
        *   Proper project metadata updates (`VERSION` increment and `CHANGELOG_DEV.md`).
        *   Verbatim code review logs and a detailed implementation plan with design rationales.

3.  **Merge Assessment:**
    *   **Blocking:** None. The code is high-quality, mathematically sound, and follows all requested constraints.
    *   **Nitpicks:** None. The developer correctly identified the "domain scaling" requirement to fix the mathematical instability identified in earlier iterations.

The solution is highly maintainable and significantly improves the physical accuracy of the application.

### Final Rating: #Correct#
