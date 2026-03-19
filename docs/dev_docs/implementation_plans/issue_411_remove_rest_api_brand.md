# Implementation Plan: Remove REST API use for car brand detection (#411)

## 1. Context & Background
The current version of the FFB engine attempts to retrieve car manufacturer (brand) information using an asynchronous REST API call to LMU's internal web server. This has proven unreliable, often returning incorrect car information (e.g., defaulting to "Aston Martin GT3").

## 2. Analysis of current implementation
* **`RestApiProvider`**: Handles asynchronous requests for steering range and manufacturer.
* **`FFBMetadataManager::UpdateMetadata`**: Triggers a log message on vehicle change that includes `REST Brand`.
* **`FFBMetadataManager::UpdateInternal`**: Resets and requests manufacturer info via REST API when a new vehicle is detected.
* **`VehicleUtils`**: Contains `ParseVehicleBrand`, which infers the brand from the vehicle name and class (from shared memory). This is already used for session logging in `main.cpp`.

### Design Rationale
> The goal is to eliminate the unreliable external dependency on the REST API for branding and consolidate on the internal `ParseVehicleBrand` logic which uses telemetry data.

## 3. Proposed Changes

### `src/io/RestApiProvider.h` & `.cpp`
* Remove all functions related to manufacturer detection:
    * `RequestManufacturer`
    * `GetManufacturer`
    * `HasManufacturer`
    * `ResetManufacturer`
    * `PerformManufacturerRequest`
    * `ParseManufacturer`
* Remove member variables:
    * `m_manufacturer`
    * `m_hasManufacturer`
    * `m_manufacturerMutex`
* Note: Keep steering range request functionality as it's still used for fallback when shared memory provides 0.0 range.

### `src/ffb/FFBMetadataManager.cpp`
* Remove `RestApiProvider::Get().RequestManufacturer(...)` and `ResetManufacturer()` from `UpdateInternal`.
* Update `UpdateMetadata` log message:
    * Remove `std::string restBrand = RestApiProvider::Get().GetManufacturer();`.
    * Use `ParseVehicleBrand(vehicleClass, vehicleName)` directly.
    * Update the log format string.

### Design Rationale
> Removing the code from `RestApiProvider` ensures we don't accidentally leave unused threads or memory. Updating `FFBMetadataManager` ensures the primary vehicle change log remains accurate using the same logic as the session log.

## 4. Test Plan
* **Build Verification**: `cmake --build build` must succeed on Linux.
* **Functional Test**: Check `tests/` for any tests using `RestApiProvider::GetManufacturer` and update/remove them.
* **Regressions**: Run `./build/tests/run_combined_tests`.

### Design Rationale
> Since we are removing an external API dependency that isn't easily testable without a running game, we focus on ensuring compilation and existing test suite integrity.

## 5. Implementation Notes
* No new configuration settings are required as we are removing a feature.
* `ParseVehicleBrand` is already robust and centralized in `VehicleUtils.cpp`.

### Encountered Issues
* **Compilation Failure in Tests**: Removing `ParseManufacturer` from `RestApiProvider` caused a compilation error in `tests/test_ffb_common.h` and `tests/test_rest_api_manufacturer.cpp` which were using it for unit testing.
* **Missing Include**: Initial code review caught a missing include for `physics/VehicleUtils.h` in `FFBMetadataManager.cpp`.

### Deviations from the Plan
* **Renamed Test File**: Renamed `tests/test_rest_api_manufacturer.cpp` to `tests/test_vehicle_brand.cpp` as suggested during code review to better reflect its new purpose (verifying local brand parsing instead of REST API).
* **Updated Test Registry**: Updated `tests/CMakeLists.txt` to include the renamed test file.

### Suggestions for the Future
* The `ParseVehicleBrand` logic in `VehicleUtils.cpp` could be expanded with more keywords as new cars are released, now that it's the primary source of brand information.

## 6. Additional Questions
* None.
