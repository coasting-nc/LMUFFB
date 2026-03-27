#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <cstring>
#include <cstdio>
#include <cstdarg>

namespace LMUFFB {
namespace Utils {
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

// Internal helper for variadic formatting
inline int vSafeFormat(char* dest, size_t dest_size, const char* format, va_list args) {
    if (!dest || dest_size == 0) return -1;
#ifdef _WIN32
    return vsnprintf_s(dest, dest_size, _TRUNCATE, format, args);
#else
    int result = vsnprintf(dest, dest_size, format, args);
    if (result >= (int)dest_size || result < 0) {
        dest[dest_size - 1] = '\0';
    }
    return result;
#endif
}

// Safe formatted print method compatible with Windows and Linux.
// It will always null terminate the destination buffer.
inline int SafeFormat(char* dest, size_t dest_size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vSafeFormat(dest, dest_size, format, args);
    va_end(args);
    return result;
}

// Safe formatted scan method compatible with Windows and Linux.
inline int SafeScan(const char* src, const char* format, ...) {
    if (!src || !format) return -1;
    va_list args;
    va_start(args, format);
    int result = vsscanf(src, format, args);
    va_end(args);
    return result;
}

} // namespace StringUtils
} // namespace Utils
} // namespace LMUFFB

#endif // STRINGUTILS_H
