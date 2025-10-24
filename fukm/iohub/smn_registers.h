#include <linux/types.h>

/***********************************************************
* Register Name : NB_TOP_OF_DRAM3
* Register Description :
* Top of Memory 3.
* Visibility : 0x2
************************************************************/
typedef union {
  struct {
    uint32_t                            TOM3_LIMIT:30;
    uint32_t                            Reserved_30_30:1;
    uint32_t                            TOM3_ENABLE:1;
  } Field;
  uint32_t Value;
} NB_TOP_OF_DRAM3_STRUCT;

#define SMN_IOHUB0NBIO0_NB_TOP_OF_DRAM3_ADDRESS    0x13b10138UL

/***********************************************************
* Register Name : NB_TOP_OF_DRAM_SLOT1
* Register Description :
* North Bridge Memory Base Register
* Visibility : 0x2
************************************************************/
typedef union {
  struct {
    uint32_t                            TOP_OF_DRAM_BIT_32:1;
    uint32_t                            Reserved_22_1:22;
    uint32_t                            TOP_OF_DRAM:9;
  } Field;
  uint32_t Value;
} NB_TOP_OF_DRAM_SLOT1_STRUCT;

#define SMN_IOHUB0NBIO0_NB_TOP_OF_DRAM_SLOT1_ADDRESS    0x13b00090UL

/***********************************************************
* Register Name : NB_UPPER_TOP_OF_DRAM2
* Register Description :
* Top of Memory 2 High.
* Visibility : 0x2
************************************************************/
typedef union {
  struct {
    uint32_t                            UPPER_TOM2:9;
    uint32_t                            Reserved_31_9:23;
  } Field;
  uint32_t Value;
} NB_UPPER_TOP_OF_DRAM2_STRUCT;

#define SMN_IOHUB0NBIO0_NB_UPPER_TOP_OF_DRAM2_ADDRESS    0x13b10068UL

typedef union {
  struct {
    uint32_t                            ENABLE:1;
    uint32_t                            Reserved_22_1:22;
    uint32_t                            LOWER_TOM2:9;
  } Field;
  uint32_t Value;
} NB_LOWER_TOP_OF_DRAM2_STRUCT;

#define SMN_IOHUB0NBIO0_NB_LOWER_TOP_OF_DRAM2_ADDRESS    0x13b10064UL


#define IOHC_MMIO_BASE    0x13b1005CUL
#define IOHC_MMIO_LIMIT    0x13b10060UL




// IOHCMISC[0...3]x00000034 (IOHC::CFG_IOHC_PCI)
#define IOMMU_DISABLE_REGISTER       0x13b10034UL

#define SMU_BAR_LOW                0x13b102e8UL
#define SMU_BAR_HIGH               0x13b102ecUL


// IOHCMISC[0...3]x00000118 (IOHC::IOHC_FEATURE_CNTL)
/*
P2P_mode. Read-write. Reset: 1h.
Description: Peer to peer mode:
00 - Mode 0 : DMA writes not matching a valid P2P range are dropped (Legacy operation)
01 - Mode 1 : DMA writes not matching a valid P2P range are forwarded to the DF
10 - Mode 2 : all DMA writes are forwarded to the DF
11 - Mode 3: Disable P2P
*/
#define SMN_IOHUB0NBIO0_IOHC_FEATURE_CNTL_ADDRESS  0x13b10118UL


#define SMN_IOHUB0NBIO0_IOHC_FEATURE_CNTL2_ADDRESS  0x13b10130UL


#define SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_LOW  0x13b102D8UL
#define SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_HIGH 0x13b102DCUL


#define SMN_IOHUB0NBIO0_IOHC_PSP_BASE_ADDR_LOW 0x13b102E0UL
#define SMN_IOHUB0NBIO0_IOHC_PSP_BASE_ADDR_HIGH 0x13b102E4UL


#define SMN_IOHUB0NBIO0_DBG_BASE_ADDR_HI_ADDRESS           0x13b102fcUL
#define SMN_IOHUB0NBIO0_DBG_BASE_ADDR_LO_ADDRESS           0x13b102f8UL


#define SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_LO_ADDRESS   0x13b10308UL
#define SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_HI_ADDRESS   0x13b1030cUL

#define SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_LO_ADDRESS       0x13b10300UL
#define SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_HI_ADDRESS       0x13b10304UL
