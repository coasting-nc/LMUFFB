#ifndef RESTAPIPROVIDER_H
#define RESTAPIPROVIDER_H

#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <optional>

class RestApiProvider {
public:
    static RestApiProvider& Get();

    // Trigger an asynchronous request for the steering range
    void RequestSteeringRange(int port);

    // Get the latest successfully retrieved range in degrees
    // Returns 0.0 if no value has been retrieved yet
    float GetFallbackRangeDeg() const;

    // Is a request currently in flight?
    bool IsRequesting() const;

    // Has a manufacturer been successfully retrieved?
    bool HasManufacturer() const { return m_hasManufacturer; }

    // Trigger an asynchronous request for the car manufacturer
    void RequestManufacturer(int port, const std::string& vehicleName);

    // Get the latest successfully retrieved manufacturer
    std::string GetManufacturer() const;

    // Reset manufacturer when vehicle changes
    void ResetManufacturer() {
        std::lock_guard<std::mutex> lock(m_manufacturerMutex);
        m_manufacturer = "Unknown";
        m_hasManufacturer = false;
    }

private:
    RestApiProvider() = default;
    ~RestApiProvider();

    void PerformRequest(int port);
    void PerformManufacturerRequest(int port, std::string vehicleName);
    float ParseSteeringLock(const std::string& json);
    std::string ParseManufacturer(const std::string& json, const std::string& vehicleName);

    friend class RestApiProviderTestAccess;

    std::atomic<bool> m_isRequesting{false};
    std::atomic<float> m_fallbackRangeDeg{0.0f};

    mutable std::mutex m_manufacturerMutex;
    std::string m_manufacturer = "Unknown";
    bool m_hasManufacturer = false;

    mutable std::mutex m_threadMutex;
    std::thread m_requestThread;
};

#endif // RESTAPIPROVIDER_H
