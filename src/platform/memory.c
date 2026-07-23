#include <stddef.h>

/*
 * Freestanding fallback currently required by generated StateSmith code.
 * NOG_C uses its own memory helpers in the compact embedded profile.
 */
void *memset(void *destination, int value, size_t count)
{
    unsigned char *output = destination;

    while (count-- > 0u) {
        *output++ = (unsigned char)value;
    }
    return destination;
}
