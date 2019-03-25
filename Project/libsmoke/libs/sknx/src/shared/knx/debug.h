#ifndef KNX_DEBUG_H
#define KNX_DEBUG_H

#ifdef _DEBUG
#include <stdarg.h>
class Debug {
public:
    static void printArray(const uint8_t *arr, size_t len) {
        printf("[");
        for(size_t i=0;i<len;i++)
            printf("%02x", arr[i]);
        printf("]\n");
    }

    static void printArray(const char *str, const uint8_t *arr, size_t len) {
        printf("%s ", str);
        printArray(arr, len);
    }

    static void print(const char *tag, const char *str, ...) {
        if(!tag || !str) return;

        printf("[%s] ", tag);
        va_list a;
        va_start(a, str);
        vprintf(str, a);
        va_end(a);
        printf("\n");
    }
};

#define LOG(x) Debug::print(__TAG(), x)
#define LOGP(x, ...) Debug::print(__TAG(), x, __VA_ARGS__)
#define TAG_DEF(x) static const char *__TAG() { return x; }

#else // !defined( _DEBUG)

class Debug {
public:
    static inline void printArray(const uint8_t *arr, size_t len) {
        (void)(arr);
        (void)(len);
    }
    static inline void print(const char *tag, const char *str) {
        (void)(tag);
        (void)(str);
    }
    static void printArray(const char *str, const uint8_t *arr, size_t len) {
        (void)(str);
        (void)(arr);
        (void)(len);
    }
};

#define LOG(x) do { } while(0)
#define LOGP(x, ...) do { } while(0)
#define TAG_DEF(x)
#endif // #ifdef _DEBUG

#endif /* ifndef KNX_DEBUG_H */
