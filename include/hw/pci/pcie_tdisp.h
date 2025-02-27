#ifndef QEMU_PCIE_TDISP_H
#define QEMU_PCIE_TDISP_H

#include "hw/pci/pci.h"

/* PCI Express Device Capabilites Registers */
#define  PCI_EXP_DEVCAP_TEE_IO  0x40000000 /* TEE-IO Supported */

bool pcie_tee_io_supported(PCIDevice *dev);

/*
 * Must be initialized after the IDE Extended Capabiltiy has been initialized.
 */
void pcie_tdisp_init(PCIDevice *dev, uint16_t offset);

#endif /* QEMMU_PCIE_TDISP_H */
