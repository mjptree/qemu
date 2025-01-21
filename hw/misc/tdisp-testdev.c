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
    const void *request, size_t *response_size, void *response)
{
    return false;
}

static bool tdisp_testdev_handle_request(DOECap *cap)
{
    TDISPTestDevState *d = TDISP_TEST_DEV(cap->pdev);
    Error *local_error;

    if (!spdm_responder_dispatch_message(d->spdm_responder, &local_error)) {
        error_report_err(local_error);
        return false;
    }

    return true;
}

static DOEProtocol doe_protocols[] = {
    { PCI_VENDOR_ID_PCI_SIG, PCI_SIG_DOE_CMA, tdisp_testdev_handle_request },
    { PCI_VENDOR_ID_PCI_SIG, PCI_SIG_DOE_SECURED_CMA,
        tdisp_testdev_handle_request },
    { },
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

    memory_region_init_io(&d->mmio, OBJECT(d), &mmio_ops, d,
        "tdisp-testdev-mmio", 4 * KiB);
    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mmio);
    pcie_doe_init(pdev, &pdev->doe_spdm, PCI_CONFIG_SPACE_SIZE, doe_protocols,
        true, 0);

    if (!device_spdm_responder_init(DEVICE(d), d->spdm_responder,
            tdisp_testdev_send_message, tdsip_testdev_receive_message,
            tdisp_testdev_get_response, errp)) {
        return;
    }
}

static void tdisp_testdev_exit(PCIDevice *pdev)
{
    pcie_doe_fini(&pdev->doe_spdm);
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
