#pragma once

/* 6.2 Page States */
typedef enum snp_page_state
{                        // Controlled by:   SW       HW        SW   SW        HW   SW  HW
    // Page State                            Assigned Validated ASID Immutable Lock GPA VMSA
    SNP_PAGE_STATE_INVALID        = 0x0,
    SNP_PAGE_STATE_DEFAULT        = 0x1,
    SNP_PAGE_STATE_HYPERVISOR     = 0x2,  // 0        0         0    0         -    -   -
    SNP_PAGE_STATE_HV_FIXED       = 0x3,  // 0        0         0    1         0    -   -
    SNP_PAGE_STATE_FIRMWARE       = 0x4,  // 1        0         0    1         -    0   0
    SNP_PAGE_STATE_FIRMWARE_IOMMU = 0x5,  // 1        0         0    1         -    1   0
    SNP_PAGE_STATE_RECLAIM        = 0x6,  // 1        0         0    0         -    -   -
    SNP_PAGE_STATE_CONTEXT        = 0x7,  // 1        0         0    1         -    0   1
    SNP_PAGE_STATE_METADATA       = 0x8,  // 1        0         0    1         -    >0  -
    SNP_PAGE_STATE_PRE_GUEST      = 0x9,  // 1        0         >0   1         -    -   -
    SNP_PAGE_STATE_PRE_SWAP       = 0xA,  // 1        1         >0   1         -    -   -
    SNP_PAGE_STATE_GUEST_INVALID  = 0xB,  // 1        0         >0   0         -    -   -
    SNP_PAGE_STATE_GUEST_VALID    = 0xC,  // 1        1         >0   0         -    -   -
    SNP_PAGE_STATE_LIMIT,
} snp_page_state_t;


typedef enum RMP_GPA_STATE
{                                  // snp_page_state_t
    RMP_GPA_STATE_FIRMWARE = 0x0,  // SNP_PAGE_STATE_FIRMWARE
    RMP_GPA_STATE_METADATA = 0x1,  // SNP_PAGE_STATE_METADATA
    RMP_GPA_STATE_FW_IOMMU = 0x2,  // SNP_PAGE_STATE_FIRMWARE_IOMMU
    RMP_GPA_STATE_LIMIT,
} RMP_GPA_STATE_T;


#define RMP_ENTRY_GPA_SHIFT                 (12ULL)
#define RMP_GPA_STATE_SHIFT                 (36ULL)    // Bits [38:36]
#define RMP_GPA_STATE_FLAG                  (0x7ULL << RMP_GPA_STATE_SHIFT)

#define PADDR_GET_GPA(paddr)                (((uint64_t)(paddr) & ~SEV_CBIT_MASK) >> RMP_ENTRY_GPA_SHIFT)
#define PADDR_TO_GPA_FIELD(paddr, state)    (PADDR_GET_GPA(paddr) | ((uint64_t)state << RMP_GPA_STATE_SHIFT))
#define RMP_GET_GPA_STATE(gpa)              (RMP_GPA_STATE_T)(((uint64_t)gpa) >> RMP_GPA_STATE_SHIFT)
#define RMP_GET_GPA(gpa)                    ((uint64_t)gpa & ~(RMP_GPA_STATE_FLAG))
#define RMP_GPA_TO_PADDR(gpa)               ((uint64_t)gpa << RMP_ENTRY_GPA_SHIFT)


typedef struct rmp_fields
{
    uint64_t assigned      : 1;
    uint64_t page_size     : 1;
    uint64_t immutable     : 1;
    uint64_t subpage_count : 9;
    uint64_t gpa           : 39;
    uint64_t asid          : 10;
    uint64_t vmsa          : 1;
    uint64_t validated     : 1;
    uint64_t lock          : 1;

} rmp_fields_t;


typedef union rmp_quadword1
{
    rmp_fields_t f;
    uint64_t     val;
} rmp_quadword1_t;

typedef union vmpl_perm_mask
{
    struct
    {
        uint32_t read            : 1;   /* If page is readable by the VMPL */
        uint32_t write           : 1;   /* If page is writable by the VMPL */
        uint32_t exec_user       : 1;   /* If page is executable by the VMPL at CPL3 */
        uint32_t exec_supervisor : 1;   /* If page is executable by the VMPL at CPL0, CPL1, or CPL2 */
        uint32_t sss             : 1;   /* If page can be used by the VMPL as a Supervisor Shadow Stack page */
        uint32_t reserved        : 3;
    } __attribute__((packed)) f;
    uint8_t val;
} __attribute__((packed)) vmpl_perm_mask_t;


typedef union rmp_quadword2
{
    struct
    {
        vmpl_perm_mask_t vmpl0;
        vmpl_perm_mask_t vmpl1;
        vmpl_perm_mask_t vmpl2;
        vmpl_perm_mask_t vmpl3;
        uint32_t implicit_key_select : 1;   /* bit 32        */
        uint32_t reserved            : 16;  /* bits 33 to 48 */
        uint32_t root_port_id        : 5;   /* bit 49 to 53  */
        uint32_t pcie_ide_stream_id  : 8;   /* bit 54 to 61  */
        uint32_t page_migration      : 2;   /* bits 62 to 63 */
    } __attribute__((packed)) f;
    uint64_t val;
} __attribute__((packed)) rmp_quadword2_t;


typedef struct rmp_entry
{
    rmp_quadword1_t q1;
    rmp_quadword2_t q2;
} rmp_entry_t;

static snp_page_state_t rmp_entry_get_state(rmp_entry_t *entry)
{
    snp_page_state_t result = SNP_PAGE_STATE_INVALID;
    rmp_fields_t *f = &entry->q1.f;

    if (f->assigned == 0 && f->validated == 0 && f->asid == 0 && f->immutable == 0)
    {
        result = SNP_PAGE_STATE_HYPERVISOR;
    }
    else if (f->assigned == 0 && f->validated == 0 && f->asid == 0 && f->immutable == 1)
    {
        result = SNP_PAGE_STATE_HV_FIXED;
    }
    else if (f->assigned == 1 && f->validated == 0 && f->asid == 0 && f->immutable == 1 &&
             f->gpa == 0 && f->vmsa == 0)
    {
        result = SNP_PAGE_STATE_FIRMWARE;
    }
    else if (f->assigned == 1 && f->validated == 0 && f->asid == 0 && f->immutable == 0 &&
             f->lock == 0)
    {
        result = SNP_PAGE_STATE_RECLAIM;
    }
    else if (f->assigned == 1 && f->validated == 0 && f->asid == 0 && f->immutable == 1 &&
             f->gpa == 0 && f->vmsa == 1)
    {
        result = SNP_PAGE_STATE_CONTEXT;
    }
    else if (f->assigned == 1 && f->validated == 0 && f->asid == 0 && f->immutable == 1 &&
             RMP_GET_GPA(f->gpa) > 0 && RMP_GET_GPA_STATE(f->gpa) == RMP_GPA_STATE_METADATA)
    {
        result = SNP_PAGE_STATE_METADATA;
    }
    else if (f->assigned == 1 && f->validated == 0 && f->asid > 0 && f->immutable == 1)
    {
        result = SNP_PAGE_STATE_PRE_GUEST;
    }
    else if (f->assigned == 1 && f->validated == 1 && f->asid > 0 && f->immutable == 1)
    {
        result = SNP_PAGE_STATE_PRE_SWAP;
    }
    else if (f->assigned == 1 && f->validated == 0 && f->asid > 0 && f->immutable == 0)
    {
        result = SNP_PAGE_STATE_GUEST_INVALID;
    }
    else if (f->assigned == 1 && f->validated == 1 && f->asid > 0 && f->immutable == 0)
    {
        result = SNP_PAGE_STATE_GUEST_VALID;
    }
    else if (f->assigned == 1 && f->validated == 0 && f->asid == 0 && f->immutable == 1 &&
             RMP_GET_GPA(f->gpa) == 1 && RMP_GET_GPA_STATE(f->gpa) == RMP_GPA_STATE_FW_IOMMU && f->vmsa == 0)
    {
        result = SNP_PAGE_STATE_FIRMWARE_IOMMU;
    }
    else
    {
        result = SNP_PAGE_STATE_INVALID;
    }

    return result;
}

