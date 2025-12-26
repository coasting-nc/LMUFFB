Based on the transcript provided, here is a detailed list of issues, bugs, and feedback points identified by GamerMuscle regarding the **lmuFFB** application.

The list is categorized to help the developer prioritize fixes.

### 1. Critical Technical & Stability Issues

*   **App Losing Connection on Window Focus Change**
    *   **Issue:** The application stops sending force feedback or loses connection to the wheel when the user clicks away from the app or when the game window is not in focus.
    *   **Timestamp:** 5:54:00, 5:57:11, 6:28:17, 6:31:55, 6:45:22.
    *   **Observation:** GamerMuscle noted, "Every time you select the game off the app, it loses... Window needs to be in focus for it to work." He had to repeatedly re-select his wheel in the dropdown menu to get it working again.
*   **Wheel Device Selection Persistence**
    *   **Issue:** The app does not seem to remember the selected wheel or maintain the hook to the device reliably.
    *   **Timestamp:** 5:26:09 ("Device not connected"), 6:28:17.
    *   **Observation:** He had to manually re-select the wheel multiple times during the session.

### 2. Force Feedback Quality & Latency

*   **Input/FFB Latency (Delay)**
    *   **Issue:** There is a perceptible delay between the car's physics behavior and the force feedback response, particularly noticeable during fast corrections or when the rear steps out.
    *   **Timestamp:** 5:30:19, 5:50:08, 6:49:49, 7:06:25, 7:47:03.
    *   **Observation:** "Definitely a lack of tightness... delay in the physics." "When it snapped out there, it's out of sync with the sim." He notes that while the app adds dynamic range, the delay makes catching slides difficult ("fast corrections feel disconnected").
*   **Excessive Vibration / Rumble (The "Flat Spot" Noise)**
    *   **Issue:** The app picks up and amplifies a constant, high-frequency vibration from the game engine (attributed to tire wear or tire model noise) that drowns out useful FFB detail. This happens even on cold tires or with minimal wear.
    *   **Timestamp:** 5:33:52, 5:58:35, 6:03:02, 6:06:02.
    *   **Technical Detail:** GamerMuscle estimates this vibration is around **25Hz to 40Hz** (timestamps 6:03:02, 6:11:43).
    *   **User Suggestion:** He suggests implementing a **Frequency Band Isolator** or a specific filter (notch filter) to remove frequencies between 25Hz–60Hz to eliminate this specific "LMU rumble" without dampening the rest of the FFB (Timestamp 6:06:36, 6:09:57).

### 3. Specific Effect Tuning & Behavior

*   **"Seat of the Pants" Implementation**
    *   **Issue:** The user felt the "Seat of the Pants" effect was behaving unexpectedly. He noted it seemed to apply load based on G-load rather than the rotation/yaw of the rear stepping out.
    *   **Timestamp:** 5:38:50, 5:45:39.
    *   **Observation:** "Not operating how you generally expect... it's applying a load for the G load."
*   **"Yaw Kick" Effect**
    *   **Issue:** Described as feeling like a "random damper" rather than a useful informative force.
    *   **Timestamp:** 5:51:20, 7:05:57.
    *   **Observation:** "The Yaw kick just seems to add in a random like damper."
*   **Understeer Effect**
    *   **Issue:** The enhanced understeer effect can feel like it is "fighting" the user unrealistically.
    *   **Timestamp:** 6:16:56.
*   **Oversteer Boost**
    *   **Issue:** User was unclear on its function, and suspected it might be contributing to the feeling of delay.
    *   **Timestamp:** 5:47:31, 5:50:14.

### 4. UI/UX & Onboarding

*   **Lack of Tooltips/Documentation**
    *   **Issue:** The user was confused by several setting names ("Steer Boost," "Yaw Kick," "Rear Align Torque" vs "Seat of the Pants").
    *   **Timestamp:** 5:26:49 ("There's no instructions"), 5:47:31.
    *   **Recommendation:** Add tooltips explaining what specific sliders do (e.g., "Steer Boost").
*   **Visibility of "Advanced Tuning"**
    *   **Issue:** The user struggled to find the "Advanced Tuning" section, suggesting the UI layout might need better visual hierarchy.
    *   **Timestamp:** 5:32:33.
    *   **Observation:** "I couldn't see it for some reason... looking at the text and it just wasn't registering."
*   **Initial Configuration Confusion**
    *   **Issue:** Confusion regarding "Missing tire load" and "Output saturated" warnings upon first launch.
    *   **Timestamp:** 5:26:25.

### 5. Game-Specific Issues (Contextualizing App Behavior)

*Note: These are issues the YouTuber attributed to the game (Le Mans Ultimate), but they are relevant to the developer because the app **exposes or amplifies** them.*

*   **Tire Model Noise:** The app exposes a "weird density" or constant rumble in the LMU tire model that the default game FFB smooths over/hides. The app makes the "shittiness" of the underlying physics more apparent (Timestamp 6:14:19, 7:49:02).
*   **Cold Tire Physics:** The user complained heavily about "driving on snow" with cold tires. The app's detailed FFB made this lack of grip feel even more exaggerated and "sloppy" compared to the muted default FFB (Timestamp 7:21:55).
*   **Default FFB Comparison:** The user concluded that the app is "a million times better" than default for dynamic range (especially for low-end wheels like T300), but the **delay** and the **unfiltered noise** (vibration) are the major trade-offs that need fixing (Timestamp 5:49:39, 7:44:02).

### Summary of Feature Requests from User
1.  **Frequency Isolator/Notch Filter:** To target and remove the specific ~25-40Hz rumble caused by the game's tire model.
2.  **Tooltips:** Explanations for "Steer Boost," "Yaw Kick," etc.
3.  **Fix Window Focus:** Allow the app to run in the background without losing connection to the wheel.
