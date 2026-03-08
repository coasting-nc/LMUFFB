#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <cstring>

namespace StringUtils {

// Safe string copy method compatible with Windows and Linux.
// It will always null terminate the destination buffer.
// The dest_size is the total size of the destination buffer including the null terminator.
inline void SafeCopy(char* dest, size_t dest_size, const char* src) {
    if (!dest || dest_size == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
#ifdef _WIN32
    strncpy_s(dest, dest_size, src, _TRUNCATE);
#else
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
#endif
}

} // namespace StringUtils

#endif // STRINGUTILS_H
