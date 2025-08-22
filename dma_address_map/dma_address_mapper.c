#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>              
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

#include "defs.h"

#define DEVICE_NAME "mapper_dev"
#define WR_VALUE _IOW('a', 'a', __u64 *)
#define RD_VALUE _IOR('a', 'b', __u64 *)
#define MAP_ADDR _IOW('a', 'c', __u64 *)

static dev_t ioclt_dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

static struct pci_dev *dev; // Pointer to the PCI device that we want to manage

static __u64 amount = 1;                     // Number of mappings to handle
static mapping_t *custom_dma_mapping = NULL; // Pointer to the custom DMA mapping structure

static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case WR_VALUE:
            break;
        case RD_VALUE:
            break;
        case MAP_ADDR:
            __u64 addr;
            if (copy_from_user(&addr, (void __user *)arg, sizeof(addr)))
            {
                pr_err("Failed to copy data from user space\n");
                return -EFAULT;
            }
            pr_info("Mapping address: 0x%lx\n", addr);
            break;
        default:
            pr_info("CMD not found!\n");
            break;
    }

    return 0;
}

static int ext_map(struct file *file, struct vm_area_struct *vma)
{
    if(!custom_dma_mapping)
    {
        pr_err("No buffer allocated for mapping\n");
        return -EINVAL;
    }

    unsigned long phys_addr = virt_to_phys(custom_dma_mapping[0].buffer.vaddr);
    unsigned long vsize = vma->vm_end - vma->vm_start;
    if (vsize > custom_dma_mapping[0].buffer.size)
    {
        pr_err("Requested size exceeds buffer size\n");
        return -EINVAL;
    }

    if (remap_pfn_range(vma, vma->vm_start, phys_addr >> PAGE_SHIFT, vsize, vma->vm_page_prot))
    {
        pr_err("Failed to remap memory\n");
        return -EAGAIN;
    }

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = etx_ioctl,
    .mmap = ext_map,

};

static int init_custom_dma_address(void)
{
    custom_dma_mapping = kzalloc(sizeof(mapping_t) * amount, GFP_KERNEL);
    if (!custom_dma_mapping)
    {
        pr_err("Failed to allocate memory for custom DMA mapping\n");
        return -ENOMEM;
    }

    // address range FD_0000_0000h - FF_FFFF_FFFFh, inclusive, have special meanings, dont use them: https://kib.kiev.ua/x86docs/AMD/IOMMU/48882-2.00.pdf p. 31
    custom_dma_mapping[0].iova = 0xFD990000;
    custom_dma_mapping[0].flags = IOMMU_READ | IOMMU_WRITE;
    custom_dma_mapping[0].buffer.size = KB(4);
    custom_dma_mapping[0].buffer.vaddr = kmalloc(custom_dma_mapping[0].buffer.size, GFP_KERNEL); // no cache-coherent allocation
    if (custom_dma_mapping[0].buffer.vaddr)
    {
        memset((void*)custom_dma_mapping[0].buffer.vaddr, 0xAB, custom_dma_mapping[0].buffer.size); // Fill with test byte 0xAB
    }
    custom_dma_mapping[0].buffer.paddr = virt_to_phys(custom_dma_mapping[0].buffer.vaddr);

    return 0;
}

static int map_custom_dma_address(void)
{
    struct iommu_domain *domain;
    int ret;

    domain = iommu_get_domain_for_dev(&dev->dev);
    if (!domain)
    {
        pr_err("Failed to get IOMMU domain\n");
        return -ENODEV;
    }

    for (int i = 0; i < amount; i++)
    {

        ret = iommu_map(domain, (unsigned long)custom_dma_mapping[i].iova,
                        custom_dma_mapping[i].buffer.paddr, custom_dma_mapping[i].buffer.size,
                        custom_dma_mapping[i].flags, GFP_KERNEL);
        if (ret)
        {
            pr_err("Failed to map custom DMA address: %d\n", ret);
            return ret;
        }

        pr_info("Custom DMA address 0x%llx mapped successfully to 0x%llx\n",
                custom_dma_mapping[i].iova, custom_dma_mapping[i].buffer.paddr);
    }

    return 0;
}

static int __init dma_mapper_init(void)
{

    if (alloc_chrdev_region(&ioclt_dev, 0, 1, DEVICE_NAME) < 0)
    {
        pr_err("Failed to allocate a major number\n");
        return -ENOMEM;
    }

    cdev_init(&etx_cdev, &fops);

    if (cdev_add(&etx_cdev, ioclt_dev, 1) < 0)
    {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(ioclt_dev, 1);
        return -ENOMEM;
    }

    if (IS_ERR(dev_class = class_create(DEVICE_NAME)))
    {
        pr_err("Cannot create the struct class\n");
        unregister_chrdev_region(ioclt_dev, 1);
        return -ENOMEM;
    }

    if (IS_ERR(device_create(dev_class, NULL, ioclt_dev, NULL, DEVICE_NAME)))
    {
        pr_err("Cannot create the Device 1\n");
        class_destroy(dev_class);
        unregister_chrdev_region(ioclt_dev, 1);
        return -ENOMEM;
    }

    // 81:00.0 Ethernet controller [0200]: Xilinx Corporation Device [10ee:0666] (rev 02)
    dev = pci_get_device(0x10EE, 0x0666, NULL);
    if (!dev)
    {
        pr_err("Device not found\n");
        return -ENODEV;
    }

    if (pci_enable_device(dev) < 0)
    {
        pr_err("Failed to enable PCI device\n");
        return -ENODEV;
    }

    if (init_custom_dma_address() < 0)
    {
        pr_err("Failed to initialize custom DMA address\n");
        return -ENOMEM;
    }

    return map_custom_dma_address();
}

static void __exit dma_mapper_exit(void)
{

    device_destroy(dev_class, ioclt_dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(ioclt_dev, 1);

    struct iommu_domain *domain;
    domain = iommu_get_domain_for_dev(&dev->dev);
    if (!domain)
    {
        pr_err("Failed to get IOMMU domain\n");
        return;
    }

    for (int i = 0; i < amount; i++)
    {
        if (custom_dma_mapping[i].buffer.vaddr)
        {

            kfree((const void *)custom_dma_mapping[i].buffer.vaddr);
            custom_dma_mapping[i].buffer.vaddr = 0;
        }

        iommu_unmap(domain, (unsigned long)custom_dma_mapping[i].iova, custom_dma_mapping[i].buffer.size);
    }

    if (custom_dma_mapping)
    {
        kfree((const void *)custom_dma_mapping);
        custom_dma_mapping = NULL;
    }

    pr_info("DMA Address Mapper module unloaded\n");
}

module_init(dma_mapper_init);
module_exit(dma_mapper_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alessandro Vezzù");
MODULE_DESCRIPTION("Custom DMA Address Mapper Module");
