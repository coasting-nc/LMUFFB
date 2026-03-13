# Verify use of wrong load fallback in calculate_sop_lateral #309

Open
coasting-nc opened this issue on Mar 9, 2026

## Description

Verify issue:

In calculate_sop_lateral, we directly use calculate_kinematic_load if (ctx.frame_warn_load) . However, that is the second-choice fallback. We should first attempt using the first choice fallback, approximate_load(), if (fl.mSuspForce > MIN_VALID_SUSP_FORCE), like we do in calculate_force.

Approximate_load is the first choice fallback because it is supposedly more precise than calculate_kinematic_load, if mSuspForce is available (which generally it is in LMU).

Verify if this is an issue, and in case fix it, by removing all calls to calculate_kinematic_load, since we don't actually need them.
It seems we don't actually need calculate_kinematic_load at all in any parts of the code, so the simplest solution is to remove all calls to it, instead of having a more compliated logic with conditions for choosing between two fallbacks.

## Context

When mTireLoad telemetry is missing (common in LMU cars), the FFB engine must use a fallback to estimate tire loads for physics calculations (Lateral Load effect, Grip estimation, etc.). The current implementation in calculate_sop_lateral skips the "Approximate" fallback (derived from mSuspForce) and uses directly the "Kinematic" fallback (physics estimation). This results in less accurate FFB for cars that have valid suspension data but blocked tire load data.

Additionally, there are inconsistencies in the code regarding the condition to use to determine whether we should use a fallback estimate for tire load. In some places we use "if (ctx.frame_warn_load)", in others we use "if (m_missing_load_frames > MISSING_LOAD_WARN_THRESHOLD)".

## TODO

* Review the code to confirm this issue, and update relevant documentation discussing it (eg. implementation plan).
* Write new regression tests that will cover all the changes we are about to make (use of correct fallback, use of the correct conditions to check when to use the first-choice fallback approximate_load.
* Remove all calls to calculate_kinematic_load
* Remove all logic that decides whether we have suspension data (eg. (fl.mSuspForce > MIN_VALID_SUSP_FORCE)) in order to use calculate_kinematic_load
* Decide the correct and best condition to decide when to use the fallback estimate for tire load (approximate_load), and use that consistently throughout all the code.
* Updates all tests that assumed the use of the wrong fallback. Update all tests that assumed different conditions for using the tire load approximation fallback (if any). Fix any other failing test.
