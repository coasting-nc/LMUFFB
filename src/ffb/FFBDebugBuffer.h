#ifndef FFDBEBUG_BUFFER_H
#define FFDBEBUG_BUFFER_H

#include <vector>
#include <mutex>
#include "FFBSnapshot.h"

namespace LMUFFB {
namespace FFB {

class FFBDebugBuffer {
public:
    explicit FFBDebugBuffer(size_t capacity);

    void Push(const FFBSnapshot& snap);
    std::vector<FFBSnapshot> GetBatch();
    size_t Size() const;

    // Public for tests/legacy compatibility
    mutable std::mutex m_mutex;

private:
    std::vector<FFBSnapshot> m_buffer;
    size_t m_capacity;
};

} // namespace FFB

// Bridge Aliases
using FFBDebugBuffer = FFB::FFBDebugBuffer;

} // namespace LMUFFB

#endif // FFDBEBUG_BUFFER_H
