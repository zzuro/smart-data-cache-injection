#pragma once

#define IOMMU_READ	    (1 << 0)
#define IOMMU_WRITE	    (1 << 1)
#define IOMMU_CACHE	    (1 << 2) /* DMA cache coherency */
#define IOMMU_NOEXEC	(1 << 3)
#define IOMMU_MMIO	    (1 << 4) /* e.g. things like MSI doorbells */
#define IOMMU_PRIV	    (1 << 5)


#define _ptr(val) ((void *) (unsigned long) (val))
#define _ul(val)  ((unsigned long) (val))
#define _u(val)   ((unsigned int) (val))
#define _int(val) ((int) (val))

#define KB(x) (_ul(x) << 10)
#define MB(x) (_ul(x) << 20)
#define GB(x) (_ul(x) << 30)


typedef struct buffer {
    volatile void* vaddr;  // Virtual address of the buffer
    __u64 paddr;  // Physical address of the buffer
    __u64 size;   // Size of the buffer in bytes
} buffer_t;

typedef struct mapping {
    __u64 iova;             // I/O Virtual Address
    buffer_t buffer;        // Buffer information
    __u64 flags;            // Flags for the mapping (e.g., read/write permissions)
} mapping_t;