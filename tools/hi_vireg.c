#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

unsigned read_mem_reg(uint32_t addr) {
    static int mem_fd;
    static char *loaded_area;
    static uint32_t loaded_offset;
    static uint32_t loaded_size;

    uint32_t offset = addr & 0xffff0000;
    uint32_t size = 0xffff;
    if (!addr || (loaded_area && offset != loaded_offset)) {
        int res = munmap(loaded_area, loaded_size);
        if (res) {
            fprintf(stderr, "read_mem_reg error: %s (%d)\n", strerror(errno), errno);
        }
    }

    if (!addr) {
        close(mem_fd);
        return true;
    }

    if (!mem_fd) {
        mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (mem_fd < 0) {
            fprintf(stderr, "can't open /dev/mem\n");
            return false;
        }
    }

    volatile char *mapped_area;
    if (offset != loaded_offset) {
        mapped_area =
            mmap(NULL, // Any adddress in our space will do
                 size, // Map length
                 PROT_READ | PROT_WRITE, // Enable reading & writting to mapped memory
                 MAP_SHARED,     // Shared with other processes
                 mem_fd,         // File to map
                 offset          // Offset to base address
            );
        if (mapped_area == MAP_FAILED) {
            fprintf(stderr, "read_mem_reg mmap error: %s (%d)\n",
                    strerror(errno), errno);
            return false;
        }
        loaded_area = (char *)mapped_area;
        loaded_size = size;
        loaded_offset = offset;
    } else
        mapped_area = loaded_area;

    unsigned data = *(volatile uint32_t *)(mapped_area + (addr - offset));

    return data;
}


int main(int argc, char** argv)
{
    printf("PT_INTF_MOD\t = 0x%08x\n", read_mem_reg(0x20580100));
    printf("PT_BT656_CFG\t = 0x%08x\n", read_mem_reg(0x20580120));
    printf("PT_UNIFY_TIMING_CFG\t = 0x%08x\n", read_mem_reg(0x20580130));
    printf("PT_GEN_TIMING_CFG\t = 0x%08x\n", read_mem_reg(0x20580134));
    printf("PT_INTF_HFB\t = 0x%08x\n", read_mem_reg(0x20580180));
    printf("PT_INTF_HACT\t = 0x%08x\n", read_mem_reg(0x20580184));
    printf("PT_INTF_HBB\t = 0x%08x\n", read_mem_reg(0x20580188));
    printf("PT_INTF_VFB\t = 0x%08x\n", read_mem_reg(0x2058018c));
    printf("PT_INTF_VACT\t = 0x%08x\n", read_mem_reg(0x20580190));
    printf("PT_INTF_VBB\t = 0x%08x\n", read_mem_reg(0x20580194));
    printf("PT_INTF_VBFB\t = 0x%08x\n", read_mem_reg(0x20580198));
    printf("PT_INTF_VBACT\t = 0x%08x\n", read_mem_reg(0x2058019c));
    printf("PT_INTF_VBBB\t = 0x%08x\n", read_mem_reg(0x205801a0));
    printf("PT_FLASH_CFG\t = 0x%08x\n", read_mem_reg(0x205801b0));
    printf("PT_FLASH_CYC0\t = 0x%08x\n", read_mem_reg(0x205801c0));
    printf("PT_FLASH_CYC1\t = 0x%08x\n", read_mem_reg(0x205801c4));
    printf("PT_SHUTTER_CYC0\t = 0x%08x\n", read_mem_reg(0x205801d0));
    printf("PT_SHUTTER_CYC1\t = 0x%08x\n", read_mem_reg(0x205801d4));
    printf("PT_SHUTTER_CYC2\t = 0x%08x\n", read_mem_reg(0x205801d8));
    printf("PT_SHUTTER_CYC3\t = 0x%08x\n", read_mem_reg(0x205801d3));
    printf("PT_STATUS\t = 0x%08x\n", read_mem_reg(0x205801e0));
    printf("PT_BT656_STATUS\t = 0x%08x\n", read_mem_reg(0x205801e4));
//    printf("\t = 0x%08x\n", read_mem_reg(0x20580xxx));
//    printf("\t = 0x%08x\n", read_mem_reg(0x20580xxx));
//    printf("\t = 0x%08x\n", read_mem_reg(0x20580xxx));


}



