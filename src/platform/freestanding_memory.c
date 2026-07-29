#include <stddef.h>
void *memset(void *destination, int value, size_t length)
{
    unsigned char *bytes = destination;
    while (length-- != 0u) {
        *bytes++ = (unsigned char)value;
    }
    return destination;
}
