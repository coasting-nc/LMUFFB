# Add one or more plots to diagnose how good the tire load estimate is (approximate_load) #342

**State:** Open
**Opened by:** coasting-nc
**Date:** Mar 13, 2026

## Description

Add one or more plots to the Log Analyser showing how good the load value returned by approximate_load function is compared to read tire load data from the game. This is a diagnostics that could be used in car in LMU or rFactor 2 that provide actual tire load data. The purpose of these plots is to diagnose if we need to further improve approximate_load or not. Plots could include both correlation and discrepancy or difference with the real values; plots could include plotting both the approximate load and the real one over time, and check the similarity or discrepancies. We will need to determine if there are also other types of plots that are more effective or complementary in determining the correctness, and efficacy of the tire load estimate for the purposes and contexts in which we need the estimate (for instance, for some use case scenarios, it might not be necessary to actually have a precise estimate of the load value, but only to precisely capture the changes in load values).

Also review and investigate if the current code for approximate_load is adequate and the best we can do. The current implementation is simply "mSuspForce + 300.0".

Update the documentation (code comments) of approximate_load with our findings.
