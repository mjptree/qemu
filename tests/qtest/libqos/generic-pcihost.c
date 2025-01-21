/*
 * libqos PCI bindings for generic PCI
 *
 * Copyright Red Hat Inc., 2022
 *
 * Authors:
 *  Eric Auger   <eric.auger@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "../libqtest.h"
#include "generic-pcihost.h"
#include "qobject/qdict.h"
#include "hw/pci/pci_regs.h"
#include "qemu/host-utils.h"

#include "qemu/module.h"

/* QGenericPCIHost */

QOSGraphObject *generic_pcihost_get_device(void *obj, const char *device)
{
    QGenericPCIHost *host = obj;
    if (!g_strcmp0(device, "pci-bus-generic")) {
        return &host->pci.obj;
    }
    fprintf(stderr, "%s not present in generic-pcihost\n", device);
    g_assert_not_reached();
}

void qos_create_generic_pcihost(QGenericPCIHost *host,
                                QTestState *qts,
                                QGuestAllocator *alloc)
{
    host->obj.get_device = generic_pcihost_get_device;
    qpci_init_generic(&host->pci, qts, alloc, false);
}

static uint8_t qpci_generic_pio_readb(QPCIBus *bus, uint32_t addr)
{
    QGenericPCIBus *s = container_of(bus, QGenericPCIBus, bus.legacy);

    return qtest_readb(bus->qts, s->gpex_pio_base + addr);
}

static void qpci_generic_pio_writeb(QPCIBus *bus, uint32_t addr, uint8_t val)
{
    QGenericPCIBus *s = container_of(bus, QGenericPCIBus, bus.legacy);

    qtest_writeb(bus->qts, s->gpex_pio_base + addr,  val);
}

static uint16_t qpci_generic_pio_readw(QPCIBus *bus, uint32_t addr)
{
    QGenericPCIBus *s = container_of(bus, QGenericPCIBus, bus.legacy);

    return qtest_readw(bus->qts, s->gpex_pio_base + addr);
}

static void qpci_generic_pio_writew(QPCIBus *bus, uint32_t addr, uint16_t val)
{
    QGenericPCIBus *s = container_of(bus, QGenericPCIBus, bus.legacy);

    qtest_writew(bus->qts, s->gpex_pio_base + addr, val);
}

static uint32_t qpci_generic_pio_readl(QPCIBus *bus, uint32_t addr)
{
    QGenericPCIBus *s = container_of(bus, QGenericPCIBus, bus.legacy);

    return qtest_readl(bus->qts, s->gpex_pio_base + addr);
}

static void qpci_generic_pio_writel(QPCIBus *bus, uint32_t addr, uint32_t val)
{
    QGenericPCIBus *s = container_of(bus, QGenericPCIBus, bus.legacy);

    qtest_writel(bus->qts, s->gpex_pio_base + addr, val);
}

static uint64_t qpci_generic_pio_readq(QPCIBus *bus, uint32_t addr)
{
    QGenericPCIBus *s = container_of(bus, QGenericPCIBus, bus.legacy);

    return qtest_readq(bus->qts, s->gpex_pio_base + addr);
}

static void qpci_generic_pio_writeq(QPCIBus *bus, uint32_t addr, uint64_t val)
{
    QGenericPCIBus *s = container_of(bus, QGenericPCIBus, bus.legacy);

    qtest_writeq(bus->qts, s->gpex_pio_base + addr, val);
}

static void qpci_generic_memread(QPCIBus *bus, uint32_t addr, void *buf, size_t len)
{
    qtest_memread(bus->qts, addr, buf, len);
}

static void qpci_generic_memwrite(QPCIBus *bus, uint32_t addr,
                                  const void *buf, size_t len)
{
    qtest_memwrite(bus->qts, addr, buf, len);
}

static uint8_t qpci_generic_config_readb(QPCIBus *bus, int devfn, uint8_t offset)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus.legacy);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint8_t val;

    qtest_memread(bus->qts, addr, &val, 1);
    return val;
}

static uint16_t qpci_generic_config_readw(QPCIBus *bus, int devfn, uint8_t offset)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus.legacy);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint16_t val;

    qtest_memread(bus->qts, addr, &val, 2);
    return le16_to_cpu(val);
}

static uint32_t qpci_generic_config_readl(QPCIBus *bus, int devfn, uint8_t offset)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus.legacy);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint32_t val;

    qtest_memread(bus->qts, addr, &val, 4);
    return le32_to_cpu(val);
}

static void
qpci_generic_config_writeb(QPCIBus *bus, int devfn, uint8_t offset, uint8_t value)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus.legacy);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);

    qtest_memwrite(bus->qts, addr, &value, 1);
}

static void
qpci_generic_config_writew(QPCIBus *bus, int devfn, uint8_t offset, uint16_t value)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus.legacy);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint16_t val = cpu_to_le16(value);

    qtest_memwrite(bus->qts, addr, &val, 2);
}

static void
qpci_generic_config_writel(QPCIBus *bus, int devfn, uint8_t offset, uint32_t value)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus.legacy);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint32_t val = cpu_to_le32(value);

    qtest_memwrite(bus->qts, addr, &val, 4);
}

static uint8_t qpcie_generic_config_readb(
    QPCIeBus *bus, int devfn, uint16_t offset)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint8_t val;

    qtest_memread(bus->legacy.qts, addr, &val, 1);
    return val;
}

static uint16_t qpcie_generic_config_readw(
    QPCIeBus *bus, int devfn, uint16_t offset)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint16_t val;

    qtest_memread(bus->legacy.qts, addr, &val, 2);
    return le16_to_cpu(val);
}

static uint32_t qpcie_generic_config_readl(
    QPCIeBus *bus, int devfn, uint16_t offset)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint32_t val;

    qtest_memread(bus->legacy.qts, addr, &val, 4);
    return le32_to_cpu(val);
}

static void qpcie_generic_config_writeb(
    QPCIeBus *bus, int devfn, uint16_t offset, uint8_t value)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);

    qtest_memwrite(bus->legacy.qts, addr, &value, 1);
}

static void qpcie_generic_config_writew(
    QPCIeBus *bus, int devfn, uint16_t offset, uint16_t value)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint16_t val = cpu_to_le16(value);

    qtest_memwrite(bus->legacy.qts, addr, &val, 2);
}

static void qpcie_generic_config_writel(
    QPCIeBus *bus, int devfn, uint16_t offset, uint32_t value)
{
    QGenericPCIBus *gbus = container_of(bus, QGenericPCIBus, bus);
    uint64_t addr = gbus->ecam_alloc_ptr + ((0 << 20) | (devfn << 12) | offset);
    uint32_t val = cpu_to_le32(value);

    qtest_memwrite(bus->legacy.qts, addr, &val, 4);
}

static void *qpci_generic_get_driver(void *obj, const char *interface)
{
    QGenericPCIBus *qpci = obj;
    if (!g_strcmp0(interface, "pci-bus")) {
        return &qpci->bus.legacy;
    } else if (!g_strcmp0(interface, "pcie-bus")) {
        return &qpci->bus;
    }
    fprintf(stderr, "%s not present in pci-bus-generic\n", interface);
    g_assert_not_reached();
}

void qpci_init_generic(QGenericPCIBus *qpci, QTestState *qts,
                       QGuestAllocator *alloc, bool hotpluggable)
{
    assert(qts);

    qpci->gpex_pio_base = 0x3eff0000;
    qpci->bus.legacy.not_hotpluggable = !hotpluggable;
    qpci->bus.legacy.has_buggy_msi = false;

    qpci->bus.legacy.pio_readb = qpci_generic_pio_readb;
    qpci->bus.legacy.pio_readw = qpci_generic_pio_readw;
    qpci->bus.legacy.pio_readl = qpci_generic_pio_readl;
    qpci->bus.legacy.pio_readq = qpci_generic_pio_readq;

    qpci->bus.legacy.pio_writeb = qpci_generic_pio_writeb;
    qpci->bus.legacy.pio_writew = qpci_generic_pio_writew;
    qpci->bus.legacy.pio_writel = qpci_generic_pio_writel;
    qpci->bus.legacy.pio_writeq = qpci_generic_pio_writeq;

    qpci->bus.legacy.memread = qpci_generic_memread;
    qpci->bus.legacy.memwrite = qpci_generic_memwrite;

    qpci->bus.legacy.config_readb = qpci_generic_config_readb;
    qpci->bus.legacy.config_readw = qpci_generic_config_readw;
    qpci->bus.legacy.config_readl = qpci_generic_config_readl;

    qpci->bus.legacy.config_writeb = qpci_generic_config_writeb;
    qpci->bus.legacy.config_writew = qpci_generic_config_writew;
    qpci->bus.legacy.config_writel = qpci_generic_config_writel;

    qpci->bus.config_readb = qpcie_generic_config_readb;
    qpci->bus.config_readw = qpcie_generic_config_readw;
    qpci->bus.config_readl = qpcie_generic_config_readl;

    qpci->bus.config_writeb = qpcie_generic_config_writeb;
    qpci->bus.config_writew = qpcie_generic_config_writew;
    qpci->bus.config_writel = qpcie_generic_config_writel;

    qpci->bus.legacy.qts = qts;
    qpci->bus.legacy.pio_alloc_ptr = 0x0000;
    qpci->bus.legacy.pio_limit = 0x10000;
    qpci->bus.legacy.mmio_alloc_ptr = 0x10000000;
    qpci->bus.legacy.mmio_limit = 0x2eff0000;
    qpci->ecam_alloc_ptr = 0x4010000000;

    qpci->obj.get_driver = qpci_generic_get_driver;
}

static void qpci_generic_register_nodes(void)
{
    qos_node_create_driver("pci-bus-generic", NULL);
    qos_node_produces("pci-bus-generic", "pci-bus");
    qos_node_produces("pci-bus-generic", "pcie-bus");
}

static void qpci_generic_pci_register_nodes(void)
{
    qos_node_create_driver("generic-pcihost", NULL);
    qos_node_contains("generic-pcihost", "pci-bus-generic", NULL);
}

libqos_init(qpci_generic_register_nodes);
libqos_init(qpci_generic_pci_register_nodes);
