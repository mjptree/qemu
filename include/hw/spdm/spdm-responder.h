#ifndef HW_SPDM_SPDM_RESPONDER_H
#define HW_SPDM_SPDM_RESPONDER_H

#include "qom/object_interfaces.h"
#include "hw/register.h"

#define TYPE_SPDM_RESPONDER "spdm-responder"
OBJECT_DECLARE_TYPE(SPDMResponder, SPDMResponderClass, SPDM_RESPONDER)

#define SPDM_STANDARD_ID_PCISIG 0x3
#define SPDM_VENDOR_ID_PCISIG   0x1

typedef enum SPDMErrorCode {
    SPDM_ERROR_CODE_INVALID_REQUEST         = 0x01,
    SPDM_ERROR_CODE_BUSY                    = 0x03,
    SPDM_ERROR_CODE_UNEXPECTED_REQUEST      = 0x04,
    SPDM_ERROR_CODE_UNSPECIFIED             = 0x05,
    SPDM_ERROR_CODE_DECRYPT_ERROR           = 0x06,
    SPDM_ERROR_CODE_UNSUPPORTED_REQUEST     = 0x07,
    SPDM_ERROR_CODE_REQUEST_IN_FLIGHT       = 0x08,
    SPDM_ERROR_CODE_INVALID_RESPPONSE_CODE  = 0x09,
    SPDM_ERROR_CODE_SESSION_LIMIT_EXCEEDED  = 0x0a,
    SPDM_ERROR_CODE_SESSION_REQUIRED        = 0x0b,
    SPDM_ERROR_CODE_RESET_REQUIRED          = 0x0c,
    SPDM_ERROR_CODE_RESPONSE_TOO_LARGE      = 0x0d,
    SPDM_ERROR_CODE_REQUEST_TOO_LARGE       = 0x0e,
    SPDM_ERROR_CODE_LARGE_RESPONSE          = 0x0f,
    SPDM_ERROR_CODE_MESSAGE_LOST            = 0x10,
    SPDM_ERROR_CODE_INVALID_POLICY          = 0x11,
    SPDM_ERROR_CODE_VERSION_MISMATCH        = 0x41,
    SPDM_ERROR_CODE_RESPONSE_NOT_READY      = 0x42,
    SPDM_ERROR_CODE_REQUEST_RESYNCH         = 0x43,
    SPDM_ERROR_CODE_OPERATION_FAILED        = 0x44,
    SPDM_ERROR_CODE_NO_PENDING_REQUESTS     = 0x45,
    SPDM_ERROR_CODE_VENDOR_DEFINED          = 0xff,
} SPDMErrorCode;

#pragma pack(1)

#define SPDM_REQUEST_CODE_VENDOR_DEFINED_REQUEST    0xfe
#define SPDM_RESPONSE_CODE_VENDOR_DEFINED_RESPONSE  0x7e

typedef struct SPDMHeader {
    uint8_t spdm_version;
    uint8_t request_response_code;
    uint8_t param1;
    uint8_t param2;
} SPDMHeader;

typedef struct SPDMVendorDefined {
    SPDMHeader header;
    uint16_t standard_id;
    uint8_t len;
} SPDMVendorDefined;

typedef struct SPDMPCIDefined {
    SPDMVendorDefined vendor_defined;
    uint16_t vendor_id;
    uint16_t req_length;
} SPDMPCIDefined;

/* PCI-SIG defined CMA/SPDM Protocol IDs */
#define PCI_SPDM_PROTOCOL_ID_IDE_KM         0x00
#define PCI_SPDM_PROTOCOL_ID_TDISP          0x01

typedef struct PCIPayload {
    uint8_t protocol_id;
} PCIPayload;

/* PCI-SIG defined IDE_KM Object IDs */
#define PCI_IDE_KM_OBJECT_ID_QUERY          0x00
#define PCI_IDE_KM_OBJECT_ID_QUERY_RESP     0x01
#define PCI_IDE_KM_OBJECT_ID_KEY_PROG       0x02
#define PCI_IDE_KM_OBJECT_ID_KP_ACK         0x03
#define PCI_IDE_KM_OBJECT_ID_K_SET_GO       0x04
#define PCI_IDE_KM_OBJECT_ID_K_SET_STOP     0x05
#define PCI_IDE_KM_OBJECT_ID_K_GOSTOP_ACK   0x06

typedef struct IDEKMMessage {
    PCIPayload payload;
    uint8_t object_id;
} IDEKMMessage;

typedef struct IDEKMQuery {
    IDEKMMessage common;
    uint8_t reserved;
    uint8_t port_index;
} IDEKMQuery;

typedef struct IDEKMQueryResp {
    IDEKMMessage common;
    uint8_t reserved;
    uint8_t port_index;
    uint8_t dev_fn_num;
    uint8_t bus_num;
    uint8_t segment;
    uint8_t max_port_index;
} IDEKMQueryResp;

/* Size of KeyProg IV */
#define PCI_IDE_KM_KEY_PROG_IV_SIZE     8

/* Size of AES-GCM256 Key */
#define PCI_IDE_KM_AES_GCM256_KEY_SIZE  32

#define ide_km_key_set(attr)    extract8(attr, 0, 1)
#define ide_km_rxtxb(attr)      extract8(attr, 1, 1)
/*
 * In IDE ECN Rev A (PCIe Base Rev 5.0) the sub stream field can be 4 bit long.
 * This has been limited to 3 bits in TDISP ECN (PCIe Base Rev 5.0/6.0).
 */
#define ide_km_sub_stream(attr, tee_io_supported) \
    extract8(attr, 4, tee_io_supported ? 3 : 4)

typedef struct IDEKMKeyProg {
    IDEKMMessage common;
    uint16_t reserved0;
    uint8_t stream_id;
    uint8_t reserved1;
    uint8_t attributes;
    uint8_t port_index;
} IDEKMKeyProg;

typedef struct IDEKMKpAck {
    IDEKMMessage common;
    uint16_t reserved;
    uint8_t stream_id;
    uint8_t status;
    uint8_t attributes;
    uint8_t port_index;
} IDEKMKpAck;

typedef struct IDEKMKSetGo {
    IDEKMMessage common;
    uint16_t reserved0;
    uint8_t stream_id;
    uint8_t reserved1;
    uint8_t attributes;
    uint8_t port_index;
} IDEKMKSetGo;

typedef struct IDEKMKSetStop {
    IDEKMMessage common;
    uint16_t reserved0;
    uint8_t stream_id;
    uint8_t reserved1;
    uint8_t attributes;
    uint8_t port_index;
} IDEKMKSetStop;

typedef struct IDEKMKGostopAck {
    IDEKMMessage common;
    uint16_t reserved0;
    uint8_t stream_id;
    uint8_t reserved1;
    uint8_t attributes;
    uint8_t port_index;
} IDEKMKGostopAck;

/* PCI-SIG defined TDISP Request Codes */
#define PCI_TDISP_REQUEST_CODE_GET_TDISP_VERSION            0x81
#define PCI_TDISP_REQUEST_CODE_GET_TDISP_CAPABILITES        0x82
#define PCI_TDISP_REQUEST_CODE_LOCK_INTERFACE_REQUEST       0x83
#define PCI_TDISP_REQUEST_CODE_GET_DEVICE_INTERFACE_REPORT  0x84
#define PCI_TDISP_REQUEST_CODE_GET_DEVICE_INTERFACE_STATE   0x85
#define PCI_TDSIP_REQUEST_CODE_START_INTERFACE_REQUEST      0x86
#define PCI_TDISP_REQUEST_CODE_STOP_INTERFACE_REQUEST       0x87
#define PCI_TDISP_REQUEST_CODE_BIND_P2P_STREAM_REQUEST      0x88
#define PCI_TDISP_REQUEST_CODE_UNBIND_P2P_STREAM_REQUEST    0x89
#define PCI_TDISP_REQUEST_CODE_SET_MMIO_ATTRIBUTE_REQUEST   0x8a
#define PCI_TDISP_REQUEST_CODE_VDM_REQUEST                  0x8b

/* PCI-SIG defined TDISP Response Codes*/
#define PCI_TDISP_RESPONSE_CODE_TDISP_VERSION               0x01
#define PCI_TDISP_RESPONSE_CODE_TDISP_CAPABILITIES          0x02
#define PCI_TDISP_RESPONSE_CODE_LOCK_INTERFACE_RESPONSE     0x03
#define PCI_TDISP_RESPONSE_CODE_DEVICE_INTERFACE_REPORT     0x04
#define PCI_TDISP_RESPONSE_CODE_DEVICE_INTERFACE_STATE      0x05
#define PCI_TDISP_RESPONSE_CODE_START_INTERFACE_RESPONSE    0x06
#define PCI_TDISP_RESPONSE_CODE_STOP_INTERFACE_RESPONSE     0x07
#define PCI_TDISP_RESPONSE_CODE_BIND_P2P_STREAM_RESPONSE    0x08
#define PCI_TDISP_RESPONSE_CODE_UNBIND_P2P_STREAM_RESPONSE  0x09
#define PCI_TDISP_RESPONSE_CODE_SET_MMIO_ATTRIBUTE_RESPONSE 0x0a
#define PCI_TDISP_RESPONSE_CODE_VDM_RESPONSE                0x0b
#define PCI_TDISP_RESPONSE_TDISP_ERROR                      0x7f

typedef struct TDISPInterfaceID {
    uint32_t function_id;
    uint32_t reserved[2];
} TDISPInterfaceID;

typedef struct TDISPMessage {
    PCIPayload payload;
    uint8_t tdisp_version;
    uint8_t message_type;
    uint16_t reserved;
    TDISPInterfaceID interface_id;
} TDISPMessage;

typedef struct TDISPGetTDISPVersion {
    TDISPMessage common;
} TDISPGetTDISPVersion;

typedef struct TDISPVersion {
    TDISPMessage common;
    uint8_t version_num_count;
} TDISPTDISPVersion;

typedef struct TDISPGetTDSIPCapabilities {
    TDISPMessage common;
    uint32_t tsm_caps;
} TDISPGetTDSIPCapabilities;

typedef struct TDISPCapabilities {
    TDISPMessage common;
    uint32_t dsm_caps;
    uint32_t req_msgs_supported[4];
    uint16_t lock_interface_flags_supported;
    uint8_t reserved[3];
    uint8_t dev_addr_width;
    uint8_t num_req_this;
    uint8_t num_req_all;
} TDISPTDISPCapabilities;

typedef struct TDISPLockInterfaceRequest {
    TDISPMessage common;
    uint16_t flags;
    uint8_t stream_id_for_default_stream;
    uint8_t reserved;
    uint64_t mmio_reporting_offset;
    uint64_t bind_p2p_address_mask;
} TDISPLockInterfaceRequest;

typedef struct TDISPLockInterfaceResponse {
    TDISPMessage common;
    uint8_t start_interface_nonce[32];
} TDISPLockInterfaceResponse;

typedef struct TDISPGetDeviceInterfaceReport {
    TDISPMessage common;
    uint16_t offset;
    uint16_t length;
} TDISPGetDeviceInterfaceReport;

typedef struct TDISPDeviceInterfaceReport {
    TDISPMessage common;
    uint16_t portion_length;
    uint16_t remainder_length;
} TDISPDeviceInterfaceReport;

typedef struct TDISPGetDeviceInterfaceState {
    TDISPMessage common;
} TDISPGetDeviceInterfaceState;

/* PCI-SIG defined TDISP TDI states */
#define PCI_TDISP_TDI_STATE_CONFIG_UNLOCKED 0x00
#define PCI_TDISP_TDI_STATE_CONFIG_LOCKED   0x01
#define PCI_TDISP_TDI_STATE_RUN             0x02
#define PCI_TDISP_TDI_STATE_ERROR           0x03

typedef struct TDISPDeviceInterfaceState {
    TDISPMessage common;
    uint8_t tdi_state;
} TDISPDeviceInterfaceState;

typedef struct TDISPStartInterfaceRequest {
    TDISPMessage common;
    uint8_t start_interface_nonce[32];
} TDISPStartInterfaceRequest;

typedef struct TDISPStartInterfaceResponse {
    TDISPMessage common;
} TDISPStartInterfaceResponse;

typedef struct TDISPStopInterfaceRequest {
    TDISPMessage common;
} TDISPStopInterfaceRequest;

typedef struct TDISPStopInterfaceResponse {
    TDISPMessage common;
} TDISPStopInterfaceResponse;

typedef struct TDISPBindP2PStreamRequest {
    TDISPMessage common;
    uint8_t p2p_stream_id;
} TDISPBindP2PStreamRequest;

typedef struct TDISPBindP2PStreamResponse {
    TDISPMessage common;
} TDISPBindP2PStreamResponse;

typedef struct TDISPUnbindP2PStreamRequest {
    TDISPMessage common;
    uint8_t p2p_stream_id;
} TDISPUnbindP2PStreamRequest;

typedef struct TDISPUnbindP2PStreamResponse {
    TDISPMessage common;
} TDISPUnbindP2PStreamResponse;

typedef struct TDISPSetMMIOAttributeRequest {
    TDISPMessage common;
    struct {
        uint64_t first_page;
        uint32_t num_pages;
        uint32_t range_attributes;
    } mmio_range;
} TDISPSetMMIOAttributeRequest;

typedef struct TDISPSetMMIOAttributeResponse {
    TDISPMessage common;
} TDISPSetMMIOAttributeResponse;

/* PCI-SIG defined TDISP error codes */
#define TDISP_ERROR_CODE_INVALID_REQUEST                0x0001
#define TDISP_ERROR_CODE_BUSY                           0x0003
#define TDISP_ERROR_CODE_INVALID_INTERFACE_STATE        0x0004
#define TDISP_ERROR_CODE_UNSPECIFIED                    0x0005
#define TDISP_ERROR_CODE_UNSUPPORTED_REQUEST            0x0007
#define TDISP_ERROR_CODE_VERSION_MISMATCH               0x0041
#define TDISP_ERROR_CODE_VENDOR_SPECIFIC_ERROR          0x00ff
#define TDISP_ERROR_CODE_INVALID_INTERFACE              0x0101
#define TDISP_ERROR_CODE_INVALID_NONCE                  0x0102
#define TDISP_ERROR_CODE_INSUFFICIENT_ENTROPY           0x0103
#define TDISP_ERROR_CODE_INVALID_DEVICE_CONFIGURATION   0x0104

typedef struct TDISPError {
    TDISPMessage common;
    uint32_t error_code;
    uint32_t error_data;
} TDISPError;

typedef struct TDISPVDMRequest {
    TDISPMessage common;
    uint8_t registry_id;
    uint8_t vendor_id_len;
} TDISPVDMRequest;

typedef struct TDISPVDMResponse {
    TDISPMessage common;
    uint8_t registry_id;
    uint8_t vendor_id_len;
} TDISPVDMResponse;

#pragma pack()

typedef bool SPDMResponderSendMessageFunc(
    DeviceState *dev, size_t message_size, const void *message);
typedef bool SPDMResponderReceiveMessageFunc(
    DeviceState *dev, size_t *message_size, void **message);
typedef bool SPDMResponderGetResponseFunc(
    DeviceState *dev, const uint32_t *session_id, size_t request_size,
    const SPDMHeader *request, size_t *response_size, SPDMHeader *response);

struct SPDMResponderClass {
    /*< private >*/
    ObjectClass parent_class;
    /*< public >*/

    bool (*device_init)(
        SPDMResponder *responder, DeviceState *dev,
        SPDMResponderSendMessageFunc send_message,
        SPDMResponderReceiveMessageFunc receive_message,
        SPDMResponderGetResponseFunc get_response, Error **errp);
    bool (*dispatch_message)(SPDMResponder *responder, Error **errp);
    uint8_t (*get_connection_version)(SPDMResponder *responder);
};

struct SPDMResponder {
    /*< private >*/
    Object parent_obj;
    /*< public >*/
};

bool device_spdm_responder_init(
    DeviceState *dev, SPDMResponder *responder,
    SPDMResponderSendMessageFunc send_message,
    SPDMResponderReceiveMessageFunc receive_message,
    SPDMResponderGetResponseFunc get_response, Error **errp);
bool spdm_responder_dispatch_message(SPDMResponder *responder, Error **errp);
uint8_t spdm_responder_get_connection_version(SPDMResponder *responder);

#endif /* HW_SPDM_SPDM_RESPONDER_H */
