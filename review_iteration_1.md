The proposed code change addresses the reported issue by fundamentally changing how the Soft Lock force is scaled within the Force Feedback (FFB) engine. By moving the Soft Lock from the "Structural" group (which is normalized by the dynamic session peak torque) to the "Texture/Absolute Nm" group (which is scaled by the wheelbase's maximum torque), the fix ensures that the physical stop provided by the soft lock is consistent and strong, regardless of how high the steering forces during driving have been.

### Analysis and Reasoning:

1.  **User's Goal:** The user wants the Soft Lock force to be significantly stronger and to work consistently, including when the car is stationary.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch directly solves the "weakness" problem. In the previous implementation, if the session peak torque was high (e.g., due to a high-downforce car or a crash), the soft lock (calculated in Nm) was divided by this peak, often resulting in a very faint force. By moving it to the absolute Nm group, it is now divided by the hardware's maximum torque (`m_wheelbase_max_nm`), ensuring it always hits the intended physical target. The provided regression test confirms this logic.
    *   **Safety & Side Effects:** The change is safe. The soft lock is now correctly exempted from the `gain_reduction_factor` (Torque Drop), which is intended for tire physics effects (like traction loss) and shouldn't impact a physical steering stop. The final output is still subject to the engine's global clamping logic.
    *   **Completeness:** The patch updates the summation logic in `FFBEngine.cpp`, increments the project version, and adds a comprehensive regression test that specifically verifies the fix for the reported normalization issue.

3.  **Merge Assessment:**
    *   **Blocking:** None. The logic is sound and the implementation is clean.
    *   **Nitpicks:**
        *   The patch includes the `implementation_plan.md` and `review_iteration_1.md` files. While useful for the agent's workflow, these would typically be excluded from a final pull request to the main repository.
        *   The `implementation_plan.md` promised an update to `src/Version.h`, which is missing from the patch (only the `VERSION` file was updated). If the project does not auto-generate headers from the version file, the UI version might remain out of sync.

### Final Rating: #Correct#
