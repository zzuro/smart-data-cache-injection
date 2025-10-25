#include "df.h"
#include <asm/amd/nb.h>
#include <linux/pci.h>


#define SMN_REQUEST_REG_OPENSIL  0x00B8
#define SMN_RESPONSE_REG_OPENSIL  0x00BC

#define SMN_REQUEST_REG_OS  0x60
#define SMN_RESPONSE_REG_OS  0x64

#define THREAD_ENABLE_SMN       0x30081018UL
#define THREAD_ENABLE_SMN2      0x4a081018UL /* substract ccd -8 only valid if ccd >= 8*/

#define THREAD_DISABLE_SMN       0x30081020UL
#define THREAD_DISABLE_SMN2      0x4a081020UL /* substract ccd -8 only valid if ccd >= 8*/

#define SMN_SMU_PSP_ARGUMENT 0x03b10700ul  
#define SMN_SMU_MESSAGE 0x03b10718ul
#define SMN_SMU_RESPONSE 0x03b10704ul


#define SIL_RESERVED_815                0x5D000
#define SIL_RESERVED_807                (SIL_RESERVED_815 + 0x3B8)
#define DFX_FUSE_CCD_PRESENT_1_OFFSET   22
#define DFX_FUSE_CCD_PRESENT_1_MASK     0xFFC00000
#define DFX_CCD_PRESENT_2_ADDR          (SIL_RESERVED_815 + 0x3BC)
#define SIL_RESERVED_811                (SIL_RESERVED_815 + 0x3BC)


typedef union {
  struct {
    uint32_t :22; // Reserved
    uint32_t CcdPresent :10;
  } Field;
  uint32_t Value;
} DFX_FUSE_CCD_PRESENT_1;

typedef union {
  struct {
    uint32_t CcdPresent :2;
    uint32_t :30; // Reserved
  } Field;
  uint32_t Value;
} DFX_CCD_PRESENT_2;

typedef union {
  struct {
    uint32_t :2; // Reserved
    uint32_t CcdDown :12;
    uint32_t :18; // Reserved
  } Field;
  uint32_t Value;
} DFX_FUSE_CCD_DOWN;

typedef union {
  struct {
    uint32_t Mp5CoreDisable :8;
    uint32_t Mp5SmtEnable :1;
    uint32_t :8; // Reserved
    uint32_t Mp5Ccx1Disable :8;
    uint32_t :7; // Reserved
  } Field;
  uint32_t Value;
} DFX_CORE_FUSE;

///
typedef union {
  struct {
    uint32_t :13; // Reserved
    uint32_t Mp0UmcCtrlXHarvest :12;
    uint32_t :7; // Reserved
  } Field;
  uint32_t Value;
} DFX_UMC_FUSE;

void read_disable_values(void);
void read_core_status(void);
void psp_write_smn_mailbox(void);
void disable_cpu_core(void);
void enable_cpu_core(void);
uint32_t smn_main(void);

#define SMN_RW_PROT(LEVEL) \
int amd_smn_rw_##LEVEL(u16 node, u32 address, u32 *value, bool write);

SMN_RW_PROT(OPENSIL)
SMN_RW_PROT(OS)