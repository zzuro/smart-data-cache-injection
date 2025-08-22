#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <x86intrin.h>

#define WR_VALUE _IOW('a', 'a', __uint64_t *)
#define RD_VALUE _IOR('a', 'b', __uint64_t *)
#define MAP_ADDR _IOW('a', 'c', __uint64_t *)

static uint64_t measure_access_time(uint8_t *addr)
{
    uint64_t start, end;
    uint32_t unused;

    _mm_mfence();
    _mm_lfence();

    start = __rdtscp(&unused);
    _mm_lfence();

    *(volatile char *)addr;

    _mm_lfence();

    end = __rdtscp(&unused);

    _mm_mfence();

    return end - start;
}

static void hex_dump(const uint8_t *data, size_t length) {
    size_t i;
    for (i = 0; i < length; i += 16) {
       
        printf("%08zx  ", i);

        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                printf("%02x ", data[i + j]);
            } else {
                printf("   "); 
            }
            if (j == 7) printf(" "); 
        }

        printf(" |");
        for (size_t j = 0; j < 16 && i + j < length; j++) {
            uint8_t c = data[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("|\n");
    }
}


int main()
{
    int fd;
    fd = open("/dev/mapper_dev", O_RDWR);
    if (fd < 0)
    {
        printf("Cannot open device file...\n");
        return 0;
    }

    size_t buffer = 4096;
    void* addr = mmap(NULL, buffer, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    printf("Access time: %lu CPU cycles\n", measure_access_time((uint8_t*)addr));
    printf("Bytes: 0x%08x", *((uint32_t*)addr));

    //hex_dump((const uint8_t*)addr, buffer);

    munmap(addr, buffer);
    close(fd);

}