#ifndef RESTAPIPROVIDER_H
#define RESTAPIPROVIDER_H

#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <optional>

namespace LMUFFB::IO {

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

    void ResetSteeringRange() {
        m_fallbackRangeDeg.store(0.0f);
    }

private:
    RestApiProvider() = default;
    ~RestApiProvider();

    void PerformRequest(int port);
    float ParseSteeringLock(const std::string& json);

    friend class ::LMUFFB::RestApiProviderTestAccess;

    std::atomic<bool> m_isRequesting{false};
    std::atomic<float> m_fallbackRangeDeg{0.0f};

    mutable std::mutex m_threadMutex;
    std::thread m_requestThread;
};

} // namespace LMUFFB::IO

// Bridge alias: keeps existing call sites (LMUFFB::RestApiProvider) compiling without changes
namespace LMUFFB {
    using RestApiProvider = LMUFFB::IO::RestApiProvider;
} // namespace LMUFFB

#endif // RESTAPIPROVIDER_H
