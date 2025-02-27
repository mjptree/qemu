/*
 * Generic PCI Express Root Port emulation
 *
 * Copyright (C) 2017 Red Hat Inc
 *
 * Authors:
 *   Marcel Apfelbaum <marcel@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "hw/spdm/spdm-responder.h"
#include "hw/pci/msix.h"
#include "hw/pci/pcie_doe.h"
#include "hw/pci/pcie_ide.h"
#include "hw/pci/pcie_port.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "migration/vmstate.h"
#include "qom/object.h"

#define TYPE_GEN_PCIE_ROOT_PORT                "pcie-root-port"
OBJECT_DECLARE_SIMPLE_TYPE(GenPCIERootPort, GEN_PCIE_ROOT_PORT)

#define GEN_PCIE_ROOT_PORT_AER_OFFSET           0x100
#define GEN_PCIE_ROOT_PORT_ACS_OFFSET \
        (GEN_PCIE_ROOT_PORT_AER_OFFSET + PCI_ERR_SIZEOF)
#define GEN_PCIE_ROOT_PORT_DOE_OFFSET \
        (GEN_PCIE_ROOT_PORT_ACS_OFFSET + PCI_ACS_SIZEOF)
#define GEN_PCIE_ROOT_PORT_IDE_OFFSET \
        (GEN_PCIE_ROOT_PORT_DOE_OFFSET + PCI_DOE_SIZEOF)

#define GEN_PCIE_ROOT_PORT_MSIX_NR_VECTOR       1
#define GEN_PCIE_ROOT_DEFAULT_IO_RANGE          4096

struct GenPCIERootPort {
    /*< private >*/
    PCIESlot parent_obj;
    /*< public >*/

    bool migrate_msix;

    /* additional resources to reserve */
    PCIResReserve res_reserve;

    /* CMA/SPDM */
    SPDMResponder *spdm_responder;
};

static uint8_t gen_rp_aer_vector(const PCIDevice *d)
{
    return 0;
}

static int gen_rp_interrupts_init(PCIDevice *d, Error **errp)
{
    int rc;

    rc = msix_init_exclusive_bar(d, GEN_PCIE_ROOT_PORT_MSIX_NR_VECTOR, 0, errp);

    if (rc < 0) {
        assert(rc == -ENOTSUP);
    } else {
        msix_vector_use(d, 0);
    }

    return rc;
}

static void gen_rp_interrupts_uninit(PCIDevice *d)
{
    msix_uninit_exclusive_bar(d);
}

static bool gen_rp_test_migrate_msix(void *opaque, int version_id)
{
    GenPCIERootPort *rp = opaque;

    return rp->migrate_msix;
}

static uint32_t gen_rp_config_read(PCIDevice *pdev, uint32_t addr, int len)
{
    uint32_t data = 0;
    if (pcie_doe_read_config(&pdev->doe_spdm, addr, len, &data)) {
        return data;
    }

    return pci_default_read_config(pdev, addr, len);
}

static void gen_rp_config_write(
    PCIDevice *pdev, uint32_t addr, uint32_t data, int len)
{
    pcie_doe_write_config(&pdev->doe_spdm, addr, data, len);
    pci_default_write_config(pdev, addr, data, len);
}

static bool gen_rp_send_message(DeviceState *dev, size_t message_size,
                                const void *message)
{
    PCIDevice *pdev = PCI_DEVICE(dev);
    return pcie_doe_send_message(&pdev->doe_spdm, message_size, message);
}

static bool gen_rp_receive_message(DeviceState *dev, size_t *message_size,
                                   void **message)
{
    PCIDevice *pdev = PCI_DEVICE(dev);
    return pcie_doe_receive_message(&pdev->doe_spdm, message_size, message);
}

static bool gen_rp_get_response(DeviceState *dev, const uint32_t *session_id,
                                size_t request_size, const SPDMHeader *request,
                                size_t *response_size, SPDMHeader *response)
{
    PCIDevice *pdev = PCI_DEVICE(dev);
    GenPCIERootPort *grp = GEN_PCIE_ROOT_PORT(dev);
    SPDMPCIDefined *request_header, *response_header;
    PCIPayload *request_payload, *response_payload;
    size_t request_payload_size, response_payload_size;
    SPDMErrorCode error_code = 0;
    bool success;

    assert(session_id && response && response_size);
    assert(*response_size >= sizeof(SPDMPCIDefined) + sizeof(PCIPayload));

    if (request_size < sizeof(SPDMPCIDefined) + sizeof(PCIPayload)) {
        return spdm_responder_get_response_error(
            grp->spdm_responder, SPDM_ERROR_CODE_INVALID_REQUEST, 0,
            response_size, response);
    }

    request_header = (SPDMPCIDefined *)request;
    response_header = (SPDMPCIDefined *)response;

    if (request_header->vendor_defined.header.request_response_code !=
        SPDM_REQUEST_CODE_VENDOR_DEFINED_REQUEST)
    {
        return spdm_responder_get_response_error(
            grp->spdm_responder, SPDM_ERROR_CODE_UNSUPPORTED_REQUEST,
            request_header->vendor_defined.header.request_response_code,
            response_size, response);
    }

    if (request_header->vendor_defined.standard_id !=
        SPDM_STANDARD_ID_PCISIG ||
        request_header->vendor_defined.len !=
            sizeof(request_header->vendor_id) ||
        request_header->vendor_id != SPDM_VENDOR_ID_PCISIG) {
        return spdm_responder_get_response_error(
            grp->spdm_responder, SPDM_ERROR_CODE_INVALID_REQUEST, 0,
            response_size, response);
    }

    if (request_size < sizeof(SPDMPCIDefined) + request_header->req_length) {
        return spdm_responder_get_response_error(
            grp->spdm_responder, SPDM_ERROR_CODE_INVALID_REQUEST, 0,
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
            grp->spdm_responder, error_code, SPDM_ERROR_CODE_INVALID_REQUEST,
            response_size, response);
    default:
        return spdm_responder_get_response_error(
            grp->spdm_responder, error_code, SPDM_ERROR_CODE_INVALID_REQUEST,
            response_size, response);
    };

    if (!success) {
        return false;
    }

    /* Error code 0 is currently reserved and not a valid error code. */
    if (error_code) {
        return spdm_responder_get_response_error(
            grp->spdm_responder, error_code, 0, response_size, response);
    }

    if (UINT16_MAX < response_payload_size) {
        error_report(
            "SPDM vendor defined response payload size exceeds maximum "
            "representable response length (size=%lu)", response_payload_size);
        return spdm_responder_get_response_error(
            grp->spdm_responder, error_code,
            SPDM_ERROR_CODE_OPERATION_FAILED, response_size, response);
    }

    response_header->vendor_defined.header.spdm_version =
        spdm_responder_get_connection_version(grp->spdm_responder);
    response_header->vendor_defined.header.request_response_code =
        SPDM_RESPONSE_CODE_VENDOR_DEFINED_RESPONSE;
    response_header->vendor_defined.standard_id = SPDM_STANDARD_ID_PCISIG;
    response_header->vendor_defined.len = sizeof(response_header->vendor_id);
    response_header->vendor_id = SPDM_VENDOR_ID_PCISIG;
    response_header->req_length = response_payload_size;
    return true;
}

static bool gen_rp_handle_request(DOECap *cap)
{
    GenPCIERootPort *d = GEN_PCIE_ROOT_PORT(cap->pdev);
    Error *local_error;

    if (!spdm_responder_dispatch_message(d->spdm_responder, &local_error)) {
        error_report_err(local_error);
        return false;
    }

    return true;
}

static void gen_rp_exit(PCIDevice *pdev)
{
    PCIERootPortClass *rpc = PCIE_ROOT_PORT_GET_CLASS(pdev);

    pcie_doe_fini(&pdev->doe_spdm);
    pcie_ide_fini(pdev);
    rpc->parent_exit(pdev);
}

static DOEProtocol doe_protocols[] = {
    { PCI_VENDOR_ID_PCI_SIG, PCI_SIG_DOE_CMA, gen_rp_handle_request },
    { PCI_VENDOR_ID_PCI_SIG, PCI_SIG_DOE_SECURED_CMA, gen_rp_handle_request },
    { },
};

static SelectiveIDEStream sel_ide_streams[] = {
    { .ide_addr_assoc_blks_num = 0 },
};

static void gen_rp_realize(DeviceState *dev, Error **errp)
{
    ERRP_GUARD();
    PCIDevice *d = PCI_DEVICE(dev);
    PCIESlot *s = PCIE_SLOT(d);
    GenPCIERootPort *grp = GEN_PCIE_ROOT_PORT(d);
    PCIERootPortClass *rpc = PCIE_ROOT_PORT_GET_CLASS(d);
    bool ide_km_supported = false;

    rpc->parent_realize(dev, errp);
    if (*errp) {
        return;
    }

    /*
     * reserving IO space led to worse issues in 6.1, when this hunk was
     * introduced. (see commit: 211afe5c69b59). Keep this broken for 6.1
     * machine type ABI compatibility only
     */
    if (s->hide_native_hotplug_cap && grp->res_reserve.io == -1 && s->hotplug) {
        grp->res_reserve.io = GEN_PCIE_ROOT_DEFAULT_IO_RANGE;
    }
    int rc = pci_bridge_qemu_reserve_cap_init(d, 0,
                                              grp->res_reserve, errp);

    if (rc < 0) {
        gen_rp_exit(d);
        return;
    }

    if (!grp->res_reserve.io) {
        pci_word_test_and_clear_mask(d->wmask + PCI_COMMAND,
                                     PCI_COMMAND_IO);
        d->wmask[PCI_IO_BASE] = 0;
        d->wmask[PCI_IO_LIMIT] = 0;
    }

    if (grp->spdm_responder) {
        pcie_doe_init(d, &d->doe_spdm, GEN_PCIE_ROOT_PORT_DOE_OFFSET,
                      doe_protocols, true, 0);

        if (!device_spdm_responder_init(DEVICE(d), grp->spdm_responder,
                                        gen_rp_send_message,
                                        gen_rp_receive_message,
                                        gen_rp_get_response, errp)) {
            gen_rp_exit(d);
            return;
        }

        ide_km_supported = true;
    }

    if (d->cap_present & QEMU_PCIE_CAP_IDE) {
        pcie_ide_init(d, GEN_PCIE_ROOT_PORT_IDE_OFFSET, ide_km_supported, NULL,
                      0, sel_ide_streams, ARRAY_SIZE(sel_ide_streams));
    }
}

static const VMStateDescription vmstate_rp_dev = {
    .name = "pcie-root-port",
    .priority = MIG_PRI_PCI_BUS,
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = pcie_cap_slot_post_load,
    .fields = (const VMStateField[]) {
        VMSTATE_PCI_DEVICE(parent_obj.parent_obj.parent_obj, PCIESlot),
        VMSTATE_STRUCT(parent_obj.parent_obj.parent_obj.exp.aer_log,
                       PCIESlot, 0, vmstate_pcie_aer_log, PCIEAERLog),
        VMSTATE_MSIX_TEST(parent_obj.parent_obj.parent_obj.parent_obj,
                          GenPCIERootPort,
                          gen_rp_test_migrate_msix),
        VMSTATE_END_OF_LIST()
    }
};

static const Property gen_rp_props[] = {
    DEFINE_PROP_BOOL("x-migrate-msix", GenPCIERootPort,
                     migrate_msix, true),
    DEFINE_PROP_UINT32("bus-reserve", GenPCIERootPort,
                       res_reserve.bus, -1),
    DEFINE_PROP_SIZE("io-reserve", GenPCIERootPort,
                     res_reserve.io, -1),
    DEFINE_PROP_SIZE("mem-reserve", GenPCIERootPort,
                     res_reserve.mem_non_pref, -1),
    DEFINE_PROP_SIZE("pref32-reserve", GenPCIERootPort,
                     res_reserve.mem_pref_32, -1),
    DEFINE_PROP_SIZE("pref64-reserve", GenPCIERootPort,
                     res_reserve.mem_pref_64, -1),
    DEFINE_PROP_PCIE_LINK_SPEED("x-speed", PCIESlot,
                                speed, PCIE_LINK_SPEED_16),
    DEFINE_PROP_PCIE_LINK_WIDTH("x-width", PCIESlot,
                                width, PCIE_LINK_WIDTH_32),
    DEFINE_PROP_BIT("x-pcie-idecap-init", PCIDevice, cap_present,
                    QEMU_PCIE_IDE_BITNR, false),
    DEFINE_PROP_LINK("x-spdm-responder", GenPCIERootPort, spdm_responder,
                     TYPE_SPDM_RESPONDER, SPDMResponder *),
};

static void gen_rp_dev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
    PCIERootPortClass *rpc = PCIE_ROOT_PORT_CLASS(klass);

    rpc->parent_exit = k->exit;

    k->config_write = gen_rp_config_write;
    k->config_read = gen_rp_config_read;
    k->vendor_id = PCI_VENDOR_ID_REDHAT;
    k->device_id = PCI_DEVICE_ID_REDHAT_PCIE_RP;
    k->exit = gen_rp_exit;
    dc->desc = "PCI Express Root Port";
    dc->vmsd = &vmstate_rp_dev;
    device_class_set_props(dc, gen_rp_props);

    device_class_set_parent_realize(dc, gen_rp_realize, &rpc->parent_realize);

    rpc->aer_vector = gen_rp_aer_vector;
    rpc->interrupts_init = gen_rp_interrupts_init;
    rpc->interrupts_uninit = gen_rp_interrupts_uninit;
    rpc->aer_offset = GEN_PCIE_ROOT_PORT_AER_OFFSET;
    rpc->acs_offset = GEN_PCIE_ROOT_PORT_ACS_OFFSET;
}

static const TypeInfo gen_rp_dev_info = {
    .name          = TYPE_GEN_PCIE_ROOT_PORT,
    .parent        = TYPE_PCIE_ROOT_PORT,
    .instance_size = sizeof(GenPCIERootPort),
    .class_init    = gen_rp_dev_class_init,
};

 static void gen_rp_register_types(void)
 {
    type_register_static(&gen_rp_dev_info);
 }
 type_init(gen_rp_register_types)
