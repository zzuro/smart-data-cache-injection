#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


int main()
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    size_t size = 4096; // Size of the memory region to map
    void *map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x84eb00000);
    if (map == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    printf("Memory mapped at address %p\n", map);

    printf("Reading first 64 bytes:\n");
    for (size_t i = 0; i < 64; i++) {
        printf("%02x ", ((unsigned char *)map)[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");

    munmap(map, size);
    close(fd);
    return 0;
}