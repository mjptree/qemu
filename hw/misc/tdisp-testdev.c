#include "qemu/osdep.h"
#include "hw/pci/pci_device.h"
#include "qom/object.h"

static void tdisp_testdev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->class_id = PCI_CLASS_OTHERS;
    dc->desc = "TDISP Test Device";
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static InterfaceInfo tdisp_testdev_interfaces[] = {
    { INTERFACE_PCIE_DEVICE },
    { },
};

static const TypeInfo tdisp_testdev_info = {
    .name = "tdisp-testdev",
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCIDevice),
    .class_init = tdisp_testdev_class_init,
    .interfaces = tdisp_testdev_interfaces,
};

static void tdisp_testdev_register_types(void)
{
    type_register_static(&tdisp_testdev_info);
}

type_init(tdisp_testdev_register_types)
