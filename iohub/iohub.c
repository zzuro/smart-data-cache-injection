#include <asm/amd/node.h>
#include <asm/apic.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>

#include "smn_registers.h"

#define BYTES_PER_LINE 16
static void __maybe_unused hexdump(unsigned long base_addr, const void *addr, size_t len)
{
    const unsigned char *ptr = addr;
    size_t i, j;

    for (i = 0; i < len; i += BYTES_PER_LINE)
    {
        u64 *u64ptr = (u64 *)(ptr + i);

        u64 first_val = u64ptr[0];
        u64 second_val = u64ptr[1];
        if ((first_val == 0xffffffffffffffff && second_val == 0xffffffffffffffff))
        {
            continue;
        }
        else
        {
            if ((first_val == 0 && second_val == 0))
            {
                continue;
            }
            else
            {
                pr_info("0x%016llx | 0x%016llx\n", first_val, second_val);
            }
        }

        printk(KERN_INFO "%08lx: ", (unsigned long)(base_addr + i));
        for (j = 0; j < BYTES_PER_LINE; j++)
        {
            if (i + j < len)
                printk(KERN_CONT "%02x ", ptr[i + j]);
            else
                printk(KERN_CONT "   ");
        }

        printk(KERN_CONT " | \n");
    }
}

static void __maybe_unused read_smn_check_error(int addr, int *val)
{
    int err = amd_smn_read(0, addr, val);
    if (err)
    {
        pr_err("error reading smn\n");
        return;
    }
    // pr_info("[+]read SMN ADDR: 0x%x Value 0x%x",addr, *val);
}

static void __maybe_unused write_smn_check_error(int addr, int val)
{
    int err = amd_smn_write(0, addr, val);
    if (err)
    {
        pr_err("error writing smn\n");
        return;
    }
    // pr_info("[+]write SMN ADDR: 0x%x Value 0x%x",addr, val);
}

static void read_write_iohub_registers(u32 ioms_bus)
{
    NB_TOP_OF_DRAM_SLOT1_STRUCT TOP_OF_DRAM;
    NB_LOWER_TOP_OF_DRAM2_STRUCT TOM2_LOWER_REG;
    NB_UPPER_TOP_OF_DRAM2_STRUCT TOM2_UPPER_REG;
    NB_TOP_OF_DRAM3_STRUCT TOM3_REG;
    u32 mmio_base, mmio_limit;

    u32 misc_data;

    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_TOP_OF_DRAM_SLOT1_ADDRESS, &TOP_OF_DRAM.Value);
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_LOWER_TOP_OF_DRAM2_ADDRESS, &TOM2_LOWER_REG.Value);
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_UPPER_TOP_OF_DRAM2_ADDRESS, &TOM2_UPPER_REG.Value);
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_TOP_OF_DRAM3_ADDRESS, &TOM3_REG.Value);

    TOP_OF_DRAM.Field.TOP_OF_DRAM = ((uint32_t)0xFF800000 & 0xFF800000) >> 23;
    write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_TOP_OF_DRAM_SLOT1_ADDRESS, TOP_OF_DRAM.Value);
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_TOP_OF_DRAM_SLOT1_ADDRESS, &TOP_OF_DRAM.Value);
    pr_info("TOP_OF_DRAM.Value = 0x%x\n", TOP_OF_DRAM.Value);

    // here is to change the size of DRAM
    unsigned long MsrData = 0x5F0000000000;
    unsigned long GnbTom2 = 0x000000FD00000000;
    unsigned long GnbTom3 = (MsrData - 1) & (uint64_t)0x000FFFFFFFC00000;

    TOM2_UPPER_REG.Field.UPPER_TOM2 = (uint32_t)((GnbTom2 >> 32) & 0xFF);
    TOM2_LOWER_REG.Field.LOWER_TOM2 = ((uint32_t)GnbTom2 & 0xFF800000) >> 23;
    TOM2_LOWER_REG.Field.ENABLE = 1;

    TOM3_REG.Field.TOM3_LIMIT = (uint32_t)(GnbTom3 >> 22);
    TOM3_REG.Field.TOM3_ENABLE = 1;

    write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_LOWER_TOP_OF_DRAM2_ADDRESS, TOM2_LOWER_REG.Value);
    write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_UPPER_TOP_OF_DRAM2_ADDRESS, TOM2_UPPER_REG.Value);
    write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_TOP_OF_DRAM3_ADDRESS, TOM3_REG.Value);

    // Read back and print the result
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_LOWER_TOP_OF_DRAM2_ADDRESS, &TOM2_LOWER_REG.Value);
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_UPPER_TOP_OF_DRAM2_ADDRESS, &TOM2_UPPER_REG.Value);
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_NB_TOP_OF_DRAM3_ADDRESS, &TOM3_REG.Value);

    unsigned long tom2_addr = ((unsigned long)TOM2_UPPER_REG.Field.UPPER_TOM2 << 32) |
                              ((unsigned long)TOM2_LOWER_REG.Field.LOWER_TOM2 << 23);
    pr_info("TOM2 address set to 0x%lx\n", tom2_addr);
    pr_info("TOM3_REG.Value = 0x%x\n", TOM3_REG.Value);
}

static void change_p2p_setting(u32 ioms_bus)
{
    u32 misc_data;
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_FEATURE_CNTL_ADDRESS, &misc_data);
    pr_info("Bus: %d old IOHC FEATURE CNTL: %x\n", ioms_bus, misc_data);

    misc_data &= ~0x6;
    // misc_data |= 0x3 << 1;
    write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_FEATURE_CNTL_ADDRESS, misc_data);
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_FEATURE_CNTL_ADDRESS, &misc_data);
    pr_info("Bus: %d new IOHC FEATURE CNTL: %x\n", ioms_bus, misc_data);
}

static void read_mmio_base_and_limit(u32 ioms_bus)
{
    u32 mmio_base, mmio_limit;

    read_smn_check_error((ioms_bus << 20) + IOHC_MMIO_BASE, &mmio_base);
    read_smn_check_error((ioms_bus << 20) + IOHC_MMIO_LIMIT, &mmio_limit);
    pr_info("IOHC MMIO Base: %x\n", mmio_base);
    pr_info("IOHC MMIO Limit: %x\n", mmio_limit);

    return;

    mmio_base = 0x43f00000 >> 16;
    mmio_limit = 0x744fffff >> 16;

    write_smn_check_error((ioms_bus << 20) + IOHC_MMIO_BASE, mmio_base);
    write_smn_check_error((ioms_bus << 20) + IOHC_MMIO_LIMIT, mmio_limit);

    read_smn_check_error((ioms_bus << 20) + IOHC_MMIO_BASE, &mmio_base);
    read_smn_check_error((ioms_bus << 20) + IOHC_MMIO_LIMIT, &mmio_limit);
    pr_info("IOHC MMIO Base: %x\n", mmio_base);
    pr_info("IOHC MMIO Limit: %x\n", mmio_limit);
}

static void read_dbg_base_and_limit(u32 ioms_bus)
{
    u32 misc_data;
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_DBG_BASE_ADDR_LO_ADDRESS, &misc_data);
    pr_info("DBG BASE ADDR LO: %x\n", misc_data);

    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_DBG_BASE_ADDR_HI_ADDRESS, &misc_data);
    pr_info("DBG BASE ADDR HI: %x\n", misc_data);

    u64 mmio_address = 0x300000000;
    u32 reg_low = (u32)(mmio_address & 0xffffffff);
    u32 reg_high = (u32)((mmio_address >> 32));

    /* if (ioms_bus == 0) */ {
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_DBG_BASE_ADDR_LO_ADDRESS, reg_low);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_DBG_BASE_ADDR_HI_ADDRESS, reg_high);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_DBG_BASE_ADDR_LO_ADDRESS, reg_low | 0x1);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_DBG_BASE_ADDR_LO_ADDRESS, reg_low | 0x3);

        read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_DBG_BASE_ADDR_LO_ADDRESS, &misc_data);
        pr_info("NEW IOHC DBG BASE ADDR LOW: %x\n", misc_data);

        read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_DBG_BASE_ADDR_HI_ADDRESS, &misc_data);
        pr_info("NEW IOHC DBG BASE ADDR HIGH: %x\n", misc_data);
    }
}

static void read_fastregctrl_base_and_limit(u32 ioms_bus)
{
    u32 misc_data;
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_LO_ADDRESS, &misc_data);
    pr_info("FASTREGCNTL BASE ADDR LO: %x\n", misc_data);

    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_HI_ADDRESS, &misc_data);
    pr_info("FASTREGCNTL BASE ADDR HI: %x\n", misc_data);

    if (ioms_bus == 0)
    {
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_LO_ADDRESS, 0xf6700000);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_HI_ADDRESS, 0x0);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_LO_ADDRESS, 0xf6700001);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_LO_ADDRESS, 0xf6700101);

        read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_LO_ADDRESS, &misc_data);
        pr_info("NEW IOHC FASTREGCNTL BASE ADDR LOW: %x\n", misc_data);

        read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREGCNTL_BASE_ADDR_HI_ADDRESS, &misc_data);
        pr_info("NEW IOHC FASTREGCNTL BASE ADDR HIGH: %x\n", misc_data);
    }
}

static void read_fastreg_base_and_limit(u32 ioms_bus)
{
    u32 misc_data;
    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_LO_ADDRESS, &misc_data);
    pr_info("FASTREG BASE ADDR LO: %x\n", misc_data);

    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_HI_ADDRESS, &misc_data);
    pr_info("FASTREG BASE ADDR HI: %x\n", misc_data);

    u64 mmio_address = 0x300000000;
    u32 reg_low = (u32)(mmio_address & 0xffffffff);
    u32 reg_high = (u32)((mmio_address >> 32));

    /* if (ioms_bus == 0) */ {
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_LO_ADDRESS, reg_low);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_HI_ADDRESS, reg_high);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_LO_ADDRESS, reg_low | 0x1);
        write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_LO_ADDRESS, reg_low | 0x3);

        read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_LO_ADDRESS, &misc_data);
        read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_HI_ADDRESS, &misc_data);
        pr_info("NEW IOHC FASTREG BASE ADDR LOW: %x ", misc_data);
        read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_FASTREG_BASE_ADDR_HI_ADDRESS, &misc_data);
        pr_cont("| BASE ADDR HIGH: %x\n", misc_data);
    }
}

static void read_psp_ccp_base_and_limit(u32 ioms_bus)
{
    u32 misc_data;

    read_smn_check_error((ioms_bus << 20) + SMU_BAR_LOW, &misc_data);
    pr_info("SMU BAR Low: %x\n", misc_data);

    read_smn_check_error((ioms_bus << 20) + SMU_BAR_HIGH, &misc_data);
    pr_info("SMU BAR High: %x\n", misc_data);

    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_LOW, &misc_data);
    pr_info("IOHC CCP BASE ADDR LOW: %x\n", misc_data);

    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_HIGH, &misc_data);
    pr_info("IOHC CCP BASE ADDR HIGH: %x\n", misc_data);

    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_PSP_BASE_ADDR_LOW, &misc_data);
    pr_info("IOHC PSP BASE ADDR LOW: %x\n", misc_data);

    read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_PSP_BASE_ADDR_HIGH, &misc_data);
    pr_info("IOHC PSP BASE ADDR HIGH: %x\n", misc_data);

    /*     if (ioms_bus == 0){
            write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_LOW, 0xf6500000);
            write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_HIGH, 0x0);
            write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_LOW, 0xf6500001);
            write_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_LOW, 0xf6500101);

            read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_LOW,&misc_data);
            pr_info("NEW IOHC CCP BASE ADDR LOW: %x\n", misc_data);

            read_smn_check_error((ioms_bus << 20) + SMN_IOHUB0NBIO0_IOHC_CCP_BASE_ADDR_HIGH,&misc_data);
            pr_info("NEW IOHC CCP BASE ADDR HIGH: %x\n", misc_data);
        } */

    return;
}

static void __maybe_unused enable_nosnoop_on_bus(void)
{
    u32 main_address = 0x13B30000 + 0x1004;
    u32 data;
    for (size_t i = 0; i < 0x4; i++)
    {
        read_smn_check_error((main_address + (i << 20)), &data);
        pr_info("SMN ADDR: 0x%08x DATA: 0x%08x\n", (main_address + (i << 20)), data);
        read_smn_check_error((main_address + (i << 20)) + 0x1000, &data);
        pr_info("SMN ADDR: 0x%08x DATA: 0x%08x\n", (main_address + (i << 20) + 0x1000), data);
        read_smn_check_error((main_address + (i << 20)) + 0x2000, &data);
        pr_info("SMN ADDR: 0x%08x DATA: 0x%08x\n", (main_address + (i << 20) + 0x2000), data);
        read_smn_check_error((main_address + (i << 20)) + 0x3000, &data);
        pr_info("SMN ADDR: 0x%08x DATA: 0x%08x\n", (main_address + (i << 20) + 0x3000), data);
    }
}

static void __maybe_unused smn_iohub_psp_a2c_ctrl_toggle(void)
{
    u32 smn_base = 0x1400000;
    u32 offset1 = 0x3a94;
    u32 offset2 = 0x3a98;
    u32 offset3 = 0x3AAC;
    u32 offset4 = 0x3AB0;
    u32 val;

    pr_info("SMN ADDR: 0x%08x ", smn_base + offset1);
    for (u32 ioms_bus = 0; ioms_bus < 4; ioms_bus += 2)
    {
        read_smn_check_error((ioms_bus << 20) + smn_base + offset1, &val);
        write_smn_check_error((ioms_bus << 20) + smn_base + offset1, val ^ 1);
        read_smn_check_error((ioms_bus << 20) + smn_base + offset1, &val);
        pr_cont("| 0x%08x", val);
    }
    pr_info("SMN ADDR: 0x%08x ", smn_base + offset2);
    for (u32 ioms_bus = 0; ioms_bus < 4; ioms_bus += 2)
    {
        read_smn_check_error((ioms_bus << 20) + smn_base + offset2, &val);
        write_smn_check_error((ioms_bus << 20) + smn_base + offset2, val ^ 1);
        read_smn_check_error((ioms_bus << 20) + smn_base + offset2, &val);
        pr_cont("| 0x%08x", val);
    }
    pr_info("SMN ADDR: 0x%08x ", smn_base + offset3);
    for (u32 ioms_bus = 0; ioms_bus < 4; ioms_bus += 2)
    {
        read_smn_check_error((ioms_bus << 20) + smn_base + offset3, &val);
        write_smn_check_error((ioms_bus << 20) + smn_base + offset3, val ^ 1);
        read_smn_check_error((ioms_bus << 20) + smn_base + offset3, &val);
        pr_cont("| 0x%08x", val);
    }
    pr_info("SMN ADDR: 0x%08x ", smn_base + offset4);
    for (u32 ioms_bus = 0; ioms_bus < 4; ioms_bus += 2)
    {
        read_smn_check_error((ioms_bus << 20) + smn_base + offset4, &val);
        write_smn_check_error((ioms_bus << 20) + smn_base + offset4, val ^ 1);
        read_smn_check_error((ioms_bus << 20) + smn_base + offset4, &val);
        pr_cont("| 0x%08x", val);
    }
    pr_cont("\n");
}

static void __maybe_unused output_timeout_registers(void)
{
    u32 smn_base = 0x1013A000;
    u32 val;

    for (u32 offset = 0x264; offset < 0x279; offset += 4)
    {
        pr_info("SMN ADDR: 0x%08x ", smn_base + offset);
        for (u32 ioms_bus = 0; ioms_bus < 4; ioms_bus++)
        {
            read_smn_check_error((ioms_bus << 20) + smn_base + offset, &val);
            pr_cont("| 0x%08x", val);
        }
        pr_cont("|\n");
    }
}

static void __maybe_unused smn_iohub_range_dump(void)
{
    u32 val;
    u32 smn_base = 0x1403A80;

    // write_smn_check_error(0x01403a94,0x52a82541);
    // write_smn_check_error(0x01403a90,0x52a82541);
    // write_smn_check_error(0x01403AAC,0x00280541);
    // write_smn_check_error(0x01403AB0,0x00280541);

    for (u32 offset = 0; offset < 0x100; offset += 4)
    {
        pr_info("SMN ADDR: 0x%08x ", smn_base + offset);
        for (u32 ioms_bus = 0; ioms_bus < 4; ioms_bus++)
        {
            read_smn_check_error((ioms_bus << 20) + smn_base + offset, &val);
            pr_cont("| 0x%08x", val);
        }
        pr_cont("|\n");
    }
}

static void __maybe_unused read_psp_coherency_setting(void)
{
    u32 val;
    read_smn_check_error(01400000 + 0x3AA8, &val);
    pr_info("PSP Coherency Setting : 0x%08x\n", val);
}

static int __init iohub_init(void)
{
    pr_info("iohub: Initializing the module\n");

    for (size_t i = 0; i < 0x4; i++)
    {
        read_write_iohub_registers(i);
    }

    return -1;
}

static void __exit iohub_exit(void)
{
    printk(KERN_INFO "iohub: Exiting the module\n");
    return;
}

module_init(iohub_init);
module_exit(iohub_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Kernel module to write a memory pointer to RAX and execute VMSAVE to a previously allocated page");
MODULE_VERSION("1.0");
