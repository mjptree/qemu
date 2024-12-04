#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "libqos/qgraph.h"
#include "libqos/pci.h"

typedef struct QTDISPTestDev {
    QOSGraphObject obj;
    QPCIDevice dev;
} QTDISPTestDev;

static void *tdisp_testdev_get_driver(void *obj, const char *interface)
{
    QTDISPTestDev *tdisp_testdev = obj;

    if (!g_strcmp0(interface, "pci-device")) {
        return &tdisp_testdev->dev;
    }

    fprintf(stderr, "%s not present in tdisp-testdev\n", interface);
    g_assert_not_reached();
}

static void *tdisp_testdev_create(void *pci_bus, QGuestAllocator *alloc,
                                 void *addr)
{
    QTDISPTestDev *tdisp_testdev = g_new0(QTDISPTestDev, 1);
    QPCIBus *bus = pci_bus;

    qpci_device_init(&tdisp_testdev->dev, bus, addr);
    tdisp_testdev->obj.get_driver = tdisp_testdev_get_driver;

    return &tdisp_testdev->obj;
}

/* Tests only initialization so far. TODO: Replace with functional tests */
static void nop(void *obj, void *data, QGuestAllocator *alloc)
{
    QTDISPTestDev *tdisp_testdev = obj;
    QPCIDevice *pdev = &tdisp_testdev->dev;
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

static void tdisp_testdev_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {
        .extra_device_opts = "addr=04.0"
    };

    add_qpci_address(&opts, &(QPCIAddress) { .devfn = QPCI_DEVFN(4, 0) });

    qos_node_create_driver("tdisp-testdev", tdisp_testdev_create);
    qos_node_consumes("tdisp-testdev", "pci-bus", &opts);

    qos_add_test("nop", "tdisp-testdev", nop, NULL);
}

libqos_init(tdisp_testdev_register_nodes);
