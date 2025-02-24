#include "qemu/osdep.h"
#include "hw/pci/pci_device.h"
#include "hw/pci/pcie.h"
#include "hw/pci/pcie_ide.h"

/*
 * static bool pcie_tee_io_supported(PCIDevice *dev)
 * {
 *     return pci_get_long(dev->config + dev->exp.exp_cap + PCI_EXP_DEVCAP) &
 *         PCI_EXP_DEVCAP_TEE_IO;
 * }
 */

static bool pcie_link_ide_enabled(PCIDevice *dev, LinkIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_LNK_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, EN);
}

static void pcie_link_ide_config_write_state(
    PCIDevice *dev, LinkIDEStream *stream, IDEStreamState state)
{
    uint8_t *stream_status =
        dev->config + stream->offset + PCI_EXP_LNK_IDE_STREAM_STATUS;
    uint32_t reg = pci_get_long(stream_status);
    reg = FIELD_DP32(reg, PCI_LNK_IDE_STREAM_STATUS_REG, STATE, state);
    pci_set_long(stream_status, reg);
}

static void pcie_link_ide_config_sel_algo_writable(
    PCIDevice *dev, LinkIDEStream *stream, bool writable)
{
    uint8_t *stream_ctrl =
        dev->wmask + stream->offset + PCI_EXP_LNK_IDE_STREAM_CTRL;
    uint32_t reg, mask = writable ? IDE_SEL_ALGO_MASK : 0;
    reg = pci_get_long(stream_ctrl);
    reg = FIELD_DP32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, SEL_ALGO, mask);
    pci_set_long(stream_ctrl, reg);
}

static IDESelAlgo pcie_link_ide_sel_algo(PCIDevice *dev, LinkIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_LNK_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, SEL_ALGO);
}

static void pcie_link_ide_set_sel_algo(
    PCIDevice *dev, LinkIDEStream *stream, IDESelAlgo sel_algo)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_LNK_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    reg = FIELD_DP32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, SEL_ALGO, sel_algo);
    pci_set_long(stream_ctrl, reg);
}

static uint8_t pcie_link_ide_tc(PCIDevice *dev, LinkIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_LNK_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, TC);
}

static void pcie_link_ide_set_tc(
    PCIDevice *dev, LinkIDEStream *stream, uint8_t tc)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_LNK_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    reg = FIELD_DP32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, TC, tc);
    pci_set_long(stream_ctrl, reg);
}

static uint8_t pcie_link_ide_stream_id(PCIDevice *dev, LinkIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_LNK_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, STREAM_ID);
}

static LinkIDEStream *pcie_link_ide_find_by_stream_id(
    PCIDevice *dev, uint8_t stream_id)
{
    LinkIDEStream *stream;
    uint8_t index;

    for (index = 0; index < dev->ide_cap.link_ide_streams_num; ++index) {
        stream = &dev->ide_cap.link_ide_streams[index];

        if (stream_id == pcie_link_ide_stream_id(dev, stream)) {
            return stream;
        }
    }

    return NULL;
}

static bool pcie_sel_ide_enabled(PCIDevice *dev, SelectiveIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, EN);
}

static void pcie_sel_ide_config_sel_algo_writable(
    PCIDevice *dev, SelectiveIDEStream *stream, bool writable)
{
    uint8_t *stream_ctrl =
        dev->wmask + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg, mask = writable ? IDE_SEL_ALGO_MASK : 0;
    reg = pci_get_long(stream_ctrl);
    reg = FIELD_DP32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, SEL_ALGO, mask);
    pci_set_long(stream_ctrl, reg);
}

static IDESelAlgo pcie_sel_ide_sel_algo(
    PCIDevice *dev, SelectiveIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, SEL_ALGO);
}

static void pcie_sel_ide_set_sel_algo(
    PCIDevice *dev, SelectiveIDEStream *stream, IDESelAlgo sel_algo)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    reg = FIELD_DP32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, SEL_ALGO, sel_algo);
    pci_set_long(stream_ctrl, reg);
}

static uint8_t pcie_sel_ide_tc(PCIDevice *dev, SelectiveIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, TC);
}

static void pcie_sel_ide_set_tc(
    PCIDevice *dev, SelectiveIDEStream *stream, uint8_t tc)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    reg = FIELD_DP32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, TC, tc);
    pci_set_long(stream_ctrl, reg);
}

static bool pcie_sel_ide_default_stream(
    PCIDevice *dev, SelectiveIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, DEFAULT_STREAM);
}

static void pcie_sel_ide_config_write_default_stream(
    PCIDevice *dev, SelectiveIDEStream *stream, bool is_default)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    reg = FIELD_DP32(
        reg, PCI_SEL_IDE_STREAM_CTRL_REG, DEFAULT_STREAM, is_default);
    pci_set_long(stream_ctrl, reg);
}

static uint8_t pcie_sel_ide_stream_id(
    PCIDevice *dev, SelectiveIDEStream *stream)
{
    uint8_t *stream_ctrl =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL;
    uint32_t reg = pci_get_long(stream_ctrl);
    return FIELD_EX32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, STREAM_ID);
}

static void pcie_sel_ide_config_write_state(
    PCIDevice *dev, SelectiveIDEStream *stream, IDEStreamState state)
{
    uint8_t *stream_status =
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_STATUS;
    uint32_t reg = pci_get_long(stream_status);
    reg = FIELD_DP32(reg, PCI_SEL_IDE_STREAM_STATUS_REG, STATE, state);
    pci_set_long(stream_status, reg);
}

static bool pcie_sel_ide_rid_assoc_valid(
    PCIDevice *dev, SelectiveIDEStream *stream)
{
    uint8_t *rid_assoc_2 =
        dev->config + stream->offset + PCI_EXP_IDE_RID_ASSOC_2;
    uint32_t reg = pci_get_long(rid_assoc_2);
    return FIELD_EX32(reg, PCI_IDE_RID_ASSOC_2_REG, V);
}

static SelectiveIDEStream *pcie_sel_ide_find_by_stream_id(
    PCIDevice *dev, uint8_t stream_id)
{
    SelectiveIDEStream *stream;
    uint8_t index;

    for (index = 0; index < dev->ide_cap.sel_ide_streams_num; ++index) {
        stream = &dev->ide_cap.sel_ide_streams[index];

        if (stream_id == pcie_sel_ide_stream_id(dev, stream)) {
            return stream;
        }
    }

    return NULL;
}

bool pcie_ide_present(PCIDevice *dev)
{
    return dev->ide_cap.offset;
}

static IDEStream *pcie_ide_stream_find_by_stream_id(
    PCIDevice *dev, uint8_t stream_id)
{
    SelectiveIDEStream *sel_ide_stream;
    LinkIDEStream *link_ide_stream;

    sel_ide_stream = pcie_sel_ide_find_by_stream_id(dev, stream_id);

    if (sel_ide_stream) {
        return &sel_ide_stream->stream;
    }

    link_ide_stream = pcie_link_ide_find_by_stream_id(dev, stream_id);

    if (link_ide_stream) {
        return &link_ide_stream->stream;
    }

    return NULL;
}

static bool pcie_ide_is_algo_supported(PCIDevice *dev, IDESelAlgo sel_algo)
{
    uint8_t *ide_cap = dev->config + dev->ide_cap.offset + PCI_EXP_IDE_CAP;
    uint32_t reg = pci_get_long(ide_cap);
    return FIELD_EX32(reg, PCI_IDE_CAP_REG, SUPP_ALGO) == sel_algo;
}

/*
 * Using the number of TCs supported for Link IDE as a proxy for the TCs
 * supported by this device overall. It is unclear how the device can
 * communicate or system software discover the number of TCs supported on each
 * Selective IDE stream.
 */
static bool pcie_ide_is_tc_supported(PCIDevice *dev, uint8_t tc)
{
    uint8_t *ide_cap = dev->config + dev->ide_cap.offset + PCI_EXP_IDE_CAP;
    uint32_t reg = pci_get_long(ide_cap);
    return FIELD_EX32(reg, PCI_IDE_CAP_REG, NUM_LNK_IDE_STREAMS_SUPP) >= tc;
}

/*
 * static bool pcie_ide_flow_thru_enabled(PCIDevice *dev)
 * {
 *     uint8_t *ide_ctrl = dev->config + dev->ide_cap.offset + PCI_EXP_IDE_CTRL;
 *     uint32_t reg = pci_get_long(ide_ctrl);
 *     return FIELD_EX32(reg, PCI_IDE_CTRL_REG, FLOW_THRU_EN);
 * }
 *
 * static size_t pcie_ide_km_get_key_size(IDESelAlgo sel_algo)
 * {
 *     switch (sel_algo) {
 *     case IDE_SEL_ALGO_AES_GCM256:
 *         return PCI_IDE_KM_AES_GCM256_KEY_SIZE;
 *     default:
 *         return 0;
 *     }
 * }
 */

static void pcie_link_ide_reg_blk_write(
    PCIDevice *dev, LinkIDEStream *link_ide_stream, uint32_t addr, int size,
    uint32_t val)
{
    uint32_t offset = link_ide_stream->offset;
    IDESelAlgo sel_algo;
    uint8_t tc;

    if (!range_covers_byte(offset, PCI_EXT_CAP_LNK_IDE_SIZEOF, addr)) {
        return;
    }

    if (range_covers_byte(
        addr, size, offset + PCI_EXP_LNK_IDE_STREAM_CTRL + 1)) {
        sel_algo = pcie_link_ide_sel_algo(dev, link_ide_stream);

        if (!pcie_ide_is_algo_supported(dev, sel_algo)) {
            /* Undo write if value invalid */
            pcie_link_ide_set_sel_algo(
                dev, link_ide_stream, link_ide_stream->stream.sel_algo);
        } else {
            link_ide_stream->stream.sel_algo = sel_algo;
        }
    }

    if (range_covers_byte(
        addr, size, offset + PCI_EXP_LNK_IDE_STREAM_CTRL + 2)) {
        sel_algo = pcie_link_ide_sel_algo(dev, link_ide_stream);

        if (!pcie_ide_is_algo_supported(dev, sel_algo)) {
            /* Undo write if value invalid */
            pcie_link_ide_set_sel_algo(
                dev, link_ide_stream, link_ide_stream->stream.sel_algo);
        } else {
            link_ide_stream->stream.sel_algo = sel_algo;
        }

        tc = pcie_link_ide_tc(dev, link_ide_stream);

        if (!pcie_ide_is_tc_supported(dev, tc)) {
            /* Undo write if value invalid */
            pcie_link_ide_set_tc(
                dev, link_ide_stream, link_ide_stream->stream.tc);
        } else {
            link_ide_stream->stream.tc = tc;
        }
    }

    if (range_covers_byte(addr, size, offset + PCI_EXP_LNK_IDE_STREAM_CTRL)) {
        if (pcie_link_ide_enabled(dev, link_ide_stream)) {
            pcie_link_ide_config_sel_algo_writable(
                dev, link_ide_stream, false);
        } else {
            pcie_link_ide_config_sel_algo_writable(dev, link_ide_stream, true);
            pcie_link_ide_config_write_state(
                dev, link_ide_stream, IDE_STREAM_STATE_INSECURE);
        }
    }
}

static void pcie_sel_ide_reg_blk_write(
    PCIDevice *dev, SelectiveIDEStream *sel_ide_stream, uint32_t addr,
    int size, uint32_t val)
{
    uint32_t offset = sel_ide_stream->offset;
    uint16_t len = PCI_EXT_CAP_SEL_IDE_SIZEOF +
        sel_ide_stream->ide_addr_assoc_blks_num *
            PCI_EXT_CAP_IDE_ADDR_ASSOC_SIZEOF;
    SelectiveIDEStream *default_stream;
    IDESelAlgo sel_algo;
    uint8_t tc;

    if (!range_covers_byte(offset, len, addr)) {
        return;
    }

    if (range_covers_byte(addr, size, offset + PCI_EXP_SEL_IDE_STREAM_CTRL)) {
        if (pcie_sel_ide_enabled(dev, sel_ide_stream) &&
            pcie_sel_ide_rid_assoc_valid(dev, sel_ide_stream)) {
            pcie_sel_ide_config_sel_algo_writable(dev, sel_ide_stream, false);
        } else {
            pcie_sel_ide_config_sel_algo_writable(dev, sel_ide_stream, true);
            pcie_sel_ide_config_write_state(
                dev, sel_ide_stream, IDE_STREAM_STATE_INSECURE);
        }
    }

    if (range_covers_byte(
        addr, size, offset + PCI_EXP_SEL_IDE_STREAM_CTRL + 1)) {
        sel_algo = pcie_sel_ide_sel_algo(dev, sel_ide_stream);

        if (!pcie_ide_is_algo_supported(dev, sel_algo)) {
            /* Undo write if value invalid */
            pcie_sel_ide_set_sel_algo(
                dev, sel_ide_stream, sel_ide_stream->stream.sel_algo);
        } else {
            sel_ide_stream->stream.sel_algo = sel_algo;
        }
    }

    if (range_covers_byte(
        addr, size, offset + PCI_EXP_SEL_IDE_STREAM_CTRL + 2)) {
        sel_algo = pcie_sel_ide_sel_algo(dev, sel_ide_stream);

        if (!pcie_ide_is_algo_supported(dev, sel_algo)) {
            /* Undo write if value invalid */
            pcie_sel_ide_set_sel_algo(
                dev, sel_ide_stream, sel_ide_stream->stream.sel_algo);
        } else {
            sel_ide_stream->stream.sel_algo = sel_algo;
        }

        tc = pcie_sel_ide_tc(dev, sel_ide_stream);

        if (!pcie_ide_is_tc_supported(dev, tc)) {
            /* Undo write if value invalid */
            pcie_sel_ide_set_tc(
                dev, sel_ide_stream, sel_ide_stream->stream.tc);
        } else {
            sel_ide_stream->stream.tc = tc;
        }
    }

    if (range_covers_byte(
        addr, size, offset + PCI_EXP_SEL_IDE_STREAM_CTRL + 3)) {
        default_stream = dev->ide_cap.default_stream;

        if (pcie_sel_ide_default_stream(dev, sel_ide_stream)) {
            /*
             * If system software sets the Default Stream bit for 2 or more
             * streams (for the same TC), the behaviour is implementation
             * defined. Here, we simply always honour the latest request.
             */
            if (default_stream && sel_ide_stream != default_stream) {
                pcie_sel_ide_config_write_default_stream(
                    dev, default_stream, false);
            }

            default_stream = sel_ide_stream;
        } else if (sel_ide_stream == default_stream) {
            default_stream = NULL;
        }

        dev->ide_cap.default_stream = default_stream;
    }
}

void pcie_ide_config_write(
    PCIDevice *dev, uint32_t addr, uint32_t val, int size)
{
    uint32_t offset = dev->ide_cap.offset + PCI_EXP_IDE_CAP;
    uint16_t len = dev->ide_cap.size - PCI_EXP_IDE_CAP;
    uint8_t stream;
    LinkIDEStream *link_ide_stream;
    SelectiveIDEStream *sel_ide_stream;

    if (!pcie_ide_present(dev) || !range_covers_byte(offset, len, addr)) {
        return;
    }

    for (stream = 0; stream < dev->ide_cap.link_ide_streams_num; ++stream) {
        link_ide_stream = &dev->ide_cap.link_ide_streams[stream];
        pcie_link_ide_reg_blk_write(dev, link_ide_stream, addr, size, val);
    }

    for (stream = 0; stream < dev->ide_cap.link_ide_streams_num; ++stream) {
        sel_ide_stream = &dev->ide_cap.sel_ide_streams[stream];
        pcie_sel_ide_reg_blk_write(dev, sel_ide_stream, addr, size, val);
    }
}

static void pcie_link_ide_stream_init(PCIDevice *dev, LinkIDEStream *stream)
{
    uint32_t reg = 0;
    reg = FIELD_DP32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, EN, true);
    reg = FIELD_DP32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, SEL_ALGO, 0x1f);
    reg = FIELD_DP32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, TC, 0x07);
    reg = FIELD_DP32(reg, PCI_LNK_IDE_STREAM_CTRL_REG, STREAM_ID, 0xff);
    pci_set_long(
        dev->wmask + stream->offset + PCI_EXP_LNK_IDE_STREAM_CTRL, reg);

    reg = FIELD_DP32(
        0, PCI_LNK_IDE_STREAM_STATUS_REG, RECV_INT_CHCK_FAIL_MSG, true);
    pci_set_long(
        dev->w1cmask + stream->offset + PCI_EXP_LNK_IDE_STREAM_STATUS, reg);
}

static void pcie_ide_addr_assoc_blk_init(PCIDevice *dev, IDEAddrAssocBlock *blk)
{
    uint32_t reg = 0;
    reg = FIELD_DP32(reg, PCI_IDE_ADDR_ASSOC_1_REG, V, true);
    reg = FIELD_DP32(reg, PCI_IDE_ADDR_ASSOC_1_REG, MEM_BASE_LO, 0x0fff);
    reg = FIELD_DP32(reg, PCI_IDE_ADDR_ASSOC_1_REG, MEM_LIMIT_LO, 0x0fff);
    pci_set_long(dev->wmask + blk->offset + PCI_EXP_IDE_ADDR_ASSOC_1, reg);

    reg = FIELD_DP32(0, PCI_IDE_ADDR_ASSOC_2_REG, MEM_LIMIT_UP, 0xffffffff);
    pci_set_long(dev->wmask + blk->offset + PCI_EXP_IDE_ADDR_ASSOC_2, reg);

    reg = FIELD_DP32(0, PCI_IDE_ADDR_ASSOC_3_REG, MEM_BASE_UP, 0xffffffff);
    pci_set_long(dev->wmask + blk->offset + PCI_EXP_IDE_ADDR_ASSOC_2, reg);
}

static void pcie_sel_ide_stream_init(PCIDevice *dev, SelectiveIDEStream *stream)
{
    uint8_t blk;
    uint32_t reg = 0;
    reg = FIELD_DP32(
        reg, PCI_SEL_IDE_STREAM_CAP_REG, NUM_ADDR_ASSOC_REG_BLK,
        stream->ide_addr_assoc_blks_num);
    pci_set_long(
        dev->config + stream->offset + PCI_EXP_SEL_IDE_STREAM_CAP, reg);

    reg = 0;
    reg = FIELD_DP32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, EN, true);
    reg = FIELD_DP32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, SEL_ALGO, 0x1f);
    reg = FIELD_DP32(
        reg, PCI_SEL_IDE_STREAM_CTRL_REG, DEFAULT_STREAM,
        pci_is_header_type0(dev));
    reg = FIELD_DP32(reg, PCI_SEL_IDE_STREAM_CTRL_REG, STREAM_ID, 0xff);
    pci_set_long(
        dev->wmask + stream->offset + PCI_EXP_SEL_IDE_STREAM_CTRL, reg);

    reg = FIELD_DP32(
        0, PCI_SEL_IDE_STREAM_STATUS_REG, RECV_INT_CHCK_FAIL_MSG, true);
    pci_set_long(
        dev->w1cmask + stream->offset + PCI_EXP_SEL_IDE_STREAM_STATUS, reg);

    reg = FIELD_DP32(0, PCI_IDE_RID_ASSOC_1_REG, RID_LIMIT, 0xffff);
    pci_set_long(
        dev->wmask + stream->offset + PCI_EXP_IDE_RID_ASSOC_1, reg);

    reg = 0;
    reg = FIELD_DP32(reg, PCI_IDE_RID_ASSOC_2_REG, V, true);
    reg = FIELD_DP32(reg, PCI_IDE_RID_ASSOC_2_REG, RID_BASE, 0xffff);
    pci_set_long(
        dev->wmask + stream->offset + PCI_EXP_IDE_RID_ASSOC_2, reg);

    for (blk = 0; blk < stream->ide_addr_assoc_blks_num; ++blk) {
        pcie_ide_addr_assoc_blk_init(dev, &stream->ide_addr_assoc_blks[blk]);
    }
}

void pcie_ide_init(
    PCIDevice *dev, uint16_t offset, bool ide_km_supported,
    LinkIDEStream *link_ide_streams, uint8_t link_ide_streams_num,
    SelectiveIDEStream *sel_ide_streams, uint8_t sel_ide_streams_num)
{
    uint16_t pci_ext_cap_ide_sizeof = PCI_EXT_CAP_IDE_SIZEOF;
    uint32_t reg;
    uint8_t stream, blk;
    SelectiveIDEStream *sel_ide_stream;

    /* Always controlled from Function 0. Must not be implemented in VF! */
    assert(!pci_is_vf(dev));

    for (stream = 0; stream < link_ide_streams_num; ++stream) {
        link_ide_streams[stream].offset = pci_ext_cap_ide_sizeof;
        pci_ext_cap_ide_sizeof += PCI_EXT_CAP_LNK_IDE_SIZEOF;
    }

    for (stream = 0; stream < sel_ide_streams_num; ++stream) {
        sel_ide_stream = &sel_ide_streams[stream];
        sel_ide_stream->offset = pci_ext_cap_ide_sizeof;
        pci_ext_cap_ide_sizeof += PCI_EXT_CAP_SEL_IDE_SIZEOF;

        for (blk = 0; blk < sel_ide_stream->ide_addr_assoc_blks_num; ++blk) {
            sel_ide_stream->ide_addr_assoc_blks[blk].offset =
                pci_ext_cap_ide_sizeof;
            pci_ext_cap_ide_sizeof += PCI_EXT_CAP_IDE_ADDR_ASSOC_SIZEOF;
        }
    }

    pcie_add_capability(
        dev, PCI_EXT_CAP_ID_IDE, 0x1, offset, pci_ext_cap_ide_sizeof);
    dev->ide_cap = (IDECap) {
        .offset = offset,
        .size = pci_ext_cap_ide_sizeof,
        .link_ide_streams_num = link_ide_streams_num,
        .link_ide_streams = link_ide_streams,
        .sel_ide_streams_num = sel_ide_streams_num,
        .sel_ide_streams = sel_ide_streams,
        .port_index = 0,
        .default_stream = NULL,
    };

    reg = 0;

    if (link_ide_streams_num) {
        reg = FIELD_DP32(reg, PCI_IDE_CAP_REG, LNK_IDE_STREAM_SUPP, true);
        reg = FIELD_DP32(reg, PCI_IDE_CAP_REG, NUM_LNK_IDE_STREAMS_SUPP,
                         link_ide_streams_num - 1);
    }

    if (sel_ide_streams_num) {
        reg = FIELD_DP32(reg, PCI_IDE_CAP_REG, SEL_IDE_STREAM_SUPP, true);
        reg = FIELD_DP32(reg, PCI_IDE_CAP_REG, NUM_SEL_IDE_STREAMS_SUPP,
                         sel_ide_streams_num - 1);
    }

    reg = FIELD_DP32(reg, PCI_IDE_CAP_REG, IDE_KM_SUPP, ide_km_supported);
    reg = FIELD_DP32(reg, PCI_IDE_CAP_REG, FLOW_THRU_SUPP,
                     !pci_is_header_type0(dev));
    pci_set_long(dev->config + offset + PCI_EXP_IDE_CAP, reg);

    reg = FIELD_DP32(0, PCI_IDE_CTRL_REG, FLOW_THRU_EN,
                     !pci_is_header_type0(dev));
    pci_set_long(dev->wmask + offset + PCI_EXP_IDE_CTRL, reg);

    for (stream = 0; stream < link_ide_streams_num; ++stream) {
        pcie_link_ide_stream_init(dev, &link_ide_streams[stream]);
    }

    for (stream = 0; stream < sel_ide_streams_num; ++stream) {
        pcie_sel_ide_stream_init(dev, &sel_ide_streams[stream]);
    }
}

static bool pcie_ide_km_get_response_query(
    PCIDevice *dev, uint32_t session_id, const IDEKMMessage *request,
    size_t request_size, IDEKMMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    IDEKMQuery *query;
    IDEKMQueryResp *query_resp;
    size_t size = sizeof(IDEKMQueryResp) + dev->ide_cap.size - PCI_EXP_IDE_CAP;

    assert(response && response_size);

    if (request_size < sizeof(IDEKMQuery)) {
        return false;
    }

    if (*response_size < size) {
        return false;
    }

    query = (IDEKMQuery *)request;
    query_resp = (IDEKMQueryResp *)response;

    if (query->port_index != dev->ide_cap.port_index) {
        return false;
    }

    if (!dev->ide_cap.session_bound) {
        dev->ide_cap.session_id = session_id;
        dev->ide_cap.session_bound = true;
    } else if (session_id != dev->ide_cap.session_id) {
        return false;
    }

    query_resp->common.object_id = PCI_IDE_KM_OBJECT_ID_QUERY_RESP;
    query_resp->port_index = dev->ide_cap.port_index;
    query_resp->dev_fn_num = dev->devfn;
    query_resp->bus_num = pci_dev_bus_num(dev);
    query_resp->segment = 0; /* Root port segments not implemented */
    query_resp->max_port_index = dev->ide_cap.port_index;
    memcpy(
        query_resp + 1, dev->config + dev->ide_cap.offset + PCI_EXP_IDE_CAP,
        dev->ide_cap.size - PCI_EXP_IDE_CAP);
    *response_size = size;
    return true;
}

static bool pcie_ide_km_get_response_key_prog(
    PCIDevice *dev, uint32_t session_id, const IDEKMMessage *request,
    size_t request_size, IDEKMMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    IDEKMKeyProg *key_prog;
    IDEKMKpAck *kp_ack;
    IDEStream *stream;
    /*
     * size_t key_size, iv_size;
     * IDESubStreamID sub_stream;
     * uint8_t rxtxb, key_set;
     */

    assert(response && response_size);

    if (request_size < sizeof(IDEKMKeyProg)) {
        return false;
    }

    if (*response_size < sizeof(IDEKMKpAck)) {
        return false;
    }

    key_prog = (IDEKMKeyProg *)request;
    kp_ack = (IDEKMKpAck *)response;

    if (key_prog->port_index != dev->ide_cap.port_index) {
        return false;
    }

    if (!dev->ide_cap.session_bound) {
        dev->ide_cap.session_id = session_id;
        dev->ide_cap.session_bound = true;
    } else if (session_id != dev->ide_cap.session_id) {
        return false;
    }

    stream = pcie_ide_stream_find_by_stream_id(dev, key_prog->stream_id);

    if (!stream) {
        return false;
    }

    /*
     * sub_stream = ide_km_sub_stream(
     *     key_prog->attributes, pcie_tee_io_supported(dev));
     * rxtxb = ide_km_rxtxb(key_prog->attributes);
     * key_set = ide_km_key_set(key_prog->attributes);
     * key_size = pcie_ide_km_get_key_size(stream->sel_algo);
     * iv_size = PCI_IDE_KM_KEY_PROG_IV_SIZE;
     */

    kp_ack->common.object_id = PCI_IDE_KM_OBJECT_ID_KP_ACK;
    kp_ack->stream_id = key_prog->stream_id;
    kp_ack->attributes = key_prog->attributes;
    kp_ack->port_index = key_prog->port_index;
    *response_size = sizeof(IDEKMKpAck);
    return true;
}

static bool pcie_ide_km_get_response_key_set_go(
    PCIDevice *dev, uint32_t session_id, const IDEKMMessage *request,
    size_t request_size, IDEKMMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    IDEKMKSetGo *k_set_go;
    IDEKMKGostopAck *k_gostop_ack;
    IDEStream *stream;

    assert(response && response_size);

    if (request_size < sizeof(IDEKMKSetGo)) {
        return false;
    }

    if (*response_size < sizeof(IDEKMKGostopAck)) {
        return false;
    }

    k_set_go = (IDEKMKSetGo *)request;
    k_gostop_ack = (IDEKMKGostopAck *)response;

    if (k_set_go->port_index != dev->ide_cap.port_index) {
        return false;
    }

    if (!dev->ide_cap.session_bound || session_id != dev->ide_cap.session_id) {
        return false;
    }

    stream = pcie_ide_stream_find_by_stream_id(dev, k_set_go->stream_id);

    if (!stream) {
        return false;
    }

    k_gostop_ack->common.object_id = PCI_IDE_KM_OBJECT_ID_K_GOSTOP_ACK;
    k_gostop_ack->stream_id = k_set_go->stream_id;
    k_gostop_ack->attributes = k_set_go->attributes;
    k_gostop_ack->port_index = k_set_go->port_index;
    *response_size = sizeof(IDEKMKGostopAck);
    return true;
}

static bool pcie_ide_km_get_response_key_set_stop(
    PCIDevice *dev, uint32_t session_id, const IDEKMMessage *request,
    size_t request_size, IDEKMMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    IDEKMKSetStop *k_set_stop;
    IDEKMKGostopAck *k_gostop_ack;
    IDEStream *stream;

    assert(response && response_size);

    if (request_size < sizeof(IDEKMKSetStop)) {
        return false;
    }

    if (*response_size < sizeof(IDEKMKGostopAck)) {
        return false;
    }

    k_set_stop = (IDEKMKSetStop *)request;
    k_gostop_ack = (IDEKMKGostopAck *)response;

    if (k_set_stop->port_index != dev->ide_cap.port_index) {
        return false;
    }

    if (!dev->ide_cap.session_bound || session_id != dev->ide_cap.session_id) {
        return false;
    }

    stream = pcie_ide_stream_find_by_stream_id(dev, k_set_stop->stream_id);

    if (!stream) {
        return false;
    }

    k_gostop_ack->common.object_id = PCI_IDE_KM_OBJECT_ID_K_GOSTOP_ACK;
    k_gostop_ack->stream_id = k_set_stop->stream_id;
    k_gostop_ack->attributes = k_set_stop->attributes;
    k_gostop_ack->port_index = k_set_stop->port_index;
    *response_size = sizeof(IDEKMKGostopAck);
    return true;
}

bool pcie_ide_km_get_response(
    PCIDevice *dev, uint32_t session_id, const PCIPayload *request,
    size_t request_size, PCIPayload *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    IDEKMMessage *request_message, *response_message;
    bool success;

    assert(request && response && request_size >= sizeof(PCIPayload) &&
        response_size && request->protocol_id == PCI_SPDM_PROTOCOL_ID_IDE_KM &&
        error_code);

    if (request_size < sizeof(IDEKMMessage)) {
        return false;
    }

    if (*response_size < sizeof(IDEKMMessage)) {
        return false;
    }

    request_message = (IDEKMMessage *)request;
    response_message = (IDEKMMessage *)response;

    switch (request_message->object_id) {
    case PCI_IDE_KM_OBJECT_ID_QUERY:
        success = pcie_ide_km_get_response_query(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    case PCI_IDE_KM_OBJECT_ID_KEY_PROG:
        success = pcie_ide_km_get_response_key_prog(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    case PCI_IDE_KM_OBJECT_ID_K_SET_GO:
        success = pcie_ide_km_get_response_key_set_go(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    case PCI_IDE_KM_OBJECT_ID_K_SET_STOP:
        success = pcie_ide_km_get_response_key_set_stop(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    default:
        *error_code = SPDM_ERROR_CODE_INVALID_REQUEST;
        success = false;
        break;
    }

    response_message->payload.protocol_id = PCI_SPDM_PROTOCOL_ID_IDE_KM;
    return success;
}
