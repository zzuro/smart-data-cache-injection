#pragma once
#include <linux/acpi.h>

#define TPH_ST_DSM_FUNC_INDEX	0xF

const guid_t pci_acpi_dsm_guid =
	GUID_INIT(0xe5c937d0, 0x3553, 0x4d7a,
		  0x91, 0x17, 0xea, 0x4d, 0x19, 0xc3, 0x43, 0x4d);

/*
 * Miscellaneous types
 */
typedef u32 acpi_status;	/* All ACPI Exceptions */
typedef u32 acpi_name;		/* 4-byte ACPI name */
typedef char *acpi_string;	/* Null terminated ASCII string */
typedef void *acpi_handle;	/* Actually a ptr to a NS Node */

union st_info {
	struct {
		u64 vm_st_valid : 1;
		u64 vm_xst_valid : 1;
		u64 vm_ph_ignore : 1;
		u64 rsvd1 : 5;
		u64 vm_st : 8;
		u64 vm_xst : 16;
		u64 pm_st_valid : 1;
		u64 pm_xst_valid : 1;
		u64 pm_ph_ignore : 1;
		u64 rsvd2 : 5;
		u64 pm_st : 8;
		u64 pm_xst : 16;
	};
	u64 value;
};

static acpi_status tph_invoke_dsm(acpi_handle handle, u32 cpu_uid,
				  union st_info *st_out)
{
	union acpi_object arg3[3], in_obj, *out_obj;

	if (!acpi_check_dsm(handle, &pci_acpi_dsm_guid, 7,
			    BIT(TPH_ST_DSM_FUNC_INDEX)))
		return AE_ERROR;

	/* DWORD: feature ID (0 for processor cache ST query) */
	arg3[0].integer.type = ACPI_TYPE_INTEGER;
	arg3[0].integer.value = 0;

	/* DWORD: target UID */
	arg3[1].integer.type = ACPI_TYPE_INTEGER;
	arg3[1].integer.value = cpu_uid;

	/* QWORD: properties, all 0's */
	arg3[2].integer.type = ACPI_TYPE_INTEGER;
	arg3[2].integer.value = 0;

	in_obj.type = ACPI_TYPE_PACKAGE;
	in_obj.package.count = ARRAY_SIZE(arg3);
	in_obj.package.elements = arg3;

	out_obj = acpi_evaluate_dsm(handle, &pci_acpi_dsm_guid, 7,
				    TPH_ST_DSM_FUNC_INDEX, &in_obj);
	if (!out_obj)
		return AE_ERROR;

	if (out_obj->type != ACPI_TYPE_BUFFER) {
		ACPI_FREE(out_obj);
		return AE_ERROR;
	}

	st_out->value = *((u64 *)(out_obj->buffer.pointer));

	ACPI_FREE(out_obj);

	return AE_OK;
}


void read_st_info(struct pci_dev *pdev, u16 cpu_uid)
{
    struct pci_dev *rp;
	acpi_handle rp_acpi_handle;
	union st_info info;

	rp = pcie_find_root_port(pdev);
	if (!rp || !rp->bus || !rp->bus->bridge)
		return;

	rp_acpi_handle = ACPI_HANDLE(rp->bus->bridge);

	if (tph_invoke_dsm(rp_acpi_handle, cpu_uid, &info) != AE_OK) {
		return;
	}

    pr_info("vm_st_valid   = %llu\n", (unsigned long long)info.vm_st_valid);
    pr_info("vm_xst_valid  = %llu\n", (unsigned long long)info.vm_xst_valid);
    pr_info("vm_ph_ignore  = %llu\n", (unsigned long long)info.vm_ph_ignore);
    pr_info("vm_st         = %llu\n", (unsigned long long)info.vm_st);
    pr_info("vm_xst        = %llu\n", (unsigned long long)info.vm_xst);
    pr_info("pm_st_valid   = %llu\n", (unsigned long long)info.pm_st_valid);
    pr_info("pm_xst_valid  = %llu\n", (unsigned long long)info.pm_xst_valid);
    pr_info("pm_ph_ignore  = %llu\n", (unsigned long long)info.pm_ph_ignore);
    pr_info("pm_st         = %llu\n", (unsigned long long)info.pm_st);
    pr_info("pm_xst        = %llu\n", (unsigned long long)info.pm_xst);

}