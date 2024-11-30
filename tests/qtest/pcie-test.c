#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "libqos/qgraph.h"
#include "libqos/pci.h"

typedef struct QPCIeTestDev {
    QOSGraphObject obj;
    QPCIDevice dev;
} QPCIeTestDev;

static void *pcie_testdev_get_driver(void *obj, const char *interface)
{
    QPCIeTestDev *pcie_testdev = obj;

    if (!g_strcmp0(interface, "pci-device")) {
        return &pcie_testdev->dev;
    }

    fprintf(stderr, "%s not present in edu\n", interface);
    g_assert_not_reached();
}

static void *pcie_testdev_create(void *pci_bus, QGuestAllocator *alloc,
                                 void *addr)
{
    QPCIeTestDev *pcie_testdev = g_new0(QPCIeTestDev, 1);
    QPCIBus *bus = pci_bus;

    qpci_device_init(&pcie_testdev->dev, bus, addr);
    pcie_testdev->obj.get_driver = pcie_testdev_get_driver;

    return &pcie_testdev->obj;
}

/* Tests only initialization so far. TODO: Replace with functional tests */
static void nop(void *obj, void *data, QGuestAllocator *alloc)
{
    QPCIeTestDev *pcie_testdev = obj;
    QPCIDevice *pdev = &pcie_testdev->dev;
    QPCIBar bar;

    g_assert_cmpuint(qpci_config_readw(pdev, PCI_VENDOR_ID), ==,
                     PCI_VENDOR_ID_QEMU);
    g_assert_cmpuint(qpci_config_readw(pdev, PCI_DEVICE_ID), ==,
                     0x11e8);

    qpci_device_enable(pdev);
    bar = qpci_iomap(pdev, 0, NULL);

    g_assert_cmpuint(qpci_io_readl(pdev, bar, 0), ==, 0x010000edu);

    qpci_iounmap(pdev, bar);
}

static void pcie_testdev_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {
        .extra_device_opts = "addr=04.0"
    };

    add_qpci_address(&opts, &(QPCIAddress) { .devfn = QPCI_DEVFN(4, 0) });

    qos_node_create_driver("edu", pcie_testdev_create);
    qos_node_consumes("edu", "pci-bus", &opts);

    qos_add_test("nop", "edu", nop, NULL);
}

libqos_init(pcie_testdev_register_nodes);
