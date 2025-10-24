#include <asm/amd/nb.h>
#include <asm/amd/node.h>
#include <linux/bitfield.h>
#include <linux/kernel.h>

static void __maybe_unused read_smn_check_error(int addr, int *val) {
    int err = amd_smn_read(0, addr, val);
    if (err) {
        //pr_err("error reading smn\n");
        return;
    }
    // pr_info("[+]read SMN ADDR: 0x%x Value 0x%x",addr, *val);
}

static void __maybe_unused write_smn_check_error(int addr, int val) {
    int err = amd_smn_write(0, addr, val);
    if (err) {
        pr_err("error writing smn\n");
        return;
    }
    // pr_info("[+]write SMN ADDR: 0x%x Value 0x%x",addr, val);
}


static void __maybe_unused output_timeout_registers(void){

    u32 smn_base = 0x1013A000;
    u32 val;

     /*
     * change timeout value for slaveERR
    */
    pr_info("SMN ADDR: 0x%08x ", 0x1013A000 + 0x260);
    for (u32 ioms_bus = 0; ioms_bus < 4; ioms_bus++){

        read_smn_check_error((ioms_bus << 20) + smn_base + 0x260, &val);
        write_smn_check_error((ioms_bus << 20) + smn_base + 0x260, 0);
        read_smn_check_error((ioms_bus << 20) + smn_base + 0x260, &val);
        pr_cont("| 0x%08x", val);
    }
    pr_cont("|\n");

    
    for (u32 offset = 0x264; offset < 0x279; offset += 4) {
        pr_info("SMN ADDR: 0x%08x ", smn_base + offset);
        for (u32 ioms_bus = 0; ioms_bus < 4; ioms_bus++){
            read_smn_check_error((ioms_bus << 20) + smn_base + offset, &val);
            write_smn_check_error((ioms_bus << 20) + smn_base + offset, 0);
            read_smn_check_error((ioms_bus << 20) + smn_base + offset, &val);
            pr_cont("| 0x%08x", val);
        }
        pr_cont("|\n");
    }
}



static int __init slave_err_init(void) {
    pr_info("Loaded Slave Err loaded\n");
    
    output_timeout_registers();

    return -1;
}

static void __exit slave_err_exit(void) {
    printk(KERN_INFO "dummy: Exiting the module\n");
    return;
}

module_init(slave_err_init);
module_exit(slave_err_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("test");