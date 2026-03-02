#include "test_ffb_common.h"
#include "../src/RestApiProvider.h"
#include <thread>
#include <chrono>

using namespace FFBEngineTests;

// Forward declaration of mock access
class RestApiProviderTestAccess {
public:
    static float ParseSteeringLock(RestApiProvider& p, const std::string& json) {
        return p.ParseSteeringLock(json);
    }
};

TEST_CASE(test_rest_api_parsing, "RestApi") {
    RestApiProvider& provider = RestApiProvider::Get();

    // 1. Valid JSON with degrees
    std::string json1 = "{\"VM_STEER_LOCK\":{\"stringValue\":\"540 deg\"}}";
    ASSERT_NEAR(RestApiProviderTestAccess::ParseSteeringLock(provider, json1), 540.0f, 0.1f);

    // 2. Valid JSON with float string
    std::string json2 = "{\"VM_STEER_LOCK\":{\"stringValue\":\"900.5\"}}";
    ASSERT_NEAR(RestApiProviderTestAccess::ParseSteeringLock(provider, json2), 900.5f, 0.1f);

    // 3. JSON with extra text
    std::string json3 = "{\"VM_STEER_LOCK\":{\"stringValue\":\"270 (540 deg)\"}}";
    ASSERT_NEAR(RestApiProviderTestAccess::ParseSteeringLock(provider, json3), 270.0f, 0.1f); // Picks first match

    // 4. Missing field
    std::string json4 = "{\"OTHER_FIELD\":{\"stringValue\":\"540 deg\"}}";
    ASSERT_EQ(RestApiProviderTestAccess::ParseSteeringLock(provider, json4), 0.0f);

    // 5. Malformed/Empty
    ASSERT_EQ(RestApiProviderTestAccess::ParseSteeringLock(provider, ""), 0.0f);
}

TEST_CASE(test_engine_rest_fallback_integration, "RestApi") {
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetRestApiEnabled(engine, true);

    TelemInfoV01 data = CreateBasicTestTelemetry();
    data.mPhysicalSteeringWheelRange = 0.0f; // Invalid

    // Manually set fallback range in provider (simulating a successful request)
    // Since we are on Linux, RequestSteeringRange doesn't do much,
    // but we can verify it doesn't crash and engine state is sane.

    FFBEngineTestAccess::SetRestApiEnabled(engine, true);
    FFBEngineTestAccess::SetRestApiPort(engine, 1234);

    // Car change trigger
    engine.calculate_force(&data, "GT3", "Test Car", 0.0f, true);

    // Verify no crash when requesting on Linux
    RestApiProvider::Get().RequestSteeringRange(1234);
}
