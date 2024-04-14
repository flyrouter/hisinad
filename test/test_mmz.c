#include <hitiny/hitiny_sys.h>
#include <aux/logger.h>

#define SZ_ARRAY 4096

void do_test(unsigned* ptr)
{
    for (unsigned i = 0; i < SZ_ARRAY; ++i) ptr[i] = (i + 1) * (i + 1);

    unsigned long long sum = 0;
    for (unsigned i = 0; i < SZ_ARRAY; ++i) sum += ptr[i];

    unsigned long long xsum = SZ_ARRAY + 1;
    xsum *= 2 * SZ_ARRAY + 1;
    xsum *= SZ_ARRAY;
    xsum /= 6;

    log_info("sum 1^2 + 2^2 + ... + %u^2 = %llu, have to be %llu", SZ_ARRAY, sum, xsum);
}

int main(int argc, char** argv)
{
    unsigned phy_addr;
    void* ptr;

    int r = hitiny_MPI_SYS_MmzAlloc_Cached(&phy_addr, &ptr, "usr-tester", 0, SZ_ARRAY * sizeof(unsigned));
    log_info("MMZ ALLOC returned %u (0 is OK)", r);
    if (r) return r;

    log_info("PHY addr 0x%x, virt 0x%x", phy_addr, ptr);

    memset(ptr, 0, SZ_ARRAY * sizeof(unsigned));
    r = hitiny_MPI_SYS_MmzFlushCache(phy_addr, ptr, SZ_ARRAY * sizeof(unsigned));
    //r = hitiny_MPI_SYS_MmzFlushCache(0, 0, 0);
    log_info("MMZ FLUSH returned 0x%x (0 is OK)", r);

    do_test(ptr);

    r = hitiny_MPI_SYS_MmzFree(phy_addr, ptr);
    log_info("MMZ FREE returned %u (0 is OK)", r);

    return 0;
}

