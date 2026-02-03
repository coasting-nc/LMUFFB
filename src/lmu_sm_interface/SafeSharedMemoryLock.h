#pragma once
#include "LmuSharedMemoryWrapper.h"

// Wrapper for SharedMemoryLock that adds timeout support without modifying vendor code
// This avoids the maintenance burden of modifying the vendor's SharedMemoryInterface.hpp
class SafeSharedMemoryLock {
public:
    // Factory method that returns a SafeSharedMemoryLock wrapper
    static std::optional<SafeSharedMemoryLock> MakeSafeSharedMemoryLock() {
        auto vendorLock = SharedMemoryLock::MakeSharedMemoryLock();
        if (vendorLock.has_value()) {
            return SafeSharedMemoryLock(std::move(vendorLock.value()));
        }
        return std::nullopt;
    }

    // Lock with timeout support - wraps the vendor's Lock() method
    // Returns false if timeout expires or lock acquisition fails
    bool Lock(DWORD timeout_ms = 50) {
        // The vendor implementation already supports timeout parameter
        // We just expose it through our wrapper
        return m_vendorLock.Lock(timeout_ms);
    }

    void Unlock() {
        m_vendorLock.Unlock();
    }

    // Move constructor and assignment to allow std::optional usage
    SafeSharedMemoryLock(SafeSharedMemoryLock&& other) = default;
    SafeSharedMemoryLock& operator=(SafeSharedMemoryLock&& other) = default;

private:
    // Private constructor - use factory method
    explicit SafeSharedMemoryLock(SharedMemoryLock&& vendorLock) 
        : m_vendorLock(std::move(vendorLock)) {}

    SharedMemoryLock m_vendorLock;
};
