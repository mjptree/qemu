#include "qemu/osdep.h"
#include "hw/pci/pci_device.h"
#include "hw/pci/pcie_tdisp.h"

bool pcie_tee_io_supported(PCIDevice *dev)
{
    return pci_get_long(dev->config + dev->exp.exp_cap + PCI_EXP_DEVCAP) &
        PCI_EXP_DEVCAP_TEE_IO;
}

void pcie_tdisp_init(PCIDevice *dev, uint16_t offset)
{
    assert(pcie_ide_present(dev));

    pci_long_test_and_set_mask(
        dev->config + dev->exp.exp_cap + PCI_EXP_DEVCAP,
        PCI_EXP_DEVCAP_TEE_IO);
}
