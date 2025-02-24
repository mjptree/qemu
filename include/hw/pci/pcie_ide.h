#ifndef QEMU_PCIE_IDE_H
#define QEMU_PCIE_IDE_H

#include "qemu/osdep.h"
#include "hw/spdm/spdm-responder.h"
#include "hw/register.h"

/* Integrity and Data Encryption Extended Capability */
#define PCI_EXT_CAP_ID_IDE  0x30

/* Size of IDE Extended Capability w/o Link IDE & Selective IDE Registers */
#define PCI_EXT_CAP_IDE_SIZEOF              0x0c
/* Size of Link IDE Register Block */
#define PCI_EXT_CAP_LNK_IDE_SIZEOF          0x08
/* Size of Selective IDE Register Block w/o Address Association Registers */
#define PCI_EXT_CAP_SEL_IDE_SIZEOF          0x14
/* Size of Address Association Register Block */
#define PCI_EXT_CAP_IDE_ADDR_ASSOC_SIZEOF   0x0c

typedef enum IDEStreamState {
    IDE_STREAM_STATE_INSECURE = 0b0000,
    IDE_STREAM_STATE_SECURE   = 0b0010,
    IDE_STREAM_STATE_MASK     = 0b1111,
} IDEStreamState;

typedef enum IDESelAlgo {
    IDE_SEL_ALGO_AES_GCM256 = 0b00000,
    IDE_SEL_ALGO_MASK       = 0b11111,
} IDESelAlgo;

typedef enum IDESubStreamID {
    IDE_SUB_STREAM_ID_PR   = 0b000,
    IDE_SUB_STREAM_ID_NPR  = 0b001,
    IDE_SUB_STREAM_ID_C    = 0b010,
    IDE_SUB_STREAM_ID_MASK = 0b111,
} IDESubStreamID;

#define IDE_SUB_STREAM_MAX_COUNT 3

/* IDE Capabilites Register */
#define PCI_EXP_IDE_CAP     0x04
REG32(PCI_IDE_CAP_REG, PCI_EXP_IDE_CAP)
    FIELD(PCI_IDE_CAP_REG, LNK_IDE_STREAM_SUPP, 0, 1)
    FIELD(PCI_IDE_CAP_REG, SEL_IDE_STREAM_SUPP, 1, 1)
    FIELD(PCI_IDE_CAP_REG, FLOW_THRU_SUPP, 2, 1)
    FIELD(PCI_IDE_CAP_REG, AGGR_SUPP, 4, 1)
    FIELD(PCI_IDE_CAP_REG, PCRC_SUPP, 5, 1)
    FIELD(PCI_IDE_CAP_REG, IDE_KM_SUPP, 6, 1)
    FIELD(PCI_IDE_CAP_REG, SEL_IDE_CONFIG_REQ_SUPP, 7, 1)
    FIELD(PCI_IDE_CAP_REG, SUPP_ALGO, 8, 5)
    FIELD(PCI_IDE_CAP_REG, NUM_LNK_IDE_STREAMS_SUPP, 13, 3)
    FIELD(PCI_IDE_CAP_REG, NUM_SEL_IDE_STREAMS_SUPP, 16, 8)

/* IDE Control Register */
#define PCI_EXP_IDE_CTRL    0x08
REG32(PCI_IDE_CTRL_REG, PCI_EXP_IDE_CTRL)
    FIELD(PCI_IDE_CTRL_REG, FLOW_THRU_EN, 2, 1)

/* Link IDE Stream Control Register */
#define PCI_EXP_LNK_IDE_STREAM_CTRL 0x00
REG32(PCI_LNK_IDE_STREAM_CTRL_REG, PCI_EXP_LNK_IDE_STREAM_CTRL)
    FIELD(PCI_LNK_IDE_STREAM_CTRL_REG, EN, 0, 1)
    FIELD(PCI_LNK_IDE_STREAM_CTRL_REG, TX_AGGR_MODE_NPR, 2, 2)
    FIELD(PCI_LNK_IDE_STREAM_CTRL_REG, TX_AGGR_MODE_PR, 4, 2)
    FIELD(PCI_LNK_IDE_STREAM_CTRL_REG, TX_AGGR_MODE_CPL, 6, 2)
    FIELD(PCI_LNK_IDE_STREAM_CTRL_REG, PCRC_EN, 8, 1)
    FIELD(PCI_LNK_IDE_STREAM_CTRL_REG, SEL_ALGO, 14, 5)
    FIELD(PCI_LNK_IDE_STREAM_CTRL_REG, TC, 19, 3)
    FIELD(PCI_LNK_IDE_STREAM_CTRL_REG, STREAM_ID, 24, 8)

/* Link IDE Stream Status Register */
#define PCI_EXP_LNK_IDE_STREAM_STATUS   0x04
REG32(PCI_LNK_IDE_STREAM_STATUS_REG, PCI_EXP_LNK_IDE_STREAM_STATUS)
    FIELD(PCI_LNK_IDE_STREAM_STATUS_REG, STATE, 0, 4)
    FIELD(PCI_LNK_IDE_STREAM_STATUS_REG, RECV_INT_CHCK_FAIL_MSG, 31, 1)

/* Selective IDE Stream Capability Register */
#define PCI_EXP_SEL_IDE_STREAM_CAP  0x00
REG32(PCI_SEL_IDE_STREAM_CAP_REG, PCI_EXP_SEL_IDE_STREAM_CAP)
    FIELD(PCI_SEL_IDE_STREAM_CAP_REG, NUM_ADDR_ASSOC_REG_BLK, 0, 4)

/* Selective IDE Stream Control Register */
#define PCI_EXP_SEL_IDE_STREAM_CTRL 0x04
REG32(PCI_SEL_IDE_STREAM_CTRL_REG, PCI_EXP_SEL_IDE_STREAM_CTRL)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, EN, 0, 1)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, TX_AGGR_MODE_NPR, 2, 2)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, TX_AGGR_MODE_PR, 4, 2)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, TX_AGGR_MODE_CPL, 6, 2)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, PCRC_EN, 8, 1)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, SEL_IDE_CFG_REQ_EN, 9, 1)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, SEL_ALGO, 14, 5)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, TC, 19, 3)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, DEFAULT_STREAM, 22, 1)
    FIELD(PCI_SEL_IDE_STREAM_CTRL_REG, STREAM_ID, 24, 8)

/* Selective IDE Stream Status Register */
#define PCI_EXP_SEL_IDE_STREAM_STATUS   0x08
REG32(PCI_SEL_IDE_STREAM_STATUS_REG, PCI_EXP_SEL_IDE_STREAM_STATUS)
    FIELD(PCI_SEL_IDE_STREAM_STATUS_REG, STATE, 0, 4)
    FIELD(PCI_SEL_IDE_STREAM_STATUS_REG, RECV_INT_CHCK_FAIL_MSG, 31, 1)

/* IDE RID Association Register 1 */
#define PCI_EXP_IDE_RID_ASSOC_1 0x0c
REG32(PCI_IDE_RID_ASSOC_1_REG, PCI_EXP_IDE_RID_ASSOC_1)
    FIELD(PCI_IDE_RID_ASSOC_1_REG, RID_LIMIT, 8, 16)

/* IDE RID Association Register 2 */
#define PCI_EXP_IDE_RID_ASSOC_2 0x10
REG32(PCI_IDE_RID_ASSOC_2_REG, PCI_EXP_IDE_RID_ASSOC_1)
    FIELD(PCI_IDE_RID_ASSOC_2_REG, V, 0, 1)
    FIELD(PCI_IDE_RID_ASSOC_2_REG, RID_BASE, 8, 16)

/* IDE Address Association Register 1 */
#define PCI_EXP_IDE_ADDR_ASSOC_1    0x00
REG32(PCI_IDE_ADDR_ASSOC_1_REG, PCI_EXP_IDE_ADDR_ASSOC_1)
    FIELD(PCI_IDE_ADDR_ASSOC_1_REG, V, 0, 1)
    FIELD(PCI_IDE_ADDR_ASSOC_1_REG, MEM_BASE_LO, 8, 12)
    FIELD(PCI_IDE_ADDR_ASSOC_1_REG, MEM_LIMIT_LO, 20, 12)

/* IDE Address Association Register 2 */
#define PCI_EXP_IDE_ADDR_ASSOC_2    0x04
REG32(PCI_IDE_ADDR_ASSOC_2_REG, PCI_EXP_IDE_ADDR_ASSOC_2)
    FIELD(PCI_IDE_ADDR_ASSOC_2_REG, MEM_LIMIT_UP, 0, 32)

/* IDE Address Association Register 3 */
#define PCI_EXP_IDE_ADDR_ASSOC_3    0x08
REG32(PCI_IDE_ADDR_ASSOC_3_REG, PCI_EXP_IDE_ADDR_ASSOC_3)
    FIELD(PCI_IDE_ADDR_ASSOC_3_REG, MEM_BASE_UP, 0, 32)

#define IDE_KEY_SET_MAX_COUNT 2

typedef struct IDESubStream {
    GByteArray *rx_key;
} IDESubStream;

typedef struct IDEKeySet {
    IDESubStream sub_stream[IDE_SUB_STREAM_MAX_COUNT];
} IDEKeySet;

typedef struct IDEStream {
    /* Selected algorithm */
    IDESelAlgo sel_algo;

    /* Traffic class */
    uint8_t tc;

    /* Received K_SET_START message */
    bool started;

    IDEKeySet key_set[IDE_KEY_SET_MAX_COUNT];
} IDEStream;

typedef struct LinkIDEStream {
    IDEStream stream;

    /* Register block offset in configuration space */
    uint16_t offset;
} LinkIDEStream;

typedef struct IDEAddrAssocBlock {
    /* Register block offset in configuration space */
    uint16_t offset;
} IDEAddrAssocBlock;

typedef struct SelectiveIDEStream {
    IDEStream stream;

    /* Register block offset in configuration space */
    uint16_t offset;

    /* Address Association blocks */
    uint8_t ide_addr_assoc_blks_num;
    IDEAddrAssocBlock *ide_addr_assoc_blks;
} SelectiveIDEStream;

typedef struct IDECap {
    /* IDE Extended Capability offset in configuration space */
    uint16_t offset;

    /* Size of the full IDE Extended Capability structure */
    uint16_t size;

    uint8_t link_ide_streams_num;
    LinkIDEStream *link_ide_streams;

    uint8_t sel_ide_streams_num;
    SelectiveIDEStream *sel_ide_streams;

    /* Secure session used to program keys */
    bool session_bound;
    uint32_t session_id;

    /* Currently hardcoded to 0 */
    uint8_t port_index;

    /* Currently only one TC and therefore only one default stream supported */
    SelectiveIDEStream *default_stream;
} IDECap;

bool pcie_ide_present(PCIDevice *dev);
void pcie_ide_init(
    PCIDevice *dev, uint16_t offset, bool ide_km_supported,
    LinkIDEStream *link_ide_streams, uint8_t link_ide_streams_num,
    SelectiveIDEStream *sel_ide_streams, uint8_t sel_ide_streams_num);

void pcie_ide_config_write(
    PCIDevice *dev, uint32_t addr, uint32_t val, int size);

bool pcie_ide_km_get_response(
    PCIDevice *dev, uint32_t session_id, const PCIPayload *request,
    size_t request_size, PCIPayload *response, size_t *response_size,
    SPDMErrorCode *error_code);

#endif /* QEMU_PCIE_IDE_H */
