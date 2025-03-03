#ifndef QEMU_PCIE_TDISP_H
#define QEMU_PCIE_TDISP_H

#include "hw/pci/pci.h"

/* PCI Express Device Capabilites Registers */
#define  PCI_EXP_DEVCAP_TEE_IO  0x40000000 /* TEE-IO Supported */

typedef enum TDISPConnectionState {
    TDISP_CONNECTION_STATE_NOT_STARTED        = 0x00,
    TDISP_CONNECTION_STATE_AFTER_VERSION      = 0x01,
    TDISP_CONNECTION_STATE_AFTER_CAPABILITIES = 0x02,
} TDISPConnectionState;

typedef struct TDISPDevice {
    TDISPConnectionState connection_state;
    uint64_t mmio_reporting_offset;
    uint32_t tsm_caps;
    uint16_t flags;
    uint8_t tdisp_version;
    uint8_t tdi_state;
    uint8_t start_interface_nonce[32];
} TDISPDevice;

bool pcie_tee_io_supported(PCIDevice *dev);

/*
 * Must be initialized after the IDE Extended Capabiltiy has been initialized.
 */
void pcie_tdisp_init(PCIDevice *dev, bool tee_limited_stream_supported);

bool pcie_tdisp_get_response(
    PCIDevice *dev, uint32_t session_id, const PCIPayload *request,
    size_t request_size, PCIPayload *response, size_t *response_size,
    SPDMErrorCode *error_code);

#endif /* QEMMU_PCIE_TDISP_H */
