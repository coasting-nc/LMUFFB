#include "RestApiProvider.h"
#include "Logger.h"
#include <iostream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

RestApiProvider& RestApiProvider::Get() {
    static RestApiProvider instance;
    return instance;
}

RestApiProvider::~RestApiProvider() {
    std::lock_guard<std::mutex> lock(m_threadMutex);
    if (m_requestThread.joinable()) {
        m_requestThread.join();
    }
}

void RestApiProvider::RequestSteeringRange(int port) {
    if (m_isRequesting.load()) return;

    std::lock_guard<std::mutex> lock(m_threadMutex);
    if (m_requestThread.joinable()) {
        m_requestThread.join();
    }

    m_isRequesting = true;
    m_requestThread = std::thread([this, port]() {
        try {
            this->PerformRequest(port);
        } catch (...) {
            Logger::Get().LogFile("RestApiProvider: Unexpected exception in request thread");
        }
        this->m_isRequesting = false;
    });
}

float RestApiProvider::GetFallbackRangeDeg() const {
    return m_fallbackRangeDeg.load();
}

bool RestApiProvider::IsRequesting() const {
    return m_isRequesting.load();
}

void RestApiProvider::PerformRequest(int port) {
    std::string response;
    bool success = false;

#ifdef _WIN32
    HINTERNET hInternet = InternetOpenA("lmuFFB", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        std::string url = "http://localhost:" + std::to_string(port) + "/rest/garage/getPlayerGarageData";
        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hConnect) {
            char buffer[4096];
            DWORD bytesRead;
            while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                response.append(buffer, bytesRead);
            }
            InternetCloseHandle(hConnect);
            success = true;
        } else {
            Logger::Get().LogFile("RestApiProvider: Failed to open URL (Port %d)", port);
        }
        InternetCloseHandle(hInternet);
    } else {
        Logger::Get().LogFile("RestApiProvider: Failed to initialize WinINet");
    }
#else
    // Mock for Linux/Testing
    // In real tests, we might want to inject this mock behavior
    Logger::Get().LogFile("RestApiProvider: Mock request on Linux (Port %d)", port);
    // For now, do nothing, we will mock ParseSteeringLock in tests
#endif

    if (success && !response.empty()) {
        float range = ParseSteeringLock(response);
        if (range > 0.0f) {
            m_fallbackRangeDeg = range;
            Logger::Get().LogFile("RestApiProvider: Retrieved steering range: %.1f deg", range);
        } else {
            Logger::Get().LogFile("RestApiProvider: Could not parse VM_STEER_LOCK from response");
        }
    }
}

float RestApiProvider::ParseSteeringLock(const std::string& json) {
    // Look for "VM_STEER_LOCK" and then "stringValue"
    // Example: "VM_STEER_LOCK" : { ... "stringValue" : "540 deg" ... }

    // 1. Find the property
    size_t propPos = json.find("\"VM_STEER_LOCK\"");
    if (propPos == std::string::npos) return 0.0f;

    // 2. Find "stringValue" within the next few hundred characters
    size_t searchLimit = 512;
    std::string sub = json.substr(propPos, searchLimit);
    size_t valPos = sub.find("\"stringValue\"");
    if (valPos == std::string::npos) return 0.0f;

    // 3. Extract the value string
    size_t colonPos = sub.find(':', valPos);
    if (colonPos == std::string::npos) return 0.0f;

    size_t startQuote = sub.find('\"', colonPos);
    if (startQuote == std::string::npos) return 0.0f;

    size_t endQuote = sub.find('\"', startQuote + 1);
    if (endQuote == std::string::npos) return 0.0f;

    std::string valueStr = sub.substr(startQuote + 1, endQuote - startQuote - 1);

    // 4. Extract numeric value using regex
    std::regex re(R"(\d*\.?\d+)");
    std::smatch match;
    if (std::regex_search(valueStr, match, re)) {
        try {
            return std::stof(match.str());
        } catch (...) {
            return 0.0f;
        }
    }

    return 0.0f;
}
