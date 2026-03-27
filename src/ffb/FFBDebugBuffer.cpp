#include "FFBDebugBuffer.h"

namespace LMUFFB::FFB {

FFBDebugBuffer::FFBDebugBuffer(size_t capacity) : m_capacity(capacity) {}

void FFBDebugBuffer::Push(const FFBSnapshot& snap) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_buffer.size() >= m_capacity) {
        return; 
    }
    m_buffer.push_back(snap);
}

std::vector<FFBSnapshot> FFBDebugBuffer::GetBatch() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FFBSnapshot> batch = std::move(m_buffer);
    m_buffer.clear();
    return batch;
}

size_t FFBDebugBuffer::Size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_buffer.size();
}

} // namespace LMUFFB::FFB
