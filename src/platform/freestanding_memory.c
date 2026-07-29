#include <stddef.h>
#include <stdint.h>

/*
 * The firmware links with -nostdlib.  Keep the standard byte-copy primitive
 * locally so compilers may use it for ordinary structure copies without
 * pulling a C runtime (or any 64-bit / floating-point helper routines).
 */
void *memcpy(void *destination, const void *source, size_t length)
{
    unsigned char *dst = destination;
    const unsigned char *src = source;
    typedef uint32_t copy_word_t __attribute__((__may_alias__));

    /* Copy aligned words when both addresses have the same alignment. */
    if ((((uintptr_t)dst ^ (uintptr_t)src) & 3u) == 0u) {
        while ((((uintptr_t)dst & 3u) != 0u) && (length != 0u)) {
            *dst++ = *src++;
            --length;
        }
        while (length >= sizeof(copy_word_t)) {
            *(copy_word_t *)dst = *(const copy_word_t *)src;
            dst += sizeof(copy_word_t);
            src += sizeof(copy_word_t);
            length -= sizeof(copy_word_t);
        }
    }
    while (length-- != 0u) {
        *dst++ = *src++;
    }
    return destination;
}

void *memset(void *destination, int value, size_t length)
{
    unsigned char *bytes = destination;
    while (length-- != 0u) {
        *bytes++ = (unsigned char)value;
    }
    return destination;
}
