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

#include "defs.h"

#define WR_VALUE _IOW('a', 'a', __uint64_t *)
#define RD_VALUE _IOR('a', 'b', __uint64_t *)
#define ST_CPUID _IOW('a', 'c', __uint16_t *)
#define GET_PHYS_ADDR _IOW('a', 'd', __uint64_t *)

#define L1_CACHE_BYTES KB(48)
#define L1_WAYNESS 12

#define L2_CACHE_BYTES MB(1)
#define L2_WAYNESS 16

#define L3_CACHE_BYTES MB(32)
#define L3_WAYNESS 16

#define CACHE_LINE_SIZE 64


#define ITERATIONS 1000
#define BUFFER_SIZE MB(2)

#define L1_INDEX(addr) ((addr >> 6) & ((1 << 6) - 1))      // 6 bits
#define L2_INDEX(addr) ((addr >> 6) & ((1 << 10) - 1))     // 10 bits
#define L3_INDEX(addr) ((addr >> 6) & ((1 << 15) - 1))     // 15 bits

static int fd;

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


static void profile_L1()
{
    uint8_t *memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS , -1, 0);

    if (memory == MAP_FAILED) {
        perror("mmap");
        return;
    }

    for(int i = 0; i < ITERATIONS; i++)
    {
        measure_access_time(memory);
    }

    __uint64_t time = 0;
    for(int i = 0; i < ITERATIONS; i++)
    {
        time += measure_access_time(memory);
    }

    munmap(memory, BUFFER_SIZE);
    printf("L1 Cache Access Time: %lu cycles\n", time / ITERATIONS);
}

static void* get_pyhsical(void* addr)
{
    __uint64_t phys_addr = (__uint64_t)addr;
    if (ioctl(fd, GET_PHYS_ADDR, &phys_addr) < 0)
    {
        printf("IOCTL ST_CPUID Failed\n");
        return 0;
    }

    return (void*)phys_addr;
}

static void access_line(void* target){

    *(volatile uint64_t*)target;
    _mm_mfence();
    _mm_lfence();
}

static void evict_L1(uint8_t *memory, uint8_t* main)
{
    
    for(int i = BUFFER_SIZE - CACHE_LINE_SIZE; i >= 0; i -= CACHE_LINE_SIZE)
    {
        uintptr_t mask = 0xFC0;
        uintptr_t mem_bits = (uintptr_t)(memory + i) & mask;
        uintptr_t te1_bits = (uintptr_t)(main) & mask;

        if(mem_bits == te1_bits && main != memory + i)
        {
            access_line(memory + i);
        }
    }

    for(int i = 0; i < BUFFER_SIZE; i += CACHE_LINE_SIZE)
    {
        uintptr_t mask = 0xFC0;
        uintptr_t mem_bits = (uintptr_t)(memory + i) & mask;
        uintptr_t te1_bits = (uintptr_t)(main) & mask;

        if(mem_bits == te1_bits && main != memory + i)
        {
            access_line(memory + i);
        }
    }

    for(int i = BUFFER_SIZE - CACHE_LINE_SIZE; i >= 0; i -= CACHE_LINE_SIZE)
    {
        uintptr_t mask = 0xFC0;
        uintptr_t mem_bits = (uintptr_t)(memory + i) & mask;
        uintptr_t te1_bits = (uintptr_t)(main) & mask;

        if(mem_bits == te1_bits && main != memory + i)
        {
            access_line(memory + i);
        }
    }
}

static void profile_L2()
{
    uint8_t *memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS , -1, 0);

    if (memory == MAP_FAILED) {
        perror("mmap");
        return;
    }

    uint8_t* main = mmap(NULL, 64, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS , -1, 0);

     memset(memory, 0xAB, BUFFER_SIZE);
    _mm_mfence();
    _mm_lfence();

    memset(main, 0xCD, 64);
    _mm_mfence();
    _mm_lfence();

    __uint64_t time = 0;
    for(int i = 0; i < ITERATIONS; i++)
    {
        evict_L1(memory, main);
        time += measure_access_time(main);
    }

    munmap(memory, BUFFER_SIZE);
    munmap(main, 64);
    printf("L2 Cache Access Time: %lu cycles\n", time / ITERATIONS);
}

int main()
{
    fd = open("/dev/mapper_dev", O_RDWR);
    if (fd < 0)
    {
        printf("Cannot open device file...\n");
        return 0;
    }

    profile_L1();
    profile_L2();


    close(fd);

    return 0;
}