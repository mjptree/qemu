#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/units.h"
#include "hw/pci/pci_device.h"
#include "hw/pci/pcie_doe.h"
#include "hw/qdev-properties.h"
#include "hw/spdm/spdm-responder.h"
#include "qom/object.h"

#define TYPE_TDISP_TEST_DEV "tdisp-testdev"
OBJECT_DECLARE_SIMPLE_TYPE(TDISPTestDevState, TDISP_TEST_DEV)

struct TDISPTestDevState {
    /*< private >*/
    PCIDevice parent_obj;
    /*< public >*/

    SPDMResponder *spdm_responder;
    MemoryRegion mmio;
};

static MemTxResult tdisp_testdev_mmio_read(
    void *opaque, hwaddr addr, uint64_t *data, unsigned int size,
    MemTxAttrs attrs)
{
    *data = 0;
    return MEMTX_OK;
}

static MemTxResult tdisp_testdev_mmio_write(
    void *opaque, hwaddr addr, uint64_t data, unsigned size, MemTxAttrs attrs)
{
    return MEMTX_OK;
}

static uint32_t tdisp_testdev_config_read(
    PCIDevice *pdev, uint32_t addr, int len)
{
    uint32_t data = 0;
    if (pcie_doe_read_config(&pdev->doe_spdm, addr, len, &data)) {
        return data;
    }

    return pci_default_read_config(pdev, addr, len);
}

static void tdisp_testdev_config_write(
    PCIDevice *pdev, uint32_t addr, uint32_t data, int len)
{
    pcie_doe_write_config(&pdev->doe_spdm, addr, data, len);
    pci_default_write_config(pdev, addr, data, len);
}

static const MemoryRegionOps mmio_ops = {
    .read_with_attrs = tdisp_testdev_mmio_read,
    .write_with_attrs = tdisp_testdev_mmio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
        .unaligned = false,
    },
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
        .unaligned = false,
    }
};

static bool tdisp_testdev_send_message(
    DeviceState *dev, size_t message_size, const void *message)
{
    PCIDevice *pdev = PCI_DEVICE(dev);
    return pcie_doe_send_message(&pdev->doe_spdm, message_size, message);
}

static bool tdsip_testdev_receive_message(
    DeviceState *dev, size_t *message_size, void **message)
{
    PCIDevice *pdev = PCI_DEVICE(dev);
    return pcie_doe_receive_message(&pdev->doe_spdm, message_size, message);
}

static bool tdisp_testdev_get_response(
    DeviceState *dev, const uint32_t *session_id, size_t request_size,
    const SPDMHeader *request, size_t *response_size, SPDMHeader *response)
{
    PCIDevice *pdev = PCI_DEVICE(dev);
    TDISPTestDevState *tdisp = TDISP_TEST_DEV(dev);
    SPDMPCIDefined *request_header, *response_header;
    PCIPayload *request_payload, *response_payload;
    size_t request_payload_size, response_payload_size;
    SPDMErrorCode error_code = 0;
    bool success;

    assert(session_id && response && response_size);
    assert(*response_size >= sizeof(SPDMPCIDefined) + sizeof(PCIPayload));

    if (request_size < sizeof(SPDMPCIDefined) + sizeof(PCIPayload)) {
        return spdm_responder_get_response_error(
            tdisp->spdm_responder, SPDM_ERROR_CODE_INVALID_REQUEST, 0,
            response_size, response);
    }

    request_header = (SPDMPCIDefined *)request;
    response_header = (SPDMPCIDefined *)response;

    if (request_header->vendor_defined.header.request_response_code !=
        SPDM_REQUEST_CODE_VENDOR_DEFINED_REQUEST)
    {
        return spdm_responder_get_response_error(
            tdisp->spdm_responder, SPDM_ERROR_CODE_UNSUPPORTED_REQUEST,
            request_header->vendor_defined.header.request_response_code,
            response_size, response);
    }

    if (request_header->vendor_defined.standard_id !=
        SPDM_STANDARD_ID_PCISIG ||
        request_header->vendor_defined.len !=
            sizeof(request_header->vendor_id) ||
        request_header->vendor_id != SPDM_VENDOR_ID_PCISIG) {
        return spdm_responder_get_response_error(
            tdisp->spdm_responder, SPDM_ERROR_CODE_INVALID_REQUEST, 0,
            response_size, response);
    }

    if (request_size < sizeof(SPDMPCIDefined) + request_header->req_length) {
        return spdm_responder_get_response_error(
            tdisp->spdm_responder, SPDM_ERROR_CODE_INVALID_REQUEST, 0,
            response_size, response);
    }

    request_payload = (PCIPayload *)
        ((uint8_t *)request + sizeof(SPDMPCIDefined));
    request_payload_size = request_header->req_length;
    response_payload = (PCIPayload *)
        ((uint8_t *)response + sizeof(SPDMPCIDefined));
    response_payload_size = *response_size - sizeof(SPDMPCIDefined);

    switch (request_payload->protocol_id) {
    case PCI_SPDM_PROTOCOL_ID_IDE_KM:
        success = pcie_ide_km_get_response(
            pdev, *session_id, request_payload, request_payload_size,
            response_payload, &response_payload_size, &error_code);
        break;
    case PCI_SPDM_PROTOCOL_ID_TDISP:
        return spdm_responder_get_response_error(
            tdisp->spdm_responder, error_code, SPDM_ERROR_CODE_INVALID_REQUEST,
            response_size, response);
    default:
        return spdm_responder_get_response_error(
            tdisp->spdm_responder, error_code, SPDM_ERROR_CODE_INVALID_REQUEST,
            response_size, response);
    };

    if (!success) {
        return false;
    }

    /* Error code 0 is currently reserved and not a valid error code. */
    if (error_code) {
        return spdm_responder_get_response_error(
            tdisp->spdm_responder, error_code, 0, response_size, response);
    }

    if (UINT16_MAX < response_payload_size) {
        error_report(
            "SPDM vendor defined response payload size exceeds maximum "
            "representable response length (size=%lu)", response_payload_size);
        return spdm_responder_get_response_error(
            tdisp->spdm_responder, error_code,
            SPDM_ERROR_CODE_OPERATION_FAILED, response_size, response);
    }

    response_header->vendor_defined.header.spdm_version =
        spdm_responder_get_connection_version(tdisp->spdm_responder);
    response_header->vendor_defined.header.request_response_code =
        SPDM_RESPONSE_CODE_VENDOR_DEFINED_RESPONSE;
    response_header->vendor_defined.standard_id = SPDM_STANDARD_ID_PCISIG;
    response_header->vendor_defined.len = sizeof(response_header->vendor_id);
    response_header->vendor_id = SPDM_VENDOR_ID_PCISIG;
    response_header->req_length = response_payload_size;
    return true;
}

static bool tdisp_testdev_handle_request(DOECap *cap)
{
    TDISPTestDevState *d = TDISP_TEST_DEV(cap->pdev);
    Error *local_error;

    if (!spdm_responder_dispatch_message(d->spdm_responder, &local_error)) {
        /*
         * In case PCIe DOE is used for SPDM message transport, returning false
         * triggers a DOE Error. In this case, we freeze
         */
        error_report_err(local_error);
        return false;
    }

    return true;
}

static void tdisp_testdev_exit(PCIDevice *pdev)
{
    pcie_doe_fini(&pdev->doe_spdm);
    pcie_ide_fini(pdev);
}

static DOEProtocol doe_protocols[] = {
    { PCI_VENDOR_ID_PCI_SIG, PCI_SIG_DOE_CMA, tdisp_testdev_handle_request },
    { PCI_VENDOR_ID_PCI_SIG, PCI_SIG_DOE_SECURED_CMA,
        tdisp_testdev_handle_request },
    { },
};

static SelectiveIDEStream sel_ide_streams[] = {
    { .ide_addr_assoc_blks_num = 0 },
};

static void tdisp_testdev_realize(PCIDevice *pdev, Error **errp)
{
    ERRP_GUARD();
    TDISPTestDevState *d = TDISP_TEST_DEV(pdev);

    if (!d->spdm_responder) {
        error_setg(errp, "tdisp-testdev requires a valid spdm-responder");
        error_append_hint(errp, "create an spdm-responder with `-object "
                          "spdm-responder-libspdm,...");
        return;
    }

    pcie_endpoint_cap_init(pdev, 0x80);
    memory_region_init_io(&d->mmio, OBJECT(d), &mmio_ops, d,
        "tdisp-testdev-mmio", 4 * KiB);
    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mmio);
    pcie_doe_init(
        pdev, &pdev->doe_spdm, PCI_CONFIG_SPACE_SIZE, doe_protocols, true, 0);
    pcie_ide_init(
        pdev, PCI_CONFIG_SPACE_SIZE + PCI_DOE_SIZEOF, true, NULL, 0,
        sel_ide_streams, ARRAY_SIZE(sel_ide_streams));

    if (!device_spdm_responder_init(DEVICE(d), d->spdm_responder,
            tdisp_testdev_send_message, tdsip_testdev_receive_message,
            tdisp_testdev_get_response, errp)) {
        tdisp_testdev_exit(pdev);
    }
}

static const Property tdisp_testdev_properties[] = {
    DEFINE_PROP_LINK(
        "spdm-responder", TDISPTestDevState, spdm_responder,
        TYPE_SPDM_RESPONDER, SPDMResponder *),
};

static void tdisp_testdev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->config_write = tdisp_testdev_config_write;
    k->config_read = tdisp_testdev_config_read;
    k->realize = tdisp_testdev_realize;
    k->exit = tdisp_testdev_exit;
    k->class_id = PCI_CLASS_OTHERS;
    k->vendor_id = PCI_VENDOR_ID_QEMU;
    k->device_id = 0x11e9;
    dc->desc = "TDISP Test Device";
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    device_class_set_props(dc, tdisp_testdev_properties);
}

static InterfaceInfo tdisp_testdev_interfaces[] = {
    { INTERFACE_PCIE_DEVICE },
    { },
};

static const TypeInfo tdisp_testdev_info = {
    .name = TYPE_TDISP_TEST_DEV,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(TDISPTestDevState),
    .class_init = tdisp_testdev_class_init,
    .interfaces = tdisp_testdev_interfaces,
};

static void tdisp_testdev_register_types(void)
{
    type_register_static(&tdisp_testdev_info);
}

type_init(tdisp_testdev_register_types)
