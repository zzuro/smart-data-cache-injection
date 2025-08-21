/*
 * dma_address_mapper.c - A Kernel Module to Map a User-Specified DMA Address
 *
 * This module demonstrates how to map a user-specified DMA address into the
 * IOMMU domain of a particular PCI device without modifying existing kernel functions.
 *
 * Usage Instructions:
 * -------------------
 * 1. Build the module:
 *    make
 *
 * 2. Load the module with a custom DMA address parameter:
 *    sudo insmod dma_address_mapper.ko custom_dma_addr=0x<valid_phys_address>
 *
 *    Replace <valid_phys_address> with the physical address you want to map.
 *    For example:
 *    sudo insmod dma_address_mapper.ko custom_dma_addr=0x12345000
 *
 * 3. Check the kernel logs to verify that the mapping was successful:
 *    dmesg | tail
 *
 *    You should see a message indicating that the DMA address was mapped successfully.
 *
 * 4. Unload the module when done:
 *    sudo rmmod dma_address_mapper
 *
 * Notes:
 * ------
 * - The module searches for a PCI device with Vendor ID 0x10EE and Device ID 0x0666.
 *   Adjust these if your device differs.
 *
 * - Ensure the specified DMA address is meaningful for your system and hardware. 
 *   The module does not validate whether the address is suitable for the device or IOMMU.
 *
 * - The physical address specified as the DMA address **must be page-aligned**. 
 *   For example, on a system with 4KB pages, the address must be a multiple of 0x1000.
 *   Non-aligned addresses will result in an invalid mapping.
 *
 * - This code example requires your kernel to be built with IOMMU support and the 
 *   necessary IOMMU APIs enabled.
 *
 * - Use at your own risk. Mapping arbitrary addresses can cause system instability 
 *   or data corruption if used incorrectly.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>

#include "defs.h"


static struct pci_dev *dev;                         // Pointer to the PCI device that we want to manage

static __u64 amount = 1;                            // Number of mappings to handle
static mapping_t* custom_dma_mapping = NULL;        // Pointer to the custom DMA mapping structure


static int init_custom_dma_address(void)
{
    custom_dma_mapping = kzalloc(sizeof(mapping_t) * amount, GFP_KERNEL);
    if (!custom_dma_mapping) {
        pr_err("Failed to allocate memory for custom DMA mapping\n");
        return -ENOMEM;
    }


    //address range FD_0000_0000h - FF_FFFF_FFFFh, inclusive, have special meanings, dont use them: https://kib.kiev.ua/x86docs/AMD/IOMMU/48882-2.00.pdf p. 31
    custom_dma_mapping[0].iova = 0xFD990000;
    custom_dma_mapping[0].flags = IOMMU_READ | IOMMU_WRITE; 
    custom_dma_mapping[0].buffer.size = KB(4); 
    custom_dma_mapping[0].buffer.vaddr = kmalloc(custom_dma_mapping[0].buffer.size, GFP_KERNEL);
    if (custom_dma_mapping[0].buffer.vaddr) {
        memset(custom_dma_mapping[0].buffer.vaddr, 0xAB, custom_dma_mapping[0].buffer.size); // Fill with test byte 0xAB
    }
    custom_dma_mapping[0].buffer.paddr = virt_to_phys(custom_dma_mapping[0].buffer.vaddr);

    return 0;
}


// Function: map_custom_dma_address()
// ---------------------------------
// Maps the user-specified DMA address (custom_dma_addr) into the IOMMU domain
// associated with the identified PCI device.
//
static int map_custom_dma_address(void)
{
    struct iommu_domain *domain;
    int ret;

    // Retrieve the IOMMU domain for the device
    domain = iommu_get_domain_for_dev(&dev->dev);
    if (!domain) {
        pr_err("Failed to get IOMMU domain\n");
        return -ENODEV;
    }


    for(int i = 0; i < amount; i++) {

        // Map the given physical address at the same IOVA (custom_dma_addr) into the IOMMU domain.
        // We grant both read and write permissions. GFP_KERNEL is the allocation flag for internal 
        // resource allocation.
        ret = iommu_map(domain, (unsigned long)custom_dma_mapping[i].iova, 
                        custom_dma_mapping[i].buffer.paddr, custom_dma_mapping[i].buffer.size,
                        custom_dma_mapping[i].flags, GFP_KERNEL);
        if (ret) {
            pr_err("Failed to map custom DMA address: %d\n", ret);
            return ret;
        }

        pr_info("Custom DMA address 0x%llx mapped successfully to 0x%llx\n",
                custom_dma_mapping[i].iova, custom_dma_mapping[i].buffer.paddr);
    }

    return 0;
}

// Function: dma_mapper_init()
// ---------------------------
// The module initialization function. Called when the module is loaded.
// It tries to find the specified PCI device, enable it, and if a custom DMA address
// has been provided, it attempts to map it via map_custom_dma_address().
static int __init dma_mapper_init(void)
{
    // Find the specific PCI device by Vendor ID and Device ID
    // Change these values if your device has different IDs

    // Find the specific PCI device using vendor ID (0x10EE) and device ID (0x0666), 
    // as shown by 'lspci -nn' output:
    // 81:00.0 Ethernet controller [0200]: Xilinx Corporation Device [10ee:0666] (rev 02)
    dev = pci_get_device(0x10EE, 0x0666, NULL);
    if (!dev) {
        pr_err("Device not found\n");
        return -ENODEV;
    }

    // Enable the PCI device so we can interact with it
    if (pci_enable_device(dev) < 0) {
        pr_err("Failed to enable PCI device\n");
        return -ENODEV;
    }

    if(init_custom_dma_address() < 0) {
        pr_err("Failed to initialize custom DMA address\n");
        return -ENOMEM;
    }

    //map custom address from array
    return map_custom_dma_address();
}

// Function: dma_mapper_exit()
// ---------------------------
// The module exit function. Called when the module is unloaded.
// It does not unmap the DMA address in this example, but logs that the module
// is unloaded. In a production environment, you may want to unmap or clean up.
static void __exit dma_mapper_exit(void)
{
    struct iommu_domain *domain;
    domain = iommu_get_domain_for_dev(&dev->dev);
    if (!domain) {
        pr_err("Failed to get IOMMU domain\n");
        return;
    }

    for(int i = 0; i < amount; i++) {
        if(custom_dma_mapping[i].buffer.vaddr) {

            kfree((const void*)custom_dma_mapping[i].buffer.vaddr);
            custom_dma_mapping[i].buffer.vaddr = 0;
        }

        iommu_unmap(domain, (unsigned long)custom_dma_mapping[i].iova, custom_dma_mapping[i].buffer.size);
    }

    if(custom_dma_mapping) {
        kfree((const void*)custom_dma_mapping);
        custom_dma_mapping = NULL;
    }

    pr_info("DMA Address Mapper module unloaded\n");
}

// Register the initialization and exit functions
module_init(dma_mapper_init);
module_exit(dma_mapper_exit);

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leon");
MODULE_DESCRIPTION("Custom DMA Address Mapper Module");
