/*
 * PCIe Data Object Exchange
 *
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "qemu/range.h"
#include "hw/pci/pci.h"
#include "hw/pci/pcie.h"
#include "hw/pci/pcie_doe.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"

#define DWORD_BYTE 4

typedef struct DoeDiscoveryReq {
    DOEHeader header;
    uint8_t index;
    uint8_t reserved[3];
} QEMU_PACKED DoeDiscoveryReq;

typedef struct DoeDiscoveryRsp {
    DOEHeader header;
    uint16_t vendor_id;
    uint8_t data_obj_type;
    uint8_t next_index;
} QEMU_PACKED DoeDiscoveryRsp;

static bool pcie_doe_data_object_ready(DOECap *cap)
{
    return !g_queue_is_empty(cap->read_data_mailbox);
}

static bool pcie_doe_discovery(DOECap *doe_cap)
{
    uint8_t request_buffer[sizeof(DoeDiscoveryReq)];
    uint8_t response_buffer[sizeof(DoeDiscoveryRsp)];
    size_t request_size;
    size_t response_size;

    DoeDiscoveryReq *req;
    DoeDiscoveryRsp *rsp;
    uint8_t index, next_index;
    DOEProtocol *prot;

    request_size = sizeof(request_buffer);
    response_size = sizeof(response_buffer);
    req = (DoeDiscoveryReq *)&request_buffer;

    if (!pcie_doe_receive_message(doe_cap, &request_size, (void **)&req)) {
        return false;
    }

    /* Discard request if length does not match DoeDiscoveryReq */
    if (pcie_doe_data_object_length_in_bytes(&req->header) != request_size) {
        return false;
    }

    index = req->index;
    rsp = (DoeDiscoveryRsp *)&response_buffer;
    rsp->header = (DOEHeader) {
        .vendor_id = PCI_VENDOR_ID_PCI_SIG,
        .data_obj_type = PCI_SIG_DOE_DISCOVERY,
        .length = DIV_ROUND_UP(response_size, DWORD_BYTE),
    };

    /* Point to the requested protocol, index 0 must be Discovery */
    if (index == 0) {
        rsp->vendor_id = PCI_VENDOR_ID_PCI_SIG;
        rsp->data_obj_type = PCI_SIG_DOE_DISCOVERY;
    } else if (index < doe_cap->protocol_num) {
        prot = &doe_cap->protocols[index - 1];
        rsp->vendor_id = prot->vendor_id;
        rsp->data_obj_type = prot->data_obj_type;
    } else {
        rsp->vendor_id = 0xFFFF;
        rsp->data_obj_type = 0xFF;
    }

    next_index = index + 1;

    if (next_index == doe_cap->protocol_num) {
        rsp->next_index = 0;
    } else {
        rsp->next_index = next_index;
    }

    return pcie_doe_send_message(doe_cap, response_size, (void *)rsp);
}

static void pcie_doe_data_object_free(void *data_object)
{
    g_byte_array_unref(data_object);
}

static void pcie_doe_reset_mbox(DOECap *st)
{
    g_queue_clear_full(st->read_data_mailbox, pcie_doe_data_object_free);
    st->read_data_bytes_in_flight = 0;

    g_queue_clear_full(st->write_data_mailbox, pcie_doe_data_object_free);
    st->write_data_bytes_in_flight = 0;
}

void pcie_doe_init(PCIDevice *dev, DOECap *doe_cap, uint16_t offset,
                   DOEProtocol *protocols, bool intr, uint16_t vec)
{
    pcie_add_capability(dev, PCI_EXT_CAP_ID_DOE, 0x1, offset,
                        PCI_DOE_SIZEOF);

    doe_cap->pdev = dev;
    doe_cap->offset = offset;

    if (intr && (msi_present(dev) || msix_present(dev))) {
        doe_cap->cap.intr = intr;
        doe_cap->cap.vec = vec;
    }

    doe_cap->read_data_mailbox = g_queue_new();
    doe_cap->read_data_bytes_capacity = PCI_DOE_DW_SIZE_MAX * DWORD_BYTE;

    doe_cap->write_data_mailbox = g_queue_new();
    doe_cap->write_data_bytes_capacity = PCI_DOE_DW_SIZE_MAX * DWORD_BYTE;

    pcie_doe_reset_mbox(doe_cap);

    doe_cap->protocols = protocols;
    for (; protocols->vendor_id; protocols++) {
        doe_cap->protocol_num++;
    }
    assert(doe_cap->protocol_num < PCI_DOE_PROTOCOL_NUM_MAX);

    /* Increment to allow for the discovery protocol */
    doe_cap->protocol_num++;
}

void pcie_doe_fini(DOECap *doe_cap)
{
    g_queue_free_full(doe_cap->read_data_mailbox, pcie_doe_data_object_free);
    g_queue_free_full(doe_cap->write_data_mailbox, pcie_doe_data_object_free);
    g_free(doe_cap);
}

uint32_t pcie_doe_build_protocol(DOEProtocol *p)
{
    return DATA_OBJ_BUILD_HEADER1(p->vendor_id, p->data_obj_type);
}

static void pcie_doe_irq_assert(DOECap *doe_cap)
{
    PCIDevice *dev = doe_cap->pdev;

    if (doe_cap->cap.intr && doe_cap->ctrl.intr) {
        if (doe_cap->status.intr) {
            return;
        }
        doe_cap->status.intr = 1;

        if (msix_enabled(dev)) {
            msix_notify(dev, doe_cap->cap.vec);
        } else if (msi_enabled(dev)) {
            msi_notify(dev, doe_cap->cap.vec);
        }
    }
}

static void pcie_doe_set_error(DOECap *doe_cap, bool err)
{
    doe_cap->status.error = err;

    if (err) {
        pcie_doe_irq_assert(doe_cap);
    }
}

/*
 * Check incoming request in write_mbox for protocol format
 */
static void pcie_doe_prepare_rsp(DOECap *doe_cap)
{
    GQueue *write_data_mailbox = doe_cap->write_data_mailbox;
    GByteArray *data_object = g_queue_peek_tail(write_data_mailbox);
    DOEHeader *header;
    uint32_t header1;
    bool success = false;
    int p;
    bool (*handle_request)(DOECap *) = NULL;

    if (doe_cap->status.error || !data_object) {
        return;
    }

    if (data_object->len < sizeof(DOEHeader)) {
        /*
         * Currently, handle_request is responsible for popping the write data
         * mailbox. However, we can't proceed here, so we need to clean up the
         * invalid data object.
         */
        g_queue_pop_tail(write_data_mailbox);
        g_byte_array_unref(data_object);
        return;
    }

    /*
     * Setting DOE_GO in PCI_DOE_CAP_CONTROL means subsequent writes to the
     * write data mailbox constitute a new data object.
     */
    g_queue_push_head(write_data_mailbox, g_byte_array_new());
    header = (DOEHeader *)data_object->data;
    header1 = DATA_OBJ_BUILD_HEADER1(header->vendor_id, header->data_obj_type);

    if (header1 ==
        DATA_OBJ_BUILD_HEADER1(PCI_VENDOR_ID_PCI_SIG, PCI_SIG_DOE_DISCOVERY)) {
        handle_request = pcie_doe_discovery;
    } else {
        for (p = 0; p < doe_cap->protocol_num - 1; p++) {
            if (header1 ==
                pcie_doe_build_protocol(&doe_cap->protocols[p])) {
                handle_request = doe_cap->protocols[p].handle_request;
                break;
            }
        }
    }

    /*
     * PCIe r6 DOE 6.30.1:
     * If the number of DW transferred does not match the
     * indicated Length for a data object, then the
     * data object must be silently discarded.
     */
    if (handle_request && (data_object->len ==
        pcie_doe_data_object_length_in_bytes(header))) {
        success = handle_request(doe_cap);
    }

    if (success) {
        pcie_doe_irq_assert(doe_cap);
    } else {
        pcie_doe_reset_mbox(doe_cap);
    }
}

/*
 * Read from DOE config space.
 * Return false if the address not within DOE_CAP range.
 */
bool pcie_doe_read_config(DOECap *doe_cap, uint32_t addr, int size,
                          uint32_t *buf)
{
    uint32_t shift;
    uint16_t doe_offset = doe_cap->offset;

    if (!range_covers_byte(doe_offset + PCI_EXP_DOE_CAP,
                           PCI_DOE_SIZEOF - 4, addr)) {
        return false;
    }

    addr -= doe_offset;
    *buf = 0;

    if (range_covers_byte(PCI_EXP_DOE_CAP, DWORD_BYTE, addr)) {
        *buf = FIELD_DP32(*buf, PCI_DOE_CAP_REG, INTR_SUPP,
                          doe_cap->cap.intr);
        *buf = FIELD_DP32(*buf, PCI_DOE_CAP_REG, DOE_INTR_MSG_NUM,
                          doe_cap->cap.vec);
    } else if (range_covers_byte(PCI_EXP_DOE_CTRL, DWORD_BYTE, addr)) {
        /* Must return ABORT=0 and GO=0 */
        *buf = FIELD_DP32(*buf, PCI_DOE_CAP_CONTROL, DOE_INTR_EN,
                          doe_cap->ctrl.intr);
    } else if (range_covers_byte(PCI_EXP_DOE_STATUS, DWORD_BYTE, addr)) {
        *buf = FIELD_DP32(*buf, PCI_DOE_CAP_STATUS, DOE_BUSY,
                          doe_cap->status.busy);
        *buf = FIELD_DP32(*buf, PCI_DOE_CAP_STATUS, DOE_INTR_STATUS,
                          doe_cap->status.intr);
        *buf = FIELD_DP32(*buf, PCI_DOE_CAP_STATUS, DOE_ERROR,
                          doe_cap->status.error);
        *buf = FIELD_DP32(*buf, PCI_DOE_CAP_STATUS, DATA_OBJ_RDY,
                          pcie_doe_data_object_ready(doe_cap));
    /* Mailbox should be DW accessed */
    } else if (addr == PCI_EXP_DOE_WR_DATA_MBOX && size == DWORD_BYTE) {
        *buf = pcie_doe_read_doe_write_data_mailbox_register(doe_cap);
    } else if (addr == PCI_EXP_DOE_RD_DATA_MBOX && size == DWORD_BYTE) {
        *buf = pcie_doe_read_doe_read_data_mailbox_register(doe_cap);
    }

    /* Process Alignment */
    shift = addr % DWORD_BYTE;
    *buf = extract32(*buf, shift * 8, size * 8);

    return true;
}

/*
 * Write to DOE config space.
 * Return if the address not within DOE_CAP range or receives an abort
 */
void pcie_doe_write_config(DOECap *doe_cap,
                           uint32_t addr, uint32_t val, int size)
{
    uint16_t doe_offset = doe_cap->offset;
    uint32_t shift;

    if (!range_covers_byte(doe_offset + PCI_EXP_DOE_CAP,
                           PCI_DOE_SIZEOF - 4, addr)) {
        return;
    }

    /* Process Alignment */
    shift = addr % DWORD_BYTE;
    addr -= (doe_offset + shift);
    val = deposit32(val, shift * 8, size * 8, val);

    switch (addr) {
    case PCI_EXP_DOE_CTRL:
        if (FIELD_EX32(val, PCI_DOE_CAP_CONTROL, DOE_ABORT)) {
            pcie_doe_set_error(doe_cap, 0);
            pcie_doe_reset_mbox(doe_cap);
            return;
        }

        if (FIELD_EX32(val, PCI_DOE_CAP_CONTROL, DOE_GO)) {
            pcie_doe_prepare_rsp(doe_cap);
        }

        if (FIELD_EX32(val, PCI_DOE_CAP_CONTROL, DOE_INTR_EN)) {
            doe_cap->ctrl.intr = 1;
        /* Clear interrupt bit located within the first byte */
        } else if (shift == 0) {
            doe_cap->ctrl.intr = 0;
        }
        break;
    case PCI_EXP_DOE_STATUS:
        if (FIELD_EX32(val, PCI_DOE_CAP_STATUS, DOE_INTR_STATUS)) {
            doe_cap->status.intr = 0;
        }
        break;
    case PCI_EXP_DOE_RD_DATA_MBOX:
        /* Mailbox should be DW accessed */
        if (size != DWORD_BYTE) {
            return;
        }

        pcie_doe_write_doe_read_data_mailbox_register(doe_cap, val);
        break;
    case PCI_EXP_DOE_WR_DATA_MBOX:
        /* Mailbox should be DW accessed */
        if (size != DWORD_BYTE) {
            return;
        }

        pcie_doe_write_doe_write_data_mailbox_register(doe_cap, val);
        break;
    case PCI_EXP_DOE_CAP:
        /* fallthrough */
    default:
        break;
    }
}

uint32_t pcie_doe_read_doe_write_data_mailbox_register(DOECap *cap)
{
    return 0;
}

void pcie_doe_write_doe_write_data_mailbox_register(DOECap *cap, uint32_t data)
{
    GQueue *write_data_mailbox = cap->write_data_mailbox;
    uint32_t remaining =
        cap->write_data_bytes_capacity - cap->write_data_bytes_in_flight;
    GByteArray *data_object;

    if (remaining < sizeof(data)) {
        /* overflow */
        pcie_doe_set_error(cap, true);
        return;
    }

    data_object = g_queue_peek_head(write_data_mailbox);

    if (!data_object) {
        data_object = g_byte_array_new();
        g_queue_push_head(write_data_mailbox, data_object);
    }

    g_byte_array_append(data_object, (guint8 *)&data, sizeof(data));
    cap->write_data_bytes_in_flight += sizeof(data);
}

uint32_t pcie_doe_read_doe_read_data_mailbox_register(DOECap *cap)
{
    GQueue *read_data_mailbox = cap->read_data_mailbox;
    GByteArray *data_object = g_queue_peek_head(read_data_mailbox);
    uint32_t data = 0;

    if (data_object) {
        if (data_object->len >= sizeof(data)) {
            memcpy(&data, data_object->data, sizeof(data));
        } else {
            /* data object padding error */
            pcie_doe_set_error(cap, true);
        }
    }

    return data;
}

void pcie_doe_write_doe_read_data_mailbox_register(DOECap *cap, uint32_t data)
{
    GQueue *read_data_mailbox = cap->read_data_mailbox;
    GByteArray *data_object = g_queue_peek_head(read_data_mailbox);

    if (data_object) {
        if (data_object->len > sizeof(data)) {
            cap->read_data_bytes_in_flight -= sizeof(data);
            g_byte_array_remove_range(data_object, 0, sizeof(data));
        } else {
            cap->read_data_bytes_in_flight -= data_object->len;
            g_queue_pop_head(read_data_mailbox);
            g_byte_array_unref(data_object);
        }
    } else {
        /* underflow */
        pcie_doe_set_error(cap, true);
    }
}

bool pcie_doe_send_message(
    DOECap *cap, size_t message_size, const void *message)
{
    GQueue *read_data_mailbox = cap->read_data_mailbox;
    size_t remaining =
        cap->read_data_bytes_capacity - cap->read_data_bytes_in_flight;
    GByteArray *data_object;

    if (remaining < message_size) {
        pcie_doe_set_error(cap, true);
        return false;
    }

    data_object =
        g_byte_array_append(g_byte_array_new(), message, message_size);
    cap->read_data_bytes_in_flight += data_object->len;
    g_queue_push_tail(read_data_mailbox, data_object);
    return true;
}

bool pcie_doe_receive_message(
    DOECap *cap, size_t *message_size, void **message)
{
    GQueue *write_data_mailbox = cap->write_data_mailbox;
    GByteArray *data_object = g_queue_pop_tail(write_data_mailbox);
    size_t buffer_size = *message_size;

    if (data_object) {
        if (likely(data_object->len <= buffer_size)) {
            *message_size = data_object->len;
        }

        memcpy(*message, data_object->data, *message_size);
        cap->write_data_bytes_in_flight -= data_object->len;
        g_byte_array_unref(data_object);
    } else {
        /* discard (almost) silently */
        *message_size = 0;
    }

    return true;
}
