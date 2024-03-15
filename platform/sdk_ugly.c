#include "sdk.h"
#include <stdint.h>
/* ugly sdk hack: start */
#define MAP_FAILED ((void *)-1)

void *mmap64(void *start, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *__addr, size_t __len);

void *mmap(void *start, size_t len, int prot, int flags, int fd, uint32_t off) {
    return mmap64(start, len, prot, flags, fd, off);
}
/* ugly sdk hack: end */
