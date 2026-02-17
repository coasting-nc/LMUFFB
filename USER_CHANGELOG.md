# lmuFFB App Version Releases by ErwinMoss

This document contains all version release posts by ErwinMoss from the [url=https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/]lmuFFB App thread[/url] on Le Mans Ultimate Community.

[b]Note:[/b] This file uses BBCode formatting for easy copy-paste to forums.

[size=5][b]February 15, 2026[/b][/size]
[b]Version 0.7.48 - Safety & Reliability Update[/b]

[b]New release[/b] (0.7.48): https://github.com/coasting-nc/LMUFFB/releases

[b]Fixed[/b]
[list]
[*][b]FFB After Finish Line[/b]: Resolved a major community request where Force Feedback would cut out immediately upon crossing the finish line. FFB now remains active during cool-down laps and after a DNF, provided you are still in control of the car. (#126)
[*][b]Wheel Jolt Protection[/b]: Implemented a new [b]Safety Slew Rate Limiter[/b]. This feature prevents dangerous, violent steering wheel jolts that could occur during session transitions or when AI takes control of the car. It ensures any sudden force changes are rounded off smoothly. (#79)
[*][b]Consistent Hardware Zeroing[/b]: Improved safety by ensuring the wheel always returns to center when the game is disconnected or the app is inactive, preventing the wheel from becoming "stuck" at full torque.
[/list]

[b]Improved[/b]
[list]
[*][b]Reduced Straight-Line Jitter[/b]: Increased the activation threshold for Slide Texture from 0.5 to 1.5 m/s. This surgically removes annoying vibration artifacts on straights caused by tire toe-in, without losing actual slide feedback.
[*][b]Expanded Road Texture Headroom[/b]: Increased the texture load cap to match the brake cap, allowing for much more detailed road feel on high-downforce cars.
[*][b]Independent Scrub Drag[/b]: The Scrub Drag effect is now decoupled from the main Road Texture toggle, giving users more granular control over their haptic configuration.
[/list]

---

[size=5][b]February 14, 2026[/b][/size]
[b]Version 0.7.47 - Advanced Physics Smoothing[/b]

[b]New release[/b] (0.7.47): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]Advanced Physics Smoothing[/b]: This update brings high-fidelity signal processing to the Force Feedback engine, removing "grainy" road noise while keeping your reaction times instant.
[*][b]Adaptive Grip Filter[/b]: Implemented a non-linear filter for tire grip. It provides heavy smoothing when you are driving steadily to remove jitter, but instantly "opens up" during a slide to ensure zero-latency feedback when you lose the car.
[*][b]Dynamic Weight Damping[/b]: Added a suspension-modeling filter to the steering weight. Steering modulation now feels more organic and weighted, simulating how real-world suspension dampers absorb high-frequency road vibrations.
[*][b]Precision Tuning[/b]: New sliders for "Weight Smoothing" and "Grip Smoothing" have been added to the Tuning window, allowing you to match the smoothness to your specific wheelbase hardware.
[/list]

---

[size=5][b]February 14, 2026[/b][/size]
[b]Version 0.7.46 - Dynamic Weight & Load-Weighted Grip[/b]

[b]New release[/b] (0.7.46): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]Dynamic Steering Weight[/b]: Steering now becomes physically heavier under heavy braking as the weight transfers to the front tires, and lightens under hard acceleration. This provides a critical cue for pitch-based weight transfer.
[*][b]Load-Weighted Grip[/b]: Refined how the app calculates overall grip. It now prioritizes the feel of the most loaded tire (the outside tire in a corner), giving you a much more accurate sense of when the car is about to wash out.
[/list]

---

[size=5][b]February 14, 2026[/b][/size]
[b]Version 0.7.45 - Internal Refactoring & Reliability[/b]

[b]New release[/b] (0.7.45): https://github.com/coasting-nc/LMUFFB/releases

[b]Refactored[/b]
[list]
[*][b]Code Base Cleanup[/b]: Streamlined the car detection systems to be more modular and robust. This improves app stability and makes it faster for us to add support for new cars in the future.
[*][b]Consistent Force Calculations[/b]: Refined how different car categories like LMP2 (Restricted vs Unrestricted) are handled internally to ensure consistent feel across all series.
[/list]

---

[size=5][b]February 14, 2026[/b][/size]
[b]Version 0.7.44 - Robust Car Detection[/b]

[b]New release[/b] (0.7.44): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved[/b]
[list]
[*][b]Smarter Car Identification[/b]: The app is now much better at detecting your car class automatically. It now handles case-insensitive names and uses a smart keyword fallback (e.g., detecting a "Hypercar" from the car name even if the category is unknown).
[*][b]Automatic Load Seeding[/b]: Improved the initial force weighting for various car classes (LMP2, LMP3, GT3, GTE, and Hypercars) to provide a better out-of-the-box experience.
[*][b]Broad Format Compatibility[/b]: Enhanced detection logic to support various mod and DLC naming conventions across different leagues (WEC, ELMS, etc.).
[/list]

---

[size=5][b]February 14, 2026[/b][/size]
[b]Version 0.7.43 - Automatic Load Normalization[/b]

[b]New release[/b] (0.7.43): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]Automatic Tire Load Normalization[/b]: This major physics update ensures that all car classes feel rich and detailed. The app now automatically detects which car you are driving and scales the road textures and haptic effects accordingly.
[*][b]Class-Based Seeding[/b]: Hypercars and Prototypes now immediately feel heavy and textured on session start (using a 9500N reference), while GT3s maintain their optimized 4800N baseline.
[*][b]Dynamic Peak Adaptation[/b]: The engine continuously monitors your actual tire loads. If your setup or aero creates higher forces than expected, the app instantly adapts to prevent "numb" or clipped feedback.
[*][b]Improved High-Speed Fidelity[/b]: Hypercars and LMP2s now maintain 100% of their road detail even at extreme speeds on the Mulsanne straight, where previously the forces would have been clipped by the hardcoded 4000N limit.
[/list]

[b]Improved[/b]
[list]
[*][b]Better Log Diagnostics[/b]: The telemetry logger now records the dynamic load reference, making it easier for advanced users to analyze how the normalization is behaving during a stint.
[/list]

---

[size=5][b]February 12, 2026[/b][/size]
[b]Version 0.7.35 - UI Stability Update[/b]

[b]New release[/b] (0.7.35): https://github.com/coasting-nc/LMUFFB/releases

[b]Fixed[/b]
[list]
[*][b]ImGui ID Conflict[/b]: Resolved the error "2 visible wheels with conflicting ID" that occurred when multiple FFB devices or presets had identical names. The app now correctly assigns unique internal identifiers to all selectable items. (#70)
[/list]

---

[size=5][b]February 12, 2026[/b][/size]
[b]Version 0.7.34 - FFB Safety Update[/b]

[b]New release[/b] (0.7.34): https://github.com/coasting-nc/LMUFFB/releases

[b]Fixed[/b]
[list]
[*][b]FFB Safety[/b]: Implemented automatic FFB muting when the car is no longer under player control (AI/Remote) or when the session has finished. This prevents violent "finish line jolts" during race weekends. (#79)
[/list]

---

[size=5][b]February 12, 2026[/b][/size]
[b]Version 0.7.33 - Preset Handling Fix & Cleanup[/b]

[b]New release[/b] (0.7.33): https://github.com/coasting-nc/LMUFFB/releases

[b]Fixed[/b]
[list]
[*][b]Preset Selection Stability[/b]: Fixed an issue where changing any setting would immediately switch the selected preset to "Custom." This now correctly maintains your current preset selection, making it much easier to fine-tune and update existing profiles.
[*][b]"Save Current Config" Restoration[/b]: The "Save Current Config" button now correctly updates the active user preset, allowing you to persist your changes with a single click.
[/list]

[b]Improved[/b]
[list]
[*][b]Dirty State Indication[/b]: Added a clearer dirty indicator (*) next to the preset name when settings have been modified from their saved state.
[*][b]Code Cleanup[/b]: Significantly simplified the internal logic for detecting modified settings, making the app more robust and easier to maintain as we add new FFB parameters.
[/list]

---

[size=5][b]February 11, 2026[/b][/size]
[b]Version 0.7.28 - Tooltip Restoration & UX Update[/b]

[b]New release[/b] (0.7.28): https://github.com/coasting-nc/LMUFFB/releases

[b]Fixed[/b]
[list]
[*][b]Tooltip Restoration[/b]: We've restored over 40 missing tooltips that were accidentally removed during recent updates. Every setting now has its detailed explanation and tuning guide back where it belongs.
[*][b]Modern Feature Documentation[/b]: Added new descriptions for newer features like Slope Detection stability, Lockup Prediction, and the dynamic signal filters.
[*][b]100% Coverage[/b]: Verified that every single button and interactive element in the app now has a helpful tooltip. No more guessing what a button does!
[/list]

[b]Improved[/b]
[list]
[*][b]Easier Discovery[/b]: Tooltips now appear when you hover over the [b]Parameter Label[/b] as well as the slider/checkbox. This makes it much easier to quickly see what a setting does without having to aim precisely for the input field.
[*][b]Guided Tuning[/b]: Standardized the "Fine Tune" instructions (Arrow Keys / Ctrl+Click) across the entire interface.
[/list]

---

[size=5][b]February 11, 2026[/b][/size]
[b]Version 0.7.27 - Security & Reputation Update[/b]

[b]New release[/b] (0.7.27): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved Security[/b]
[list]
[*][b]Executable Metadata[/b]: We've added proper version information and company metadata to the application executable. This helps major antivirus software identify lmuFFB as a legitimate application rather than an "unknown" file.
[*][b]Heuristic Reduction[/b]: Replaced internal process-access calls with safer window-based checks to avoid triggering false-positive alerts on some security software.
[*][b]Hardened Build[/b]: Enabled advanced security flags (ASLR, DEP) to protect the application against memory-based exploits.
[/list]

---

[size=5][b]February 11, 2026[/b][/size]
[b]Version 0.7.25 - vJoy Support Removal[/b]

[b]New release[/b] (0.7.25): https://github.com/coasting-nc/LMUFFB/releases

[b]Removed[/b]
[list]
[*][b]vJoy Support Removed[/b]: We have completely removed vJoy support and its dynamic library loading mechanism. This eliminates a common "runtime library loading" heuristic that some antivirus software flagged as suspicious.
[*][b]Simplified Setup[/b]: As the app now exclusively uses DirectInput for FFB output, removing the legacy vJoy code simplifies the internal architecture and reduces the application footprint.
[/list]

---

[size=5][b]February 11, 2026[/b][/size]
[b]Version 0.7.24 - Privacy & Security Update[/b]

[b]New release[/b] (0.7.24): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved Security[/b]
[list]
[*][b]Disabled Clipboard Access[/b]: Modified the application build configuration to completely disable clipboard access in the user interface. This removes a common heuristic trigger for antivirus software. Copy/Paste functionality in text fields is disabled.
[*][b]Removed Window Title Tracking[/b]: Removed the internal logic that tracked the active window title for focus management. This behavior was sometimes flagged as "spyware-like" activity monitoring. Focus management now relies solely on standard window activation events.
[/list]

---

[size=5][b]February 11, 2026[/b][/size]
[b]Version 0.7.23 - Reputational Safety Update[/b]

[b]New release[/b] (0.7.23): https://github.com/coasting-nc/LMUFFB/releases

[b]Removed[/b]
[list]
[*][b]Screenshot Feature Removed[/b]: We have removed the "Save Screenshot" feature to prevent false-positive flags from Windows Defender and VirusTotal. The use of certain screen capture APIs (necessary for the multi-window composite screenshot) is a common heuristic for spyware. Since the feature is no longer critical for development, removing it ensures a safer and more reputable experience for all users.
[*][b]Cleaned Internal Codebase[/b]: Removed all dependencies on the [code]stb_image_write.h[/code] library and deleted the associated platform capture logic.
[/list]

---

[size=5][b]February 10, 2026[/b][/size]
[b]Version 0.7.21 - Understeer Math Overhaul[/b]

[b]New release[/b] (0.7.21): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved[/b]
[list]
[*][b]Mathematical Stabilization (S-Curve Ramp)[/b]: We've replaced the old "binary switch" for grip detection with a modern S-Curve (Smoothstep) ramp. This means the transition from straight-line driving to a full cornering understeer cue is now completely seamless, with no sudden shifts or "notchy" feeling in the force.
[*][b]Advanced Singularity Protection[/b]: The internal math now has even better guards against extreme telemetry values. Even if the game sends "noisy" data during a curb-strike or accident, the understeer effect remains smooth and physically realistic.
[*][b]Better Graph Clarity[/b]: The live physics graph now shows a much cleaner and more stable slope calculation, making it easier to see exactly when your tires are losing grip.
[/list]

---

[size=5][b]February 10, 2026[/b][/size]
[b]Version 0.7.20 - Slope Stability Hardening[/b]

[b]New release[/b] (0.7.20): https://github.com/coasting-nc/LMUFFB/releases

[b]Fixed[/b]
[list]
[*][b]FFB "Explosion" Prevention[/b]: Fixed a critical issue where the steering wheel could suddenly jerk or vibrate violently during slow corners. This was caused by a "division-by-zero" style mathematical explosion in the grip detection code. We've added safety "clamps" to ensure these values always stay within physically realistic limits.
[*][b]Smoother Understeer Cues[/b]: Refined the way the understeer effect blends in at low speeds. The "Confidence" system is now smarter about rejecting noise, resulting in a much more stable and analog feel through the wheel.
[/list]

[b]Improved[/b]
[list]
[*][b]Hardened Safety Suite[/b]: We've added a comprehensive "Stress Test" suite to our internal diagnostics. This puts the FFB engine through thousands of frames of synthetic "bad data" (spikes, noise, and jitter) to ensure the feedback remains safe and stable for your hardware even in extreme conditions.
[/list]

---

[size=5][b]February 9, 2026[/b][/size]
[b]Version 0.7.18 - Batch Log Analysis[/b]

[b]New release[/b] (0.7.18): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]Batch Log Analysis[/b]: You can now analyze an entire directory of log files at once. The new [code]batch[/code] command runs all diagnostics (session info, slope analysis, plots, and reports) for every log file in the folder.
[*][b]One-Click Diagnostics[/b]: Perfect for analyzing an entire race weekend or comparing multiple car/track combinations. All results are neatly organized into a dedicated results directory.
[/list]

[b]Improved[/b]
[list]
[*][b]Diagnostics Workflow[/b]: The Log Analyzer tool is now even easier to use for deep dives into FFB performance and telemetry stability.
[/list]

---

[size=5][b]February 7, 2026[/b][/size]
[b]Version 0.7.17 - Test Suite Consolidation[/b]

[b]New release[/b] (0.7.17): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved[/b]
[list]
[*][b]Unified Testing Framework[/b]: Completed the migration of all internal diagnostic tests to a new automated registration system. This ensures that 100% of our physics and platform tests are executed every time we build the app, preventing "silent regressions" in complex FFB effects.
[*][b]Cleaner Internal Codebase[/b]: Standardized the way we verify FFB integrity across Windows and Linux, removing redundant internal code and simplifying the development workflow.
[/list]

---

[size=5][b]February 5, 2026[/b][/size]
[b]Version 0.7.12 - Preset & Telemetry Versioning[/b]

[b]New release[/b] (0.7.12): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]Preset Version Tracking[/b]: Your saved presets now store the version of lmuFFB they were created with. This helps ensure compatibility as the physics engine evolves.
[*][b]Telemetry Versioning[/b]: Telemetry logs now include the app version in the file header, making it easier to analyze logs using the Log Analyzer tool.
[/list]

[b]Fixed[/b]
[list]
[*][b]Preset Loading Fix[/b]: Resolved an issue where the Master Gain setting was not correctly restored when switching between user presets.
[*][b]Auto-Migration[/b]: Any old presets from previous versions are automatically updated to the current format on first load.
[/list]

---

[size=5][b]February 5, 2026[/b][/size]
[b]Version 0.7.11 - Advanced Threshold Mapping[/b]

[b]New release[/b] (0.7.11): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved[/b]
[list]
[*][b]New Slope Threshold System[/b]: Replaced the old "Sensitivity" slider with a more powerful [b]Min/Max Threshold[/b] system. You can now define exactly where the understeer effect begins (Min) and where it reaches full strength (Max).
[*][b]Predictable Feedback[/b]: The new linear mapping provides a much more intuitive feeling through the wheel. As the tire curve slope drops between your two thresholds, the FFB lightens in a perfectly proportional way.
[*][b]Seamless Migration[/b]: Your existing "Sensitivity" settings are automatically converted to the new threshold system on first launch, so your current FFB feel is preserved while unlocking the new tuning capabilities.
[/list]

[b]Fixed[/b]
[list]
[*][b]Physics Stability[/b]: Hardened the slope detection math against edge cases and tiny value fluctuations, ensuring a smoother understeer cue during aggressive steering maneuvers.
[/list]

---
 
[size=5][b]February 4, 2026[/b][/size]
[b]Version 0.7.10 - Python Log Analyzer[/b]

[b]New release[/b] (0.7.10): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]lmuFFB Log Analyzer Tool[/b]: A powerful new standalone tool for diagnosing Force Feedback issues. It processes your telemetry logs to find stability problems in the slope detection algorithm and provides deep insight into tire behavior.
[*][b]Visual Diagnostics[/b]: Automatically generate plots including time-series data, tire curves (Slip Angle vs Grip), and statistical distributions of your driving data.
[*][b]Automated Performance Reports[/b]: Generate full text reports that highlight "sticky" understeer, unstable slope calculations, and frequent grip-floor hits.
[*][b]Easy Command-Line Workflow[/b]: Simple commands like [code]analyze[/code], [code]plots[/code], and [code]report[/code] make it easy to get professional-grade diagnostics in seconds.
[/list]

---
 
[size=5][b]February 4, 2026[/b][/size]
[b]Version 0.7.9 - Telemetry Logger[/b]

[b]New release[/b] (0.7.9): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]High-Frequency Telemetry Logger[/b]: You can now record your FFB data to `.csv` files for diagnostics and tuning. The logger runs in the background at 100Hz (decimated from 400Hz) and captures every nuance of the physics engine, including hidden values like slip-derivatives and grip factors.
[*][b]User Markers[/b]: Press the "MARKER" button during a slide or interesting moment to tag it in the log file for easy searching later.
[*][b]Auto-Logging Mode[/b]: Enable "Auto-Start on Session" in Advanced Settings to have lmuFFB automatically record every time you leave the pits.
[*][b]Smart Filenaming[/b]: Logs are automatically sorted by time, car, and track name (e.g., `lmuffb_log_..._Ferrari_488_Spa.csv`).
[/list]

[b]Improved[/b]
[list]
[*][b]Stability & Performance[/b]: The logger uses a dedicated worker thread, ensuring zero impact on your FFB feel or frame timing.
[/list]

---
 
[size=5][b]February 3, 2026[/b][/size]
[b]Version 0.7.5 - Test Infrastructure Refactoring[/b]

[b]New release[/b] (0.7.5): https://github.com/coasting-nc/LMUFFB/releases

[b]Internal Changes[/b]
[list]
[*][b]Codebase Modularization[/b]: Refactored the internal test suite, splitting the monolithic `test_ffb_engine.cpp` into 9 modular files. This improves maintainability and development speed for future updates. No user-facing changes to FFB logic or physics.
[/list]

---

[size=5][b]February 3, 2026[/b][/size]
[b]Version 0.7.3 - Slope Stability Fixes[/b]

[b]New release[/b] (0.7.3): https://github.com/coasting-nc/LMUFFB/releases

[b]Fixed[/b]
[list]
[*][b]Understeer Stability Overhaul[/b]: Resolved issues with "sticky" understeer on straights and random FFB jolts when using Slope Detection. The algorithm now intelligently decays the slip-slope toward zero when not cornering and uses a configurable confidence gate based on steering speed.
[*][b]Oscillation Prevention[/b]: Introduced a new "Alpha Threshold" parameter to prevent the slope calculation from reacting to tiny, noisy telemetry fluctuations during straight-line driving.
[/list]

[b]Added[/b]
[list]
[*][b]Advanced Stability Controls[/b]: Added new Sliders to the GUI for fine-tuning Slope Detection:
    [list]
    [*][b]Alpha Threshold[/b]: Controls the sensitivity to steering changes.
    [*][b]Decay Rate[/b]: Controls how fast the effect fades after a corner.
    [*][b]Confidence Gate[/b]: Automatically smooths out jolts during low-intensity maneuvers.
    [/list]
[*][b]Stability Verification Suite[/b]: Added 7 new automated tests to ensure slope detection remains stable across various driving scenarios (corner-to-straight transitions, noise rejection, and persistence).
[/list]

---

[size=5][b]February 3, 2026[/b][/size]
[b]Version 0.7.2 - Smooth Transitions[/b]

[b]New release[/b] (0.7.2): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved[/b]
[list]
[*][b]Smoother Low-Speed FFB[/b]: Upgraded the low-speed "fade-in" logic from linear to a smooth S-curve (Smoothstep). This results in a more natural transition as you begin moving from a standstill or drive through the pit lane, eliminating "angular" changes in steering weight.
[/list]

[b]Added[/b]
[list]
[*][b]Enhanced Reliability Suite[/b]: Added a dedicated suite of physics verification tests to ensure the new Smoothstep transitions are mathematically perfect and don't introduce noise while stationary.
[/list]

---

[size=5][b]February 2, 2026[/b][/size]
[b]Version v0.7.1 - Slope Detection Stability & UI[/b]

[b]New release[/b] (v0.7.1) https://github.com/coasting-nc/LMUFFB/releases

This update addresses several stability issues discovered in the initial Slope Detection release (v0.7.0) and improves the user interface for better tuning.

[b]Fixes & Improvements:[/b]
[list]
[*] [b]Oscillation Fix[/b]: Lateral G Boost (Slide) is now automatically disabled when Slope Detection is ON. This prevents a feedback loop caused by different grip calculation methods between front and rear axles.
[*] [b]UI Tooltips[/b]: Added detailed tooltips for Slope Detection parameters with recommended values for Direct Drive, Belt, and Gear-driven wheels.
[*] [b]Slope Graph[/b]: Added a new live graph in the "Internal Physics" header to monitor the slope (dG/dAlpha) in real-time. This helps in visual tuning and understanding tire behavior.
[*] [b]Smoother Transitions[/b]: Updated default settings for a less aggressive response out-of-the-box (Sensitivity: 0.5, Smoothing Tau: 0.04s).
[*] [b]Diagnostic Improvements[/b]: Added live slope values to the diagnostic snapshot for easier troubleshooting.
[/list]

---

[size=5][b]February 1, 2026[/b][/size]
[b]Version v0.7.0 - Dynamic Slope Detection[/b]

[b]New release[/b] (v0.7.0) https://github.com/coasting-nc/LMUFFB/releases

This is a major update introducing [b]Dynamic Slope Detection[/b] for tire grip estimation.

[b]Major Features:[/b]
[list]
[*] [b]Dynamic Understeer Feel[/b]: Instead of using a fixed "Optimal Slip Angle", the app now monitors the slope of the tire curve (Lateral G vs Slip Angle) in real-time. This allows lmuFFB to detect exactly when the tire starts to saturate, providing a much more precise and natural Understeer feel that adapts to different cars, tires, and track conditions.
[*] [b]Advanced Signal Processing[/b]: Uses Savitzky-Golay filtering to calculate clean derivatives from noisy telemetry data, ensuring smooth and stable feedback.
[*] [b]Safety Hardening[/b]: Added comprehensive regression tests to ensure all settings are correctly synchronized and saved across sessions.
[/list]

[b]Tuning Tip:[/b] You can find the new Slope Detection settings under the "Grip Estimation" section. For most users, the default settings will provide a significantly improved Understeer cue out-of-the-box.

---

[size=5][b]Post #1 - July 24, 2025[/b][/size]
[b]Initial Thread Creation[/b]

lmuFFB early version released: https://github.com/coasting-nc/LMUFFB/releases/

Some of the FFB formulas might need refinements. Adjust the FFB configuration options in lmuFFB to see which of the currently supported effects you prefer.

Post your screenshots of the Graphs, the main lmuFFB window and the console here if you can (use the Save Screenshot button to capture them all).

Your testing and feedback is greatly appreciated!

Project homepage: https://github.com/coasting-nc/LMUFFB
Download latest version: https://github.com/coasting-nc/LMUFFB/releases/

---

[size=5][b]Post #10 - December 6, 2025[/b][/size]
[b]Early Version Release[/b]

I've created an early version of LMUFFB: https://github.com/coasting-nc/LMUFFB/releases

It needs vJoy + the rF2 shared-memory plugin. I haven't fully tested it yet (sorting my wheel), so please give it a spin and drop any bugs/feedback here or on GitHub. Thanks!

---

[size=5][b]Post #41 - December 9, 2025[/b][/size]
[b]Version v0.4.0b - LMU 1.2 Support Added[/b]

Support for LMU 1.2 added, give it a try: https://github.com/coasting-nc/LMUFFB/releases/tag/v0.4.0b

Remember that this is an experimental version, for safety you should first lower you wheel max strength from your device driver configurator, since the app might cause strong force spikes and oscillations. Eg. 10 % max force for DDs, 20-30 % for gear/belt driven wheels.

The app probably still needs refinements to the FFB formulas. Adjust the FFB configuration options in lmuFFB to see if you manage to get a workable FFB signal.
Post your screenshots of the Troubleshooting Graphs window and the main lmuFFB window here if you can.

Your testing and feedback is greatly appreciated!

---

[size=5][b]December 11, 2025[/b][/size]
[b]Version v0.4.4b[/b]

[b]New release[/b]: https://github.com/coasting-nc/LMUFFB/releases/tag/v0.4.4b

Updates to install instructions:
[list]
[*]vJoy might not be necessary, ignore the warning. Just set the ingame FFB to 0% and off.
[*]If FFB feels "backwards" or "inverted" (wheel pushes away from center instead of pulling toward it), check or uncheck the "Invert FFB" checkbox in the lmuFFB GUI to reverse the force direction
[*]See README.txt for the rest of the instructions
[/list]

Known issues:
You will see warnings about Missing Tire Load and Missing Grip Fraction data detected. This is EXPECTED and NOT a bug in lmuFFB. This is a BUG IN LMU 1.2 - the game is currently returning ZERO (0) for all tire load and grip fraction values, even though the shared memory interface includes these fields.

---

[size=5][b]December 15, 2025[/b][/size]
[b]Version v0.4.15+ - Major Stability Fix[/b]

[b]New release[/b] (v0.4.15+) https://github.com/coasting-nc/LMUFFB/releases

This should fix the FFB instability issues (eg. intermittent inverse FFB).
It also removes the messages and instructions about vJoy and the rFactor 2 shared memory dll, since vJoy is no longer needed, and there are no problems if you keep the shared memory dll in the game.

Full changelogs: https://github.com/coasting-nc/LMUFFB/blob/main/CHANGELOG_DEV.md
 
---

[size=5][b]December 15, 2025[/b][/size]
[b]Version v0.4.16 - Yaw Kick Effect[/b]

[b]New release[/b] (v0.4.16) https://github.com/coasting-nc/LMUFFB/releases

This introduces a new Yaw Kick effect within the Seat of Pants effect, that tells when the rear starts to step out. This should help catch early slides.

---

[size=5][b]December 16, 2025[/b][/size]
[b]Version v0.4.18 - Gyroscopic Damping[/b]

[b]New release[/b] (v0.4.18) https://github.com/coasting-nc/LMUFFB/releases

Added Gyroscopic Damping (stabilization effect to prevent "tank slappers" during drifts), and smoothing to the Yaw Kick effect.

---

[size=5][b]December 16, 2025[/b][/size]
[b]Version v0.4.19 - Coordinate System Fix[/b]

[b]New release[/b] (v0.4.19) https://github.com/coasting-nc/LMUFFB/releases

This fixes a few bugs on the use of rF2 / LMU coordinate system. This should fix the issue of the broken slide rumble.

---

[size=5][b]December 19, 2025[/b][/size]
[b]Version v0.4.20 - Force Direction Fixes[/b]

[b]New release[/b] (v0.4.20) https://github.com/coasting-nc/LMUFFB/releases

Fixed two force direction inversions that were causing the wheel to pull in the direction of the turn/slide instead of resisting it, creating unstable positive feedback loops (Positive Feedback Loop in Scrub Drag and Yaw Kick).

Let me know if this new version fixes the issue with the slide rumble. If you still get this or some other issue, please post a screenshot of the UI with at least the main window with the settings (you can use the save screenshot button for convenience).

---

[size=5][b]December 19, 2025[/b][/size]
[b]Version v0.4.24 - Guide Presets[/b]

[b]New release[/b] (v0.4.24) https://github.com/coasting-nc/LMUFFB/releases

Note: make sure to check you have in-game FFB disabled and strength set to 0%, since after an LMU update these values can get enabled again.

Added [b]Guide Presets[/b]: Added 5 new built-in presets corresponding to the "Driver's Guide to Testing LMUFFB".
[list]
[*]Guide: Understeer (Front Grip): Isolates the grip modulation effect.
[*]Guide: Oversteer (Rear Grip): Isolates SoP and Rear Aligning Torque.
[*]Guide: Slide Texture (Scrub): Isolates the scrubbing vibration (mutes base force).
[*]Guide: Braking Lockup: Isolates the lockup vibration (mutes base force).
[*]Guide: Traction Loss (Spin): Isolates the wheel spin vibration (mutes base force).
[/list]

These presets allow users to quickly configure the app for the specific test scenarios described in the documentation.

---

[size=5][b]December 20, 2025[/b][/size]
[b]Version v0.4.31 - T300 Preset[/b]

[b]New release[/b] (v0.4.31) https://github.com/coasting-nc/LMUFFB/releases

Fixed inverted SoP (Lateral G) effect, and added a new T300 preset that has working default values for understeer and oversteer / SoP effects.

---

[size=5][b]December 20, 2025[/b][/size]
[b]Version v0.4.37 - Slide Texture Overhaul[/b]

[b]New release[/b] (v0.4.37) https://github.com/coasting-nc/LMUFFB/releases

Fixed Slide Rumble bug (due to "phase explosion" issue that caused massive constant force pulls during frame stutters).

[b]Slide Texture Overhaul[/b]: Now detects rear slip (drifting/donuts), uses optimized frequencies (10-70Hz) for better rumble on belt wheels, and adds a manual Pitch slider.

[b]Tuning[/b]: All defaults and presets have been re-calibrated around a T300 baseline

---

[size=5][b]December 20, 2025[/b][/size]
[b]Version v0.4.38 - Frame Stutter Fixes[/b]

[b]New release[/b] (v0.4.38) https://github.com/coasting-nc/LMUFFB/releases

More fixes to ensure consistent FFB even in case of physics frames stuttering.

---

[size=5][b]December 20, 2025[/b][/size]
[b]Version v0.4.39 - Improved Tyre Physics[/b]

[b]New release[/b] (v0.4.39) https://github.com/coasting-nc/LMUFFB/releases

Implemented better approximations for tyre grip and load.

---

[size=5][b]December 21, 2025[/b][/size]
[b]Version v0.4.43 - Latency Reduction & Flatspot Suppression[/b]

[b]New release[/b] (v0.4.43) https://github.com/coasting-nc/LMUFFB/releases

Fixed "delay" in FFB: reduced latency to < 15ms due to SoP Smoothing and Slip Angle Smoothing

Added sliders to adjust SoP Smoothing and Slip Angle Smoothing

[b]Eliminating LMU tyre flatspot vibration[/b]:
[list]
[*]Added Dynamic Notch Filter (Flatspot Suppression) with Dynamic Suppression Strength
[*]Added Static Notch Filter
[*]See this guide on how to use them: Dynamic Flatspot Suppression - User Guide
[/list]

Fixes to Yaw Kick noise: Muted for low forces and low speeds.

---

[size=5][b]December 21, 2025[/b][/size]
[b]Version v0.4.44 - Focus Loss Diagnostics[/b]

[b]New release[/b] (v0.4.44) https://github.com/coasting-nc/LMUFFB/releases

Fixes and diagnostics for the wheel disconnecting when lmuffb app is not focused.

If you still get issues with the FFB disappearing or wheel disconnecting when lmuffb app is not focused, try the following:
[list]
[*]Start the lmuFFB app and select your wheel in the drop down menu BEFORE starting LMU (in this way it might keep its priority on the wheel)
[*]In any case, paste in this forum thread the text or screenshot from the lmuFFB app console, since there are new logs that help diagnose what is causing the problem.
[/list]

---

[size=5][b]December 23, 2025[/b][/size]
[b]Version v0.4.45 / v0.4.46 / v0.5.2 - GUI Improvements[/b]

[b]New release[/b] (v0.4.45) https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Added checkbox for [b]"Always on Top" Mode[/b]
[*]Fine adjustments to the sliders with keyboard keys (left and right arrows)
[/list]

Version v0.4.46:
[list]
[*]Reorganized GUI with 10 collapsible sections, including General FFB settings, Understeer/Front Tyres, Oversteer/Rear Tyres, Grip Estimation, Haptics, and Textures.
[/list]

Version 0.5.2:
[list]
[*]Improved visual design and readability of the app.
[*]Normalized most sliders to a 0-100% range
[/list]

---

[size=5][b]December 24, 2025[/b][/size]
[b]Version 0.5.7 - New Sliders[/b]

[b]New release[/b] (0.5.7): https://github.com/coasting-nc/LMUFFB/releases

Added new sliders:
[list]
[*][b]optimal_slip_angle[/b]: used to estimate grip, affects these effects: Understeer, Oversteer Boost, Slide Texture.
[*][b]optimal_slip_ratio[/b]: controls the "Combined Friction Circle" logic. It determines how much Longitudinal Slip (Braking or Acceleration) contributes to the calculated "Grip Loss." It also scales the vibration strength for Slide Texture.
[*][b]steering_shaft_smoothing[/b]: low pass filter smoothing for the steering shaft torque. It could help reduce "vibrations" from the steering shaft, noticeable in DD wheels. Could be similar to the in-game FFB smoothing settig.
[/list]

---

[size=5][b]December 24, 2025[/b][/size]
[b]Version 0.5.8 / 0.5.10 - Error Handling & Smoothing[/b]

[b]New release[/b] (0.5.8): https://github.com/coasting-nc/LMUFFB/releases

Improves Error Handling for no FFB when lmuFFB not on top.
[list]
[*]Aggressive FFB Recovery: Implemented more robust DirectInput connection recovery. Try to re-acquire the wheel even when the error code is "unknown"
[*]Set "Always on Top" on by default
[*]Added more informative prints to the console when FFB / wheel connection is lost
[/list]

If you still get the issue, paste here a screenshot of the console of lmuFFB.

[b]EDIT: New release (0.5.10)[/b]: https://github.com/coasting-nc/LMUFFB/releases

Added 3 additional sliders that expose three pre-existing smoothing settings:
[list]
[*]Yaw Kick Smoothing, Gyroscopic Damping Smoothing, Chassis Inertia (Load) Smoothing
[/list]

---

[size=5][b]December 25, 2025[/b][/size]
[b]Version 0.5.14 - Error Logging[/b]

[b]New release[/b] (0.5.14): https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Improved more informative error logs on the console to diagnose FFB lost or wheel disconnected.
[/list]

---

[size=5][b]December 25, 2025[/b][/size]
[b]Version 0.6.1 - Advanced Braking & Lockup Effects[/b]

[b]New release[/b] (0.6.1): https://github.com/coasting-nc/LMUFFB/releases

Added new section of settings for [b]Advanced Braking & Lockup effects[/b] adjustments:

Introduces [b]Predictive Lockup logic[/b] based on [b]Angular Deceleration of the wheels[/b], allowing the driver to feel the onset of a lockup before the tire saturates.

[list]
[*]Added sliders to determine at which level of slip the lockup vibration starts (0% amplitude, defaults at 5% slip) and saturates (100% amplitude, defaults at 12-15% slip).
[*]Added Vibration Gamma for a non linear response from 0% to 100% amplitude: 1.0 = Linear, 2.0 = Quadratic, 3.0 = Cubic (Sharp/Late)
[*]Added [b]ABS Pulse[/b] effect based on Brake Pressure.
[*]Uses Brake Pressure info to ensure that Brake Bias changes and ABS are correctly felt through the wheel.
[*]Modulating the vibration amplitude based on Brake Fade (mBrakeTemp > 800°C).
[/list]

Rear vs Front lockup differentiation:
[list]
[*]Added "Rear Boost" adjustment that multiplies vibration amplitude when the rears lockup
[*]Also using a different vibration frequency for rear vs front lockups.
[/list]

Advanced gating logic to avoid false positives and false negatives in lockup detection:
[list]
[*]Brake Gate: Effect is disabled if Brake Input < 2%.
[*]Airborne Gate: Effect is disabled if mSuspForce < 50N (Wheel in air).
[*]Bump Rejection: Effect is disabled if Suspension Velocity > Threshold (e.g., 1.0 m/s).
[*]Relative Deceleration: The wheel must be slowing down significantly faster than the chassis.
[/list]

Added sliders to adjust some of the main gating mechanisms:
[list]
[*]Prediction sensitivity: angular deceleration threshold
[*]Bump Rejection: suspension velocity threshold to disable prediction. Increase for bumpy tracks (Sebring)
[/list]

---

[size=5][b]December 25, 2025[/b][/size]
[b]Version 0.6.2 - DirectInput Recovery[/b]

[b]New release[/b] (0.6.2): https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Improved mechanism to [b]reacquire the wheel device from DirectInput[/b] if LMU regains exclusive control to send FFB.
[/list]

---

[size=5][b]December 27, 2025[/b][/size]
[b]Version 0.6.20 - Tooltip Overhaul[/b]

[b]New release[/b] (0.6.20): https://github.com/coasting-nc/LMUFFB/releases

Changes since v. 0.6.2:
[list]
[*]Overhauled all tooltips
[*]Added Adjustable Yaw Kick Activation Threshold (to remove any noise from the Yaw Kick, and only "kick in" when there is an actual slide starting)
[*]LMU tire vibration removal (Static Notch Filter): Added "Filter Width" (0.1 to 10.0 Hz) for the frequency to block
[*]Renamed "SoP Lateral G" to "Lateral G"
[*]Renamed "Rear Align Torque" to "SoP Self-Aligning Torque"
[*]Expanded the ranges of some sliders
[*]Added sliders to adjust frequency / pitch for vibration effects
[*]Troubleshooting Guide in Readme
[*]Added console prints that detect when some telemetry data (Grip, Tire Load, Suspension) is missing
[*]Save Screenshot button now also include a screenshot of the console
[/list]

---

[size=5][b]December 28, 2025[/b][/size]
[b]Version 0.6.22 - Vibration Fixes[/b]

[b]New release[/b] (0.6.22): https://github.com/coasting-nc/LMUFFB/releases

Fixed vibrations when car still / in the pits:
[list]
[*]Disabled vibration effects when speed below a certain threashold (ramp from 0.0 vibrations at < 0.5 m/s to 1.0 vibrations at > 2.0 m/s).
[*]Automatic Idle Smoothing: Steering Torque is smoothed when car is stationary or moving slowly (< 3.0 m/s). This should remove any remaining engine vibrations.
[*]Road Texture Fallback: alternative method to calculate Road Texture when mVerticalTireDeflection is encrypted from the game (the fallback switches to using Vertical Acceleration  / mLocalAccel.y)
[*]Added console print to diagnose when mVerticalTireDeflection is encrypted
[*]Clarified other console prints about missing data (eg. mTireLoad): this is due to the data being encrypted by LMU on all cars (due to both licensing issues, and, for some data, to avoid esports exploits).
[/list]

---

[size=5][b]December 28, 2025[/b][/size]
[b]Version 0.6.23 - Smoothing Adjustments[/b]

[b]New release[/b] (0.6.23): https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Increased default max speed for smoothing of steering torque from 10km/h to 18/km (you should get no more engine / clutch vibrations below 18 km/h).
[*]Added slider to the GUI to adjust this value (It's a new  Advanced Settings at the bottom of the GUI, below the Tactile Textures section).
[/list]

If you still get the issue of vibrations at low speed, please post a screenshot with the Graphs, include all three of the graph sections.

[b]Edit[/b]: Note that you should set Max Torque Ref according to the Car's Peak Output, NOT your wheelbase capability. The current tooltip is wrong about this. A value of 100 Nm often works best, and values of at least ~40-60 Nm should prevent clipping.

---

[size=5][b]January 1, 2026[/b][/size]
[b]Version 0.6.30 - Low-Speed Fixes[/b]

[b]New release[/b] (0.6.30): https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Fixed Remaining Vibrations at Low-Speed and when in Pits: applied muting and smoothing also to SoP (including Self Aligning Torque) and Base Torque (steering rack).
[*]Fixed: some settings were not being saved to the ini file, so they were been restored to defaults every time you opened the app.
[*]Added auto save feature: every time you change a slider, the configuration gets saved to the ini file.
[*]Updated T300 preset, tested on LMP2 car
[*]Clarified Max Torque Ref tooltip: it represents the expected PEAK torque of the CAR in the game, NOT the wheelbase capability
[/list]

Findings from more testing: the Understeer Effect seems to be working. In fact, with the LMP2 it is even too sensitive and makes the wheel too light. I had to set the understeer effect slider to 0.84, and even then it was too strong. It was not the case with LMGT3 (Porsche).

---

[size=5][b]January 2, 2026[/b][/size]
[b]Version 0.6.31 - Understeer Effect Fixes[/b]

[b]New release[/b] (0.6.31): https://github.com/coasting-nc/LMUFFB/releases

Fixes for Understeer Effect:
[list]
[*]Reduced the previous range from 0-200 to 0-2 (since the useful values seemed to be only between 0-2)
[*]Normalized the slider as a percentage 0-200% (now 200% corresponds to the previous 2.0)
[*]Changed optimal slip angle value in T300 preset to 0.10 (up from 0.06)
[*]Overhauled the tooltips for "Understeer Effect" and "Optimal Slip Angle"
[/list]

---

[size=5][b]January 2, 2026[/b][/size]
[b]Version 0.6.32 - Preset Fix[/b]

[b]New release[/b] (0.6.32): https://github.com/coasting-nc/LMUFFB/releases

Fixed: "Test: Understeer Only" Preset to ensure proper effect isolation.

---

[size=5][b]January 4, 2026[/b][/size]
[b]Version 0.6.35 - DD Presets[/b]

[b]New release[/b] (0.6.35): https://github.com/coasting-nc/LMUFFB/releases

Added new DD presets based on @dISh and Gamer Muscle presets from today:
[list]
[*]GT3 DD 15 Nm (Simagic Alpha)
[*]LMPx/HY DD 15 Nm (Simagic Alpha)
[*]GM DD 21 Nm (Moza R21 Ultra)
[*]GM + Yaw Kick DD 21 Nm (Moza R21 Ultra) - This is a modified version which adds Yaw Kick, which GM did not use and that can give road details.
[/list]

Updated the default preset to match the "GT3 DD 15 Nm (Simagic Alpha)" preset.

---

[size=5][b]January 5, 2026[/b][/size]
[b]Version 0.6.36 - Code Architecture Improvements[/b]

[b]New release[/b] (0.6.36): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved[/b]
[list]
[*][b]Performance Optimization[/b]: Improved code organization and maintainability. Fixed an issue where "Torque Drop" was incorrectly affecting texture effects (Road, Slide, Spin, Bottoming), which now remain distinct during drifts and burnouts.
[/list]

---

[size=5][b]January 31, 2026[/b][/size]
[b]Version 0.6.37 - Application Icon[/b]

[b]New release[/b] (0.6.37): https://github.com/coasting-nc/LMUFFB/releases

[b]Special Thanks[/b] to:
[list]
[*][b]@MartinVindis[/b] for designing the application icon
[*][b]@DiSHTiX[/b] for implementing it
[/list]

[b]Added[/b]
[list]
[*][b]Application Icon[/b]: The app now has a custom icon that appears in Windows Explorer and the Taskbar, making it easier to identify.
[/list]

---

[size=5][b]January 31, 2026[/b][/size]
[b]Version 0.6.38 - LMU Plugin Update[/b]

[b]New release[/b] (0.6.38): https://github.com/coasting-nc/LMUFFB/releases

[b]Special Thanks[/b] to [b]@DiSHTiX[/b] for the LMU Plugin update!

[b]Fixed[/b]
[list]
[*][b]Compatibility with 2025 LMU Plugin[/b]: Fixed build errors after the LMU plugin update, ensuring full compatibility with LMU 1.2/1.3 standards. The fix preserves official files to make future plugin updates easier.
[/list]

---

[size=5][b]January 31, 2026[/b][/size]
[b]Version 0.6.39 - Auto-Connect & Performance[/b]

[b]New release[/b] (0.6.39): https://github.com/coasting-nc/LMUFFB/releases

[b]Special Thanks[/b] to [b]@AndersHogqvist[/b] for the Auto-connect feature!

[b]Added[/b]
[list]
[*][b]Auto-Connect to LMU[/b]: The app now automatically attempts to connect to the game every 2 seconds. No more manual "Retry" clicking! The GUI shows "Connecting to LMU..." in yellow while searching and "Connected to LMU" in green when active.
[/list]

[b]Improved[/b]
[list]
[*][b]Faster Force Feedback[/b]: Reduced internal processing overhead by 33%, improving responsiveness at the critical 400Hz update rate (down from 1,200 to 800 operations per second).
[*][b]Connection Reliability[/b]: Fixed potential resource leaks and improved connection robustness, especially when the game window isn't immediately available.
[/list]

---

[b]Total Releases Found[/b]: 31 version releases from December 6, 2025 to January 31, 2026

[b]Project Links[/b]:
[list]
[*]GitHub Repository: https://github.com/coasting-nc/LMUFFB
[*]GitHub Releases: https://github.com/coasting-nc/LMUFFB/releases
[*]Forum Thread: https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/
[/list]
