/*
 * QTest testcase for PCIe
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "libqtest.h"
#include "qemu/module.h"
#include "libqos/qgraph.h"
#include "libqos/pci.h"

/* Tests only initialization so far. TODO: Replace with functional tests */
static void nop(void *obj, void *data, QGuestAllocator *alloc)
{
    g_assert_true(false);
}

static void register_pcie_test(void)
{
    qos_node_create_driver("pci-testdev", NULL);
    /* qos_node_consumes("pci-testdev", ); */
    qos_add_test("nop", "pci-testdev", nop, NULL);
}

libqos_init(register_pcie_test);
