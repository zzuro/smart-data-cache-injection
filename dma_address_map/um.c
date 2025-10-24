#define _GNU_SOURCE

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
#include <sched.h>

#include "../go-pci-lib/libgopci.h"

#define WR_VALUE _IOW('a', 'a', __uint64_t *)
#define RD_VALUE _IOR('a', 'b', __uint64_t *)
#define ST_CPUID _IOW('a', 'c', __uint16_t *)
#define PRINT_ST_INFO _IOW('a', 'e', __u16 *)

#define ITERATIONS 50

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

static void hex_dump(const uint8_t *data, size_t length)
{
    size_t i;
    for (i = 0; i < length; i += 16)
    {

        printf("%08zx  ", i);

        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < length)
            {
                printf("%02x ", data[i + j]);
            }
            else
            {
                printf("   ");
            }
            if (j == 7)
                printf(" ");
        }

        printf(" |");
        for (size_t j = 0; j < 16 && i + j < length; j++)
        {
            uint8_t c = data[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("|\n");
    }
}

static uint16_t get_tag(int fd, uint8_t wrong_cpu)
{
    int cpu = sched_getcpu();
    if (cpu == -1)
    {
        perror("sched_getcpu");
        return 0;
    }

    if (wrong_cpu)
    {
        cpu += 10;
    }

    uint16_t tag = (uint16_t)cpu;
    if (ioctl(fd, ST_CPUID, &tag) < 0)
    {
        printf("IOCTL ST_CPUID Failed\n");
        return 0;
    }

    return tag;
}

int main(int argc, char *argv[])
{
    
    
    uint8_t wrong_cpu = 0;
    uint8_t no_ph = 0;
    uint64_t addr_in = 0;
    if (argc > 1 && strcmp(argv[1], "no_tag") == 0)
    {
        wrong_cpu = 1;
    }

    if (argc > 1 && strcmp(argv[1], "no_th") == 0)
    {
        no_ph = 1;
    }

    if (argc > 1 && strcmp(argv[1], "addr") == 0)
    {
        if (argc > 2) {
            addr_in = strtoull(argv[2], NULL, 0);
        } else {
            printf("Please provide an address after 'addr'\n");
            return 0;
        }

        printf("Using provided address: 0x%lx\n", addr_in);
    }

    int fd;
    fd = open("/dev/mapper_dev", O_RDWR);
    //fd = open("/dev/mem", O_RDWR | O_SYNC);
    //if (fd < 0)
    //{
    //    printf("Cannot open device file...\n");
    //    return 0;
    //}

    uint16_t cpu = (uint16_t)sched_getcpu();
    if (cpu == -1)
    {
        perror("sched_getcpu");
        return 0;
    }

    //if(ioctl(fd, PRINT_ST_INFO, &cpu) < 0)
    //{
    //    printf("IOCTL ST_CPUID Failed\n");
    //    return 0;
    //}


    size_t buffer = 4096;
    void *addr = mmap(NULL, buffer, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    //void *addr = mmap(NULL, buffer, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);
    //void *addr = mmap(NULL, buffer, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x84eb00000);
    if (addr == MAP_FAILED)
    {
        printf("Mapping Failed\n");
        return -1;
    }

    // Initial read to populate cache
    for (int i = 0; i < ITERATIONS; i++)
    {
        //measure_access_time((uint8_t *)addr);
    }

    //for core 2 tag 0, 1, 128, 129
    uint64_t time = 0;
    uint32_t res = 0;
    if (!addr_in)
    {
        for (int i = 0; i < ITERATIONS; i++)
        {
            //_mm_clflushopt((void *)addr);
            //_mm_mfence();
            //_mm_lfence();
            res = sdci(get_tag(fd, wrong_cpu));
            usleep(5000); // wait for request to be processed
            time += measure_access_time((uint8_t *)addr);
            if (res != *((uint32_t *)addr))
            {
                printf("Data mismatch! Expected: 0x%08x, Got: 0x%08x\n", res, *((uint32_t *)addr));
            }
        }
    }
    else
    {
        
        for (int i = 0; i < ITERATIONS; i++)
        {
            res = sdci_addr(1, addr_in); //tag 1 for L3 of core 2
            usleep(5000);
            //time += measure_access_time((uint8_t *)addr);
            //if (res != *((uint32_t *)addr))
            //{
            //    printf("Data mismatch! Expected: 0x%08x, Got: 0x%08x\n", res, *((uint32_t *)addr));
            //}
        }
    }
    printf("CPU cycles: %lu\n", time / ITERATIONS);
    munmap(addr, buffer);
    close(fd);
}