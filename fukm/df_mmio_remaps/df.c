#include "df.h"

#include <asm/amd/nb.h>
#include <linux/bitfield.h>
#include <linux/kernel.h>

#include "smn.h"

#define PCI_CONF1_ADDRESS(bus, devfn, reg) \
    (0x80000000 | ((reg & 0xF00) << 16) | (bus << 16) | (devfn << 8) | (reg & 0xFC))
#define PCI_DEVFN(slot, func) ((((slot) & 0x1f) << 3) | ((func) & 0x07))

static uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint16_t reg)
{
    uint32_t address = 0;
    uint32_t value = 0;
    uint32_t devfn = PCI_DEVFN(device, function);
    // Construct the configuration address
    address = (uint32_t)PCI_CONF1_ADDRESS(bus, devfn, reg);
    // Write the address to the PCI_CONFIG_ADDRESS (0xCF8) port
    outl(address, 0xcf8);
    // Read the value from the PCI_CONFIG_DATA (0xCFC) port
    value = inl(0xcfc);
    return value;
}

static void pci_config_write32(uint8_t bus, uint8_t device, uint8_t function, uint16_t reg, uint32_t value)
{
    uint32_t address = 0;
    uint32_t devfn = PCI_DEVFN(device, function);
    // Construct the configuration address
    address = (uint32_t)PCI_CONF1_ADDRESS(bus, devfn, reg);
    // Write the address to the PCI_CONFIG_ADDRESS (0xCF8) port
    outl(address, 0xcf8);
    // write the value to the PCI_CONFIG_DATA (0xCFC) port
    outl(value, 0xcfc);
}

static struct pci_dev *bus_dev_func_to_pci_dev(u8 bus, u8 dev, u8 func)
{
    struct pci_bus *pci_bus;
    struct pci_dev *pdev = NULL;
    unsigned int devfn = PCI_DEVFN(dev, func);

    /* Iterate over all PCI buses in the system */
    list_for_each_entry(pci_bus, &pci_root_buses, node)
    {
        /* Check if this is the bus we're looking for */
        if (pci_bus->number == bus)
        {
            /* Iterate over devices on this bus to find the matching devfn */
            pdev = pci_get_slot(pci_bus, devfn);
            if (pdev)
            {
                /* Found the device */
                return pdev;
            }
        }
    }
    /* Return NULL if the device wasn't found */
    return NULL;
}

#define DF_READ_PORT(LEVEL)                                                                  \
    int __df_indirect_read_port_##LEVEL(u16 node, u8 func, u16 reg, u8 instance_id, u32 *lo) \
    {                                                                                        \
        struct pci_dev *F4;                                                                  \
        int err = -ENODEV;                                                                   \
        u32 ficaa = 0;                                                                       \
        F4 = bus_dev_func_to_pci_dev(0, 0x18, 0x4);                                          \
        if (!F4)                                                                             \
            return err;                                                                      \
        if (instance_id != DF_BROADCAST)                                                     \
        {                                                                                    \
            ficaa |= FIELD_PREP(DF_FICAA_INST_EN, 1);                                        \
            ficaa |= FIELD_PREP(DF_FICAA_INST_ID, instance_id);                              \
        }                                                                                    \
        reg >>= 2;                                                                           \
        ficaa |= FIELD_PREP(DF_FICAA_REG_NUM, reg);                                          \
        ficaa |= FIELD_PREP(DF_FICAA_FUNC_NUM, func);                                        \
        pci_config_write32(0, 0x18, 0x4, FICAA_OFFSET_##LEVEL, ficaa);                       \
        *lo = pci_config_read32(0, 0x18, 0x4, FICAD_OFFSET_##LEVEL);                         \
        return err;                                                                          \
    }

#define DF_READ(LEVEL)                                                                  \
    int __df_indirect_read_##LEVEL(u16 node, u8 func, u16 reg, u8 instance_id, u32 *lo) \
    {                                                                                   \
        struct pci_dev *F4;                                                             \
        int err = -ENODEV;                                                              \
        u32 ficaa = 0;                                                                  \
        F4 = node_to_amd_nb(node)->link;                                                \
        if (!F4)                                                                        \
            return err;                                                                 \
        if (instance_id != DF_BROADCAST)                                                \
        {                                                                               \
            ficaa |= FIELD_PREP(DF_FICAA_INST_EN, 1);                                   \
            ficaa |= FIELD_PREP(DF_FICAA_INST_ID, instance_id);                         \
        }                                                                               \
        reg >>= 2;                                                                      \
        ficaa |= FIELD_PREP(DF_FICAA_REG_NUM, reg);                                     \
        ficaa |= FIELD_PREP(DF_FICAA_FUNC_NUM, func);                                   \
        err = pci_write_config_dword(F4, FICAA_OFFSET_##LEVEL, ficaa);                  \
        if (err)                                                                        \
        {                                                                               \
            pr_warn("Error writing DF Indirect FICAA, FICAA=0x%x\n", ficaa);            \
            return err;                                                                 \
        }                                                                               \
        err = pci_read_config_dword(F4, FICAD_OFFSET_##LEVEL, lo);                      \
        if (err)                                                                        \
            pr_warn("Error reading DF Indirect FICAD LO, FICAA=0x%x.\n", ficaa);        \
        pr_debug("node=%u inst=0x%x func=0x%x reg=0x%x val=0x%x",                       \
                 node, instance_id, func, reg << 2, *lo);                               \
        return err;                                                                     \
    }

#define DF_WRITE_PORT(LEVEL)                                                                 \
    int __df_indirect_write_port_##LEVEL(u16 node, u8 func, u16 reg, u8 instance_id, u32 lo) \
    {                                                                                        \
        struct pci_dev *F4;                                                                  \
        int err = -ENODEV;                                                                   \
        u32 ficaa = 0;                                                                       \
        F4 = bus_dev_func_to_pci_dev(0, 0x18, 0x4);                                          \
        if (!F4)                                                                             \
            return err;                                                                      \
        if (instance_id != DF_BROADCAST)                                                     \
        {                                                                                    \
            ficaa |= FIELD_PREP(DF_FICAA_INST_EN, 1);                                        \
            ficaa |= FIELD_PREP(DF_FICAA_INST_ID, instance_id);                              \
        }                                                                                    \
        reg >>= 2;                                                                           \
        ficaa |= FIELD_PREP(DF_FICAA_REG_NUM, reg);                                          \
        ficaa |= FIELD_PREP(DF_FICAA_FUNC_NUM, func);                                        \
        pci_config_write32(0, 0x18, 0x4, FICAA_OFFSET_##LEVEL, ficaa);                       \
        pci_config_write32(0, 0x18, 0x4, FICAD_OFFSET_##LEVEL, lo);                          \
        return err;                                                                          \
    }

#define DF_WRITE(LEVEL)                                                                 \
    int __df_indirect_write_##LEVEL(u16 node, u8 func, u16 reg, u8 instance_id, u32 lo) \
    {                                                                                   \
        struct pci_dev *F4;                                                             \
        int err = -ENODEV;                                                              \
        u32 ficaa = 0;                                                                  \
        F4 = node_to_amd_nb(node)->link;                                                \
        if (!F4)                                                                        \
            return err;                                                                 \
        if (instance_id != DF_BROADCAST)                                                \
        {                                                                               \
            ficaa |= FIELD_PREP(DF_FICAA_INST_EN, 1);                                   \
            ficaa |= FIELD_PREP(DF_FICAA_INST_ID, instance_id);                         \
        }                                                                               \
        reg >>= 2;                                                                      \
        ficaa |= FIELD_PREP(DF_FICAA_REG_NUM, reg);                                     \
        ficaa |= FIELD_PREP(DF_FICAA_FUNC_NUM, func);                                   \
        err = pci_write_config_dword(F4, FICAA_OFFSET_##LEVEL, ficaa);                  \
        if (err)                                                                        \
        {                                                                               \
            pr_warn("Error writing DF Indirect FICAA, FICAA=0x%x\n", ficaa);            \
            return err;                                                                 \
        }                                                                               \
        err = pci_write_config_dword(F4, FICAD_OFFSET_##LEVEL, lo);                     \
        if (err)                                                                        \
            pr_warn("Error reading DF Indirect FICAD LO, FICAA=0x%x.\n", ficaa);        \
        pr_debug("node=%u inst=0x%x func=0x%x reg=0x%x val=0x%x",                       \
                 node, instance_id, func, reg << 2, lo);                                \
        return err;                                                                     \
    }

DF_READ(PSP)
DF_READ(SMU)
DF_READ(MICROCODE)
DF_READ(BIOS_OS)
DF_READ(LW_SMI)
DF_READ(RES)
DF_WRITE(PSP)
DF_WRITE(SMU)
DF_WRITE(MICROCODE)
DF_WRITE(BIOS_OS)
DF_WRITE(LW_SMI)
DF_WRITE(RES)

DF_READ_PORT(PSP)
DF_READ_PORT(SMU)
DF_READ_PORT(MICROCODE)
DF_READ_PORT(BIOS_OS)
DF_READ_PORT(LW_SMI)
DF_READ_PORT(RES)
DF_WRITE_PORT(PSP)
DF_WRITE_PORT(SMU)
DF_WRITE_PORT(MICROCODE)
DF_WRITE_PORT(BIOS_OS)
DF_WRITE_PORT(LW_SMI)
DF_WRITE_PORT(RES)

void read_all_levels(u8 instance, u8 func, u16 reg)
{
    u32 lo = 0;
    __df_indirect_read_PSP(0, func, reg, instance, &lo);
    pr_info("PSP read 0x%x\n", lo);
    __df_indirect_read_SMU(0, func, reg, instance, &lo);
    pr_info("SMU read 0x%x\n", lo);
    __df_indirect_read_MICROCODE(0, func, reg, instance, &lo);
    pr_info("MICROCODE read 0x%x\n", lo);
    __df_indirect_read_BIOS_OS(0, func, reg, instance, &lo);
    pr_info("BIOS_OS read 0x%x\n", lo);
    __df_indirect_read_LW_SMI(0, func, reg, instance, &lo);
    pr_info("LW_SMI read 0x%x\n", lo);
    __df_indirect_read_RES(0, func, reg, instance, &lo);
    pr_info("RES read 0x%x\n", lo);
}

void read_all_levels_port(u8 instance, u8 func, u16 reg)
{
    u32 lo = 0;
    __df_indirect_read_port_PSP(0, func, reg, instance, &lo);
    pr_info("PSP port read 0x%x\n", lo);
    __df_indirect_read_port_SMU(0, func, reg, instance, &lo);
    pr_info("SMU port read 0x%x\n", lo);
    __df_indirect_read_port_MICROCODE(0, func, reg, instance, &lo);
    pr_info("MICROCODE port read 0x%x\n", lo);
    __df_indirect_read_port_BIOS_OS(0, func, reg, instance, &lo);
    pr_info("BIOS_OS port read 0x%x\n", lo);
    __df_indirect_read_port_LW_SMI(0, func, reg, instance, &lo);
    pr_info("LW_SMI port read 0x%x\n", lo);
    __df_indirect_read_port_RES(0, func, reg, instance, &lo);
    pr_info("RES port read 0x%x\n", lo);
}

void write_all_levels(u8 instance, u8 func, u16 reg, u32 lo)
{
    __df_indirect_write_PSP(0, func, reg, instance, lo);
    pr_info("PSP write 0x%x\n", lo);
    __df_indirect_write_SMU(0, func, reg, instance, lo);
    pr_info("SMU write 0x%x\n", lo);
    __df_indirect_write_MICROCODE(0, func, reg, instance, lo);
    pr_info("MICROCODE write 0x%x\n", lo);
    __df_indirect_write_BIOS_OS(0, func, reg, instance, lo);
    pr_info("BIOS_OS write 0x%x\n", lo);
    __df_indirect_write_LW_SMI(0, func, reg, instance, lo);
    pr_info("LW_SMI write 0x%x\n", lo);
    __df_indirect_write_RES(0, func, reg, instance, lo);
    pr_info("RES write 0x%x\n", lo);
}

void write_all_levels_port(u8 instance, u8 func, u16 reg, u32 lo)
{
    __df_indirect_write_port_PSP(0, func, reg, instance, lo);
    pr_info("PSP port write 0x%x\n", lo);
    __df_indirect_write_port_SMU(0, func, reg, instance, lo);
    pr_info("SMU port write 0x%x\n", lo);
    __df_indirect_write_port_MICROCODE(0, func, reg, instance, lo);
    pr_info("MICROCODE port write 0x%x\n", lo);
    __df_indirect_write_port_BIOS_OS(0, func, reg, instance, lo);
    pr_info("BIOS_OS port write 0x%x\n", lo);
    __df_indirect_write_port_LW_SMI(0, func, reg, instance, lo);
    pr_info("LW_SMI port write 0x%x\n", lo);
    __df_indirect_write_port_RES(0, func, reg, instance, lo);
    pr_info("RES port write 0x%x\n", lo);
}

bool use_smn = false;

static void print_base_and_limit(uint32_t number)
{

    if (number != 4)
        return;

    uint32_t value, value2;
    __df_indirect_read_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOLIMITADDRESS_0_REG + 0x10 * number, 0x20, &value2);
    __df_indirect_read_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOBASEADDRESS_0_REG + 0x10 * number, 0x20, &value);
    uint64_t base, limit;
    base = ((u64)value) << 16;
    limit = ((u64)value2) << 16 | 0xFFFF;

    pr_info("MMIO Region %2d:\n", number);
    pr_info("  Base Address : 0x%016llx\n", base);
    pr_info("  Limit Address: 0x%016llx\n", limit);

    MMIO_ADDRESS_CONTROL_REGISTER control;
    __df_indirect_read_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOADDRESSCONTROL_0_REG + 0x10 * number, 0x20, &control.Value);
    pr_info("  Flags:\n");
    pr_info("    RE        : %d\n", control.Field.RE);
    pr_info("    WE        : %d\n", control.Field.WE);
    pr_info("    CpuDis    : %d\n", control.Field.CpuDis);
    pr_info("    Np        : %d\n", control.Field.Np);
    pr_info("    DstFabricID: 0x%x\n", control.Field.DstFabricID);
}

static void spoof_dram_limit(void)
{
    // D18F7x208 DRAM LIMIT
    DRAM_BASE_ADDRESS_REGISTER control;
    __df_indirect_read_BIOS_OS(0, DRAMLIMITADDRESS_0_FUNC_ZEN5, DRAMLIMITADDRESS_0_REG_ZEN5, 0x3, &control.Value);
    pr_info("BEFORE DramLimitAddr 0x%llx\n", ((u64)control.Field.DramBaseAddr) << 28);

    DRAM_ADDRESS_CTL_REGISTER DRAMCTL;
    __df_indirect_read_BIOS_OS(0, DRAMADDRESSCTL_0_FUNC, DRAMADDRESSCTL_0_REG, 0x3, &DRAMCTL.Value);
    pr_info("Flags:\n");
    pr_info("  AddrRngVal    : %d\n", DRAMCTL.Field.AddrRngVal);
    pr_info("  LgcyMmioHoleEn: %d\n", DRAMCTL.Field.LgcyMmioHoleEn);
    pr_info("  SCM          : %d\n", DRAMCTL.Field.SCM);
    pr_info("  RemapEn      : %d\n", DRAMCTL.Field.RemapEn);
    pr_info("  RemapSel     : %d\n", DRAMCTL.Field.RemapSel);
    pr_info("  DstFabricID  : 0x%x\n", DRAMCTL.Field.DstFabricID);
}

static void spoof_base_and_limit(uint32_t number)
{
    if (number == 4)
    {
        // uint64_t addr = 0xd1227000;
        // uint64_t base = 0xd0000000 >> 16;
        // uint64_t limit = 0xd7fe0000 >> 16;

        // uint64_t base =  0x100000000 >> 16;
        // uint64_t limit = 0x1ffffffff >> 16;

        // __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOBASEADDRESS_0_REG + 0x10 * number, 0x20, base);
        // __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOLIMITADDRESS_0_REG + 0x10 * number, 0x20, limit);

        MMIO_ADDRESS_CONTROL_REGISTER control;
        __df_indirect_read_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOADDRESSCONTROL_0_REG + 0x10 * number, 0x20, &control.Value);

        control.Field.RE = 1;
        control.Field.WE = 1;
        control.Field.DstFabricID = 0x77;
        __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOADDRESSCONTROL_0_REG + 0x10 * number, 0x20, control.Value);
    }

    // if (number == 17 && false)
    // {
    //     uint64_t addr =  0x115e9e000;
    //     uint64_t base =  0x100000000 >> 16;
    //     uint64_t limit = 0x1ffffffff >> 16;

    //     __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOBASEADDRESS_0_REG + 0x10 * number, 0x20, base);
    //     __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOLIMITADDRESS_0_REG + 0x10 * number, 0x20, limit);

    //     MMIO_ADDRESS_CONTROL_REGISTER control;
    //     __df_indirect_read_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOADDRESSCONTROL_0_REG + 0x10 * number, 0x20, &control.Value);

    //     control.Field.RE = 1;
    //     control.Field.WE = 1;
    //     control.Field.DstFabricID = 0x03;

    //     __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOADDRESSCONTROL_0_REG + 0x10 * number, 0x20, control.Value);

    //     // //Set CCM0 to CSBLOCK
    //     // __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOBASEADDRESS_0_REG + 0x10 * number, 0x10, base);
    //     // __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOLIMITADDRESS_0_REG + 0x10 * number, 0x10, limit);

    //     // control.Value = 0;
    //     // __df_indirect_read_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOADDRESSCONTROL_0_REG + 0x10 * number, 0x10, &control.Value);

    //     // control.Field.RE = 1;
    //     // control.Field.WE = 1;
    //     // control.Field.DstFabricID = 0x03;

    //     // __df_indirect_write_BIOS_OS(0, MMIOADDRESSCONTROL_15_FUNC, MMIOADDRESSCONTROL_0_REG + 0x10 * number, 0x10, control.Value);
    // }
}

static int __init df_tmr_init(void)
{
    pr_info("Loaded DF MMIO Remap Print\n");

    for (size_t i = 0; i < 20; i++)
    {
        print_base_and_limit(i);
        spoof_base_and_limit(i);
        print_base_and_limit(i);
    }

    return -1;
}

static void __exit df_tmr_exit(void)
{
    printk(KERN_INFO "dummy: Exiting the module\n");
    return;
}

module_init(df_tmr_init);
module_exit(df_tmr_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to write a memory pointer to RAX and execute VMSAVE to a previously allocated page");
MODULE_VERSION("1.0");