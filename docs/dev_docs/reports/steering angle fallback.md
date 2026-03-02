


A known limitation with the rFactor 2 / Le Mans Ultimate Shared Memory API is that the `mPhysicalSteeringWheelRange` is indeed frequently `0` or less for certain vehicles (especially in Le Mans Ultimate).

### Getting the Steering Wheel Rotation Range

**REST API Fallback**
If the Shared Memory returns `<= 0` for `mPhysicalSteeringWheelRange`, make an HTTP GET request to the game's built-in REST API to read the car's garage setup data. 

**REST API fallback how to:**
*   **Endpoint:** Query `http://localhost:6397/rest/garage/getPlayerGarageData` (Port 6397 for LMU, 5397 for rF2).
*   **JSON Path:** Look for the setup property `"VM_STEER_LOCK"` and read its `"stringValue"`.
*   **Parsing:** Because the REST API returns a string (e.g., `"540 deg"`), you should parse it to extract just the numeric part. Eg. you can use a regular expression (`\d*\.?\d+`).

#### Safety

Note that the REST API for LMU is not officially supported or documented, and it is known to cause crashes in the game is queried too frequently. 
Therefore we must be careful to query it the absolute minimum number of times necessary to get the steering angle.
For additional safety, we should add a UI checkbox / toggle to allow the user to disable this feature (getting the steering range from the REST API) if the current reading is `<= 0`. For safety we should disable it by default.

### Summary :
* If `mPhysicalSteeringWheelRange` is `<= 0`, make a background HTTP request to the game's REST API (`/rest/garage/getPlayerGarageData`), extract `"VM_STEER_LOCK"`, and parse the number from the string. Then calculate the final angle.