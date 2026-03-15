#include "test_ffb_common.h"
#include "io/RestApiProvider.h"

namespace FFBEngineTests {

TEST_CASE(test_rest_api_manufacturer_parsing, "Internal") {
    RestApiProvider& provider = RestApiProvider::Get();

    std::string json = R"raw([
        {"desc": "Ferrari 488 GTE Evo", "manufacturer": "Ferrari"},
        {"desc": "Porsche 911 GT3 R (992)", "manufacturer": "Porsche"},
        {"desc": "Proton Competition 2025 #60:ELMS", "manufacturer": "Porsche"}
    ])raw";

    // Exact match
    ASSERT_EQ_STR(RestApiProviderTestAccess::ParseManufacturer(provider, json, "Ferrari 488 GTE Evo"), "Ferrari");

    // Loose match
    ASSERT_EQ_STR(RestApiProviderTestAccess::ParseManufacturer(provider, json, "Proton Competition 2025 #60:ELMS"), "Porsche");

    // Loose match 2
    ASSERT_EQ_STR(RestApiProviderTestAccess::ParseManufacturer(provider, json, "911 GT3 R (992)"), "Porsche");

    // Unknown match
    ASSERT_EQ_STR(RestApiProviderTestAccess::ParseManufacturer(provider, json, "Unknown Car"), "Unknown");
}

}
