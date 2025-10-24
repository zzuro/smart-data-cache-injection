#include "smn.h"


#define SMN_RW(LEVEL) \
int amd_smn_rw_##LEVEL(u16 node, u32 address, u32 *value, bool write){ \
	struct pci_dev *root;\
	int err = -ENODEV;\
	root = node_to_amd_nb(node)->link;\
	if (!root)\
		return err;\
	err = pci_write_config_dword(root, SMN_REQUEST_REG_##LEVEL, address);\
	if (err) {\
		pr_warn("Error programming SMN address 0x%x.\n", address);\
		return err;\
	}\
	err = (write ? pci_write_config_dword(root, SMN_RESPONSE_REG_##LEVEL, *value)\
		     : pci_read_config_dword(root, SMN_RESPONSE_REG_##LEVEL, value));\
	return err;\
}
SMN_RW(OPENSIL)
SMN_RW(OS)