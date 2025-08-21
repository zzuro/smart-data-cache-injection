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

// Fallback definitions if not provided by the current kernel version
#ifndef IOMMU_READ
#define IOMMU_READ  0x1
#endif
#ifndef IOMMU_WRITE
#define IOMMU_WRITE 0x2
#endif

// Pointer to the PCI device that we want to manage
static struct pci_dev *dev;

// User-specified DMA address parameter
// This is the address we want to map into the device's IOMMU domain.
//
// It can be specified when loading the module, for example:
// sudo insmod dma_address_mapper.ko custom_dma_addr=0x12345000
static dma_addr_t custom_dma_addr = 0;
module_param(custom_dma_addr, ullong, 0644);
MODULE_PARM_DESC(custom_dma_addr, "User-specified DMA address");

// Function: map_custom_dma_address()
// ---------------------------------
// Maps the user-specified DMA address (custom_dma_addr) into the IOMMU domain
// associated with the identified PCI device.
//

u64 begin_dma[] = {0xFD00000000, 0xFE20000000, 0xfdf9100000, 0xFFFFdf9108000, 0x00084eb00000, 0x93c00000, 0x75d20000, 0ull}; // Example physical address
u64 end_dma[] = {0xFFFFFFFFFF+1, 0xFFFFFFFFFF+1, 0xfdf9110000, 0xFFFFdf9109000, 0x00084ec00000, 0x9c1fffffull + 1, 0x77ffffff + 1, 0ull}; // Example physical address

u64 begin_phys_addr[] = {0xFD00000000, 0xFE20000000, 0xfdf9100000, 0xFFFFdf9108000, 0x00084eb00000, 0x93c00000, 0x75d20000, 0ull};


// Return Values:
//   0 on success
//  <0 on failure (e.g., if the IOMMU domain cannot be found or mapping fails)
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

    // The user-specified address is assumed to be a valid physical address.

    int i = 0;
    while(begin_dma[i] != 0) {
        u64 size = end_dma[i] - begin_dma[i];
        
        ret = iommu_map(domain, (unsigned long)begin_dma[i], begin_phys_addr[i], size,
            IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
        if (ret) {
            pr_err("Failed to map custom DMA address: %d\n", ret);
            return ret;
        }
        // Mapping succeeded
        pr_info("Custom DMA address 0x%llx mapped successfully to 0x%llx\n", begin_dma[i], begin_phys_addr[i]);
        i++;
    }

    // Map the given physical address at the same IOVA (custom_dma_addr) into the IOMMU domain.
    // We grant both read and write permissions. GFP_KERNEL is the allocation flag for internal 
    // resource allocation.

    return -1;
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
    // 41:00.0 Ethernet controller [0200]: Xilinx Corporation Device [10ee:0666] (rev 02)
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

    // Check if a custom DMA address has been provided
    if (custom_dma_addr == 0) {
        pr_info("Mapping user-specified DMA address\n");
        return map_custom_dma_address();
    } else {
        pr_err("Custom DMA address not specified\n");
        return -EINVAL;
    }
}

// Function: dma_mapper_exit()
// ---------------------------
// The module exit function. Called when the module is unloaded.
// It does not unmap the DMA address in this example, but logs that the module
// is unloaded. In a production environment, you may want to unmap or clean up.
static void __exit dma_mapper_exit(void)
{
    pr_info("DMA Address Mapper module unloaded\n");
}

// Register the initialization and exit functions
module_init(dma_mapper_init);
module_exit(dma_mapper_exit);

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leon");
MODULE_DESCRIPTION("Custom DMA Address Mapper Module");
