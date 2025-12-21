## Question 1

Here is the full codebase and documentation of a  force feedback (FFB) app. Your task is to investigate an issue. 

An user tested the app and reported that there was a constant vibration that he want to eliminate or mask. This vibration might be coming from the Steering Shaft Torque given by the game Le Mans Ultimate (LMU). This vibration might be due to tyre flat-spots. The user identified the frequency of the vibration as being between 10 and 60 Hz.

The user disabled many settings / sliders in our LMUFFB app to determine what was causing the vibration, and it seems it was caused by the Steering Shaft Torque signal itself, and not by any effect our app produces.

The user tried to mask the vibration by various means (eg. FFB signal equalizer in the Moza device driver, or adding smoothing), but he said that the things he tried also masked other useful details from the force feedback in addition to the vibration. Therefore, we want to find out if we can improve the LMUFFB app to more effectively mask out or eliminate this vibration without reducing, compromising or affecting any other detail of the force feedback.

Your task is to investigate the following:

* verify whether  we might still be causing some vibration from our app, even when we have only the Steering Torque Shaft force enabled and everything else disabled.
* if we are not causing this vibration, and the game signal itself is the cause, determine ways in which we can "mask" or eliminate this vibration. What are the possible solutions for this?

	* Can we add a "counter" frequency signal that cancels out that vibration? This is assuming that that vibration is at a constant frequency.

	* Can we add some tools to "troubleshoot" and identify the exact frequency of this vibration in the steering shaft torque? Like some spectrum analyzer plot, or something else?

	* Can we add some form of damping or smoothing only to the steering shaft torque? Can we do this without adding any significant latency (that is, less than 15 milliseconds)?

	* Are there other recommended solutions?

----
