### Code Review Iteration 1

**Reviewer Feedback:**
1. **Blocking:** Compilation failure due to missing include in `FFBMetadataManager.cpp`.
2. **Blocking:** High risk of a crash/instability due to a potential dangling pointer in the logging logic. (Actually, `ParseVehicleBrand` returns `const char*` which points to string literals, so it's safe, but the reviewer was cautious. I should double check).
3. **Blocking:** Functional regression: The steering range fallback is now effectively dead code because its trigger was removed without replacement.
4. **Blocking:** Failure to follow mandatory project maintenance steps (versioning, changelog, and process documentation).

**My Response:**
1. I will add `#include "physics/VehicleUtils.h"` to `FFBMetadataManager.cpp`.
2. `ParseVehicleBrand` returns `const char*` which are string literals (e.g., `return "Ferrari";`), so they are not dangling pointers. However, I'll ensure the include is there to avoid any ambiguity.
3. I will re-add a trigger for `RequestSteeringRange` in `FFBMetadataManager.cpp` or ensure it's called appropriately. Wait, `FFBEngine::calculate_force` already calls `RestApiProvider::Get().RequestSteeringRange(m_rest_api_port);` if the range is invalid. Let me check that.
4. I will update `VERSION` and `CHANGELOG_DEV.md` in the final steps as per my plan, but I'll make sure to record these reviews as well.

**Checking `FFBEngine.cpp` for `RequestSteeringRange`:**
In `FFBEngine::calculate_force`:
```cpp
    // Trigger REST API Fallback if enabled and range is invalid (Issue #221)
    if (seeded && m_rest_api_enabled && data->mPhysicalSteeringWheelRange <= 0.0f) {
        RestApiProvider::Get().RequestSteeringRange(m_rest_api_port);
    }
```
So it is still triggered. However, `FFBMetadataManager::UpdateInternal` previously had:
```cpp
        RestApiProvider::Get().ResetSteeringRange(); // Issue #379
        RestApiProvider::Get().RequestManufacturer(6397, m_vehicle_name); // TODO: use configured port
```
The steering range was requested via `RequestManufacturer` indirectly? No, `RequestManufacturer` is a separate endpoint.
Wait, `RequestSteeringRange` is for `/rest/garage/getPlayerGarageData`.
The `FFBEngine` triggers it every frame if range is 0.
So it should be fine. But maybe we want to trigger it once on car change too?

Let's check `RestApiProvider::RequestSteeringRange`.
It uses `PerformRequest` which hits `http://localhost:" + std::to_string(port) + "/rest/garage/getPlayerGarageData`.
It seems `RequestManufacturer` hit `/rest/race/car`.

If I remove `RequestManufacturer`, I'm only removing the brand detection.
The steering range is still handled in `FFBEngine.cpp`.

However, the reviewer says "it fails to replace the trigger for the REST API background thread. Previously, RequestManufacturer was the entry point that started the thread to perform requests."
This is not entirely true, `RequestSteeringRange` also starts the thread.

But maybe it's better to trigger a steering range request on car change in `FFBMetadataManager` too, just to be proactive.
