#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "hw/pci/pci_device.h"
#include "hw/pci/pcie_tdisp.h"

bool pcie_tee_io_supported(PCIDevice *dev)
{
    return pci_get_long(dev->config + dev->exp.exp_cap + PCI_EXP_DEVCAP) &
        PCI_EXP_DEVCAP_TEE_IO;
}

static const uint8_t tdisp_versions_supported[] = {
    TDISP_VERSION(1, 0),
};

static bool pcie_tdisp_version_supported(uint8_t version)
{
    uint8_t index;

    for (index = 0; index < ARRAY_SIZE(tdisp_versions_supported); ++index) {
        if (version == tdisp_versions_supported[index]) {
            return true;
        }
    }

    return false;
}

void pcie_tdisp_init(PCIDevice *dev, bool tee_limited_stream_supported)
{
    SelectiveIDEStream *stream;
    uint32_t reg;
    uint8_t index;

    assert(pcie_ide_km_supported(dev));

    pci_long_test_and_set_mask(
        dev->config + dev->exp.exp_cap + PCI_EXP_DEVCAP,
        PCI_EXP_DEVCAP_TEE_IO);

    if (tee_limited_stream_supported) {
        reg =
            pci_get_long(dev->config + dev->ide_cap.offset + PCI_EXP_IDE_CAP);

        if (FIELD_EX32(reg, PCI_IDE_CAP_REG, SEL_IDE_STREAM_SUPP)) {
            reg = FIELD_DP32(reg, PCI_IDE_CAP_REG, TEE_LTD_STREAM_SUPP, true);
            pci_set_long(
                dev->config + dev->ide_cap.offset + PCI_EXP_IDE_CAP, reg);

            for (index = 0; index < dev->ide_cap.sel_ide_streams_num;
                 ++index) {
                stream = &dev->ide_cap.sel_ide_streams[index];

                if (!pcie_sel_ide_enabled(dev, stream)) {
                    pcie_sel_ide_config_tee_limited_stream_writable(
                        dev, stream, true);
                }
            }
        } else {
            error_report(
                "TEE-limited stream support requires selective IDE stream "
                "support");
        }
    }
}

static bool pcie_tdisp_ide_stream_all_keys_set(
    PCIDevice *dev, const IDEKeySet *key_set)
{
    const IDESubStream *sub_stream;
    uint8_t sub_stream_id;

    for (sub_stream_id = 0; sub_stream_id < IDE_SUB_STREAM_MAX_COUNT;
            ++sub_stream_id) {
        sub_stream = &key_set->sub_streams[sub_stream_id];

        if (!sub_stream->rx_key || !sub_stream->rx_key->len ||
            !sub_stream->rx_iv || !sub_stream->rx_iv->len ||
            !sub_stream->tx_key || !sub_stream->tx_key->len ||
            !sub_stream->tx_iv || !sub_stream->tx_iv->len) {
            return false;
        }
    }

    return true;
}

static bool pcie_tdisp_get_response_tdisp_error(
    PCIDevice *dev, uint8_t version, uint32_t error_code, uint32_t error_data,
    TDISPMessage *response, size_t *response_size)
{
    TDISPError *tdisp_error;
    assert(response && response_size);

    if (*response_size < sizeof(TDISPError)) {
        return false;
    }

    tdisp_error = (TDISPError *)response;
    tdisp_error->common.tdisp_version = version;
    tdisp_error->common.message_type = PCI_TDISP_RESPONSE_CODE_TDISP_ERROR;
    tdisp_error->common.interface_id.function_id =
        TDISP_FUNCTION_ID(pci_get_id(dev), 0, false);
    tdisp_error->error_code = error_code;
    tdisp_error->error_data = error_data;
    *response_size = sizeof(TDISPError);
    return true;
}

static bool pcie_tdisp_get_response_tdisp_version(
    PCIDevice *dev, uint32_t session_id, const TDISPMessage *request,
    size_t request_size, TDISPMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    TDISPGetTDISPVersion *get_tdisp_version;
    TDISPTDISPVersion *tdisp_version;
    size_t versions_size = sizeof(tdisp_versions_supported);

    assert(response && response_size);

    if (!dev->ide_cap.session_bound) {
        dev->ide_cap.session_id = session_id;
        dev->ide_cap.session_bound = true;
    } else if (session_id != dev->ide_cap.session_id) {
        return false;
    }

    if (dev->tdisp_dsm.connection_state !=
        TDISP_CONNECTION_STATE_NOT_STARTED) {
        return false;
    }

    if (*response_size < sizeof(TDISPTDISPVersion) + versions_size) {
        return false;
    }

    if (request_size < sizeof(TDISPGetTDISPVersion)) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_REQUEST, 0,
            response, response_size);
    }

    get_tdisp_version = (TDISPGetTDISPVersion *)request;
    dev->tdisp_dsm.connection_state = TDISP_CONNECTION_STATE_AFTER_VERSION;

    tdisp_version = (TDISPTDISPVersion *)response;
    tdisp_version->common.tdisp_version =
        get_tdisp_version->common.tdisp_version;
    tdisp_version->common.message_type = PCI_TDISP_RESPONSE_CODE_TDISP_VERSION;
    tdisp_version->common.interface_id.function_id =
        TDISP_FUNCTION_ID(pci_get_id(dev), 0, false);
    tdisp_version->version_num_count = ARRAY_SIZE(tdisp_versions_supported);
    memcpy(
        (uint8_t *)tdisp_version + sizeof(TDISPTDISPVersion),
        tdisp_versions_supported, versions_size);
    *response_size = sizeof(TDISPTDISPVersion) + versions_size;
    return true;
}

static bool pcie_tdisp_get_response_tdisp_capabilities(
    PCIDevice *dev, uint32_t session_id, const TDISPMessage *request,
    size_t request_size, TDISPMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    TDISPGetTDISPCapabilities *get_tdisp_capabilities;
    TDISPTDISPCapabilities *tdisp_capabilities;

    assert(response && response_size);

    if (!dev->ide_cap.session_bound || session_id != dev->ide_cap.session_id) {
        return false;
    }

    if (dev->tdisp_dsm.connection_state !=
        TDISP_CONNECTION_STATE_AFTER_VERSION) {
        return false;
    }

    if (*response_size < sizeof(TDISPTDISPCapabilities)) {
        return false;
    }

    if (request_size < sizeof(TDISPGetTDISPCapabilities)) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_REQUEST, 0,
            response, response_size);
    }

    if (!pcie_tdisp_version_supported(request->tdisp_version)) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_VERSION_MISMATCH, 0,
            response, response_size);
    }

    get_tdisp_capabilities = (TDISPGetTDISPCapabilities *)request;
    dev->tdisp_dsm.tdisp_version =
        get_tdisp_capabilities->common.tdisp_version;
    dev->tdisp_dsm.tsm_caps = get_tdisp_capabilities->tsm_caps;
    dev->tdisp_dsm.connection_state =
        TDISP_CONNECTION_STATE_AFTER_CAPABILITIES;

    tdisp_capabilities = (TDISPTDISPCapabilities *)response;
    tdisp_capabilities->common.tdisp_version =
        get_tdisp_capabilities->common.tdisp_version;
    tdisp_capabilities->common.message_type =
        PCI_TDISP_RESPONSE_CODE_TDISP_CAPABILITIES;
    tdisp_capabilities->common.interface_id.function_id =
        TDISP_FUNCTION_ID(pci_get_id(dev), 0, false);
    tdisp_capabilities->dsm_caps = 0;
    tdisp_capabilities->req_msgs_supported[0] =
        TDISP_REQ_MSG_SUPPORTED(GET_TDISP_VERSION) |
        TDISP_REQ_MSG_SUPPORTED(GET_TDISP_CAPABILITIES) |
        TDISP_REQ_MSG_SUPPORTED(LOCK_INTERFACE_REQUEST) |
        TDISP_REQ_MSG_SUPPORTED(GET_DEVICE_INTERFACE_REPORT) |
        TDISP_REQ_MSG_SUPPORTED(GET_DEVICE_INTERFACE_STATE) |
        TDISP_REQ_MSG_SUPPORTED(START_INTERFACE_REQUEST) |
        TDISP_REQ_MSG_SUPPORTED(STOP_INTERFACE_REQUEST);
    tdisp_capabilities->req_msgs_supported[1] = 0;
    tdisp_capabilities->req_msgs_supported[2] = 0;
    tdisp_capabilities->req_msgs_supported[3] = 0;
    tdisp_capabilities->lock_interface_flags_supported = 0;
    tdisp_capabilities->dev_addr_width = sizeof(void *) * 8;
    tdisp_capabilities->num_req_this = 1;
    tdisp_capabilities->num_req_all = 1;
    *response_size = sizeof(TDISPTDISPCapabilities);
    return true;
}

static bool pcie_tdisp_get_response_lock_interface_response(
    PCIDevice *dev, uint32_t session_id, const TDISPMessage *request,
    size_t request_size, TDISPMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    TDISPLockInterfaceRequest *lock_interface_request;
    TDISPLockInterfaceResponse *lock_interface_response;
    SelectiveIDEStream *stream;

    assert(response && response_size);

    if (!dev->ide_cap.session_bound || session_id != dev->ide_cap.session_id) {
        return false;
    }

    if (dev->tdisp_dsm.connection_state <
        TDISP_CONNECTION_STATE_AFTER_CAPABILITIES) {
        return false;
    }

    if (*response_size < sizeof(TDISPLockInterfaceResponse)) {
        return false;
    }

    if (request_size < sizeof(TDISPLockInterfaceRequest)) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_REQUEST, 0,
            response, response_size);
    }

    if (dev->tdisp_dsm.tdi_state != PCI_TDISP_TDI_STATE_CONFIG_UNLOCKED) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version,
            TDISP_ERROR_CODE_INVALID_INTERFACE_STATE, 0, response,
            response_size);
    }

    lock_interface_request = (TDISPLockInterfaceRequest *)request;
    stream = dev->ide_cap.default_stream;

    if (!stream || pcie_sel_ide_stream_id(dev, stream) !=
        lock_interface_request->stream_id_for_default_stream) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_REQUEST, 0,
            response, response_size);
    }

    /*
     * Unclear how to determine the relevant (active?) key set which to val-
     * idate for the TDI's state transition. Just picking a default here for
     * now.
     */
    if (!pcie_tdisp_ide_stream_all_keys_set(dev, &stream->stream.key_sets[0])) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_REQUEST, 0,
            response, response_size);
    }

    dev->tdisp_dsm.tdi_state = PCI_TDISP_TDI_STATE_CONFIG_LOCKED;

    lock_interface_response = (TDISPLockInterfaceResponse *)response;
    lock_interface_response->common.tdisp_version =
        lock_interface_request->common.tdisp_version;
    lock_interface_response->common.message_type =
        PCI_TDISP_RESPONSE_CODE_LOCK_INTERFACE_RESPONSE;
    lock_interface_response->common.interface_id.function_id =
        TDISP_FUNCTION_ID(pci_get_id(dev), 0, false);
    memcpy(
        lock_interface_response->start_interface_nonce,
        dev->tdisp_dsm.start_interface_nonce,
        sizeof(lock_interface_response->start_interface_nonce));
    *response_size = sizeof(TDISPLockInterfaceResponse);
    return true;
}

/*
 * static bool pcie_tdisp_get_response_device_interface_report(
 *     PCIDevice *dev, uint32_t session_id, const TDISPMessage *request,
 *     size_t request_size, TDISPMessage *response, size_t *response_size,
 *     SPDMErrorCode *error_code)
 * {
 *     TDISPGetDeviceInterfaceReport *get_device_interface_report;
 *     TDISPDeviceInterfaceReport *device_interface_report;
 *
 *     assert(response && response_size);
 *
 *     if (!dev->ide_cap.session_bound ||
 *         session_id != dev->ide_cap.session_id) {
 *         return false;
 *     }
 *
 *     if (dev->tdisp_dsm.connection_state <
 *         TDISP_CONNECTION_STATE_AFTER_CAPABILITIES) {
 *         return false;
 *     }
 *
 *     if (request_size < sizeof(TDISPGetDeviceInterfaceReport)) {
 *         return false;
 *     }
 *
 *     if (*response_size < sizeof(TDISPDeviceInterfaceReport)) {
 *         return false;
 *     }
 *
 *     get_device_interface_report = (TDISPGetDeviceInterfaceReport *)request;
 *     device_interface_report = (TDISPDeviceInterfaceReport *)response;
 *
 *     device_interface_report->common.message_type =
 *         PCI_TDISP_RESPONSE_CODE_DEVICE_INTERFACE_REPORT;
 *     return true;
 * }
 */

static bool pcie_tdisp_get_response_device_interface_state(
    PCIDevice *dev, uint32_t session_id, const TDISPMessage *request,
    size_t request_size, TDISPMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    TDISPGetDeviceInterfaceState *get_device_interface_state;
    TDISPDeviceInterfaceState *device_interface_state;

    assert(response && response_size);

    if (!dev->ide_cap.session_bound || session_id != dev->ide_cap.session_id) {
        return false;
    }

    if (dev->tdisp_dsm.connection_state <
        TDISP_CONNECTION_STATE_AFTER_CAPABILITIES) {
        return false;
    }

    if (*response_size < sizeof(TDISPDeviceInterfaceState)) {
        return false;
    }

    if (request_size < sizeof(TDISPGetDeviceInterfaceState)) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_REQUEST, 0,
            response, response_size);
    }

    get_device_interface_state = (TDISPGetDeviceInterfaceState *)request;
    device_interface_state = (TDISPDeviceInterfaceState *)response;
    device_interface_state->common.tdisp_version =
        get_device_interface_state->common.tdisp_version;
    device_interface_state->common.message_type =
        PCI_TDISP_RESPONSE_CODE_DEVICE_INTERFACE_STATE;
    device_interface_state->common.interface_id.function_id =
        TDISP_FUNCTION_ID(pci_get_id(dev), 0, false);
    device_interface_state->tdi_state = dev->tdisp_dsm.tdi_state;
    *response_size = sizeof(TDISPDeviceInterfaceState);
    return true;
}

static bool pcie_tdisp_nonce_valid(
    const uint8_t *nonce, const uint8_t *expected_nonce, size_t nonce_size)
{
    size_t index;
    bool valid = true;

    for (index = 0; index < nonce_size; ++index) {
        valid = valid && (nonce[index] == expected_nonce[index]);
    }

    return valid;
}

static bool pcie_tdisp_get_response_start_interface_response(
    PCIDevice *dev, uint32_t session_id, const TDISPMessage *request,
    size_t request_size, TDISPMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    TDISPStartInterfaceRequest *start_interface_request;
    TDISPStartInterfaceResponse *start_interface_response;

    assert(response && response_size);

    if (!dev->ide_cap.session_bound || session_id != dev->ide_cap.session_id) {
        return false;
    }

    if (dev->tdisp_dsm.connection_state <
        TDISP_CONNECTION_STATE_AFTER_CAPABILITIES) {
        return false;
    }

    if (*response_size < sizeof(TDISPStartInterfaceResponse)) {
        return false;
    }

    if (request_size < sizeof(TDISPStartInterfaceRequest)) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_REQUEST, 0,
            response, response_size);
    }

    if (dev->tdisp_dsm.tdi_state != PCI_TDISP_TDI_STATE_CONFIG_LOCKED) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version,
            TDISP_ERROR_CODE_INVALID_INTERFACE_STATE, 0, response,
            response_size);
    }

    start_interface_request = (TDISPStartInterfaceRequest *)request;

    if (!pcie_tdisp_nonce_valid(
            start_interface_request->start_interface_nonce,
            dev->tdisp_dsm.start_interface_nonce,
            sizeof(start_interface_request->start_interface_nonce))) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_NONCE, 0,
            response, response_size);
    }

    dev->tdisp_dsm.tdi_state = PCI_TDISP_TDI_STATE_RUN;

    start_interface_response = (TDISPStartInterfaceResponse *)response;
    start_interface_response->common.tdisp_version =
        start_interface_request->common.tdisp_version;
    start_interface_response->common.message_type =
        PCI_TDISP_RESPONSE_CODE_START_INTERFACE_RESPONSE;
    start_interface_response->common.interface_id.function_id =
        TDISP_FUNCTION_ID(pci_get_id(dev), 0, false);
    *response_size = sizeof(TDISPStartInterfaceResponse);
    return true;
}

static bool pcie_tdisp_get_response_stop_interface_response(
    PCIDevice *dev, uint32_t session_id, const TDISPMessage *request,
    size_t request_size, TDISPMessage *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    TDISPStopInterfaceRequest *stop_interface_request;
    TDISPStopInterfaceResponse *stop_interface_response;

    assert(response && response_size);

    if (!dev->ide_cap.session_bound || session_id != dev->ide_cap.session_id) {
        return false;
    }

    if (dev->tdisp_dsm.connection_state <
        TDISP_CONNECTION_STATE_AFTER_CAPABILITIES) {
        return false;
    }

    if (*response_size < sizeof(TDISPStopInterfaceResponse)) {
        return false;
    }

    if (request_size < sizeof(TDISPStopInterfaceRequest)) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request->tdisp_version, TDISP_ERROR_CODE_INVALID_REQUEST, 0,
            response, response_size);
    }

    stop_interface_request = (TDISPStopInterfaceRequest *)request;

    dev->tdisp_dsm.tdi_state = PCI_TDISP_TDI_STATE_CONFIG_UNLOCKED;

    stop_interface_response = (TDISPStopInterfaceResponse *)response;
    stop_interface_response->common.tdisp_version =
        stop_interface_request->common.tdisp_version;
    stop_interface_response->common.message_type =
        PCI_TDISP_RESPONSE_CODE_STOP_INTERFACE_RESPONSE;
    stop_interface_response->common.interface_id.function_id =
        TDISP_FUNCTION_ID(pci_get_id(dev), 0, false);
    *response_size = sizeof(TDISPStopInterfaceResponse);
    return true;
}

bool pcie_tdisp_get_response(
    PCIDevice *dev, uint32_t session_id, const PCIPayload *request,
    size_t request_size, PCIPayload *response, size_t *response_size,
    SPDMErrorCode *error_code)
{
    TDISPMessage *request_message, *response_message;
    bool success;

    assert(request && response && request_size >= sizeof(PCIPayload) &&
        response_size && request->protocol_id == PCI_SPDM_PROTOCOL_ID_TDISP &&
        error_code);

    if (request_size < sizeof(TDISPMessage)) {
        return false;
    }

    if (*response_size < sizeof(TDISPMessage)) {
        *error_code = SPDM_ERROR_CODE_INVALID_REQUEST;
        return true;
    }

    request_message = (TDISPMessage *)request;
    response_message = (TDISPMessage *)response;
    response_message->payload.protocol_id = PCI_SPDM_PROTOCOL_ID_TDISP;

    if (dev->tdisp_dsm.connection_state >=
        TDISP_CONNECTION_STATE_AFTER_CAPABILITIES &&
        request_message->tdisp_version != dev->tdisp_dsm.tdisp_version) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request_message->tdisp_version,
            TDISP_ERROR_CODE_VERSION_MISMATCH, 0, response_message,
            response_size);
    }

    if (request_message->interface_id.function_id !=
        TDISP_FUNCTION_ID(pci_get_id(dev), 0, false)) {
        return pcie_tdisp_get_response_tdisp_error(
            dev, request_message->tdisp_version,
            TDISP_ERROR_CODE_INVALID_INTERFACE, 0, response_message,
            response_size);
    }

    switch (request_message->message_type) {
    case PCI_TDISP_REQUEST_CODE_GET_TDISP_VERSION:
        success = pcie_tdisp_get_response_tdisp_version(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    case PCI_TDISP_REQUEST_CODE_GET_TDISP_CAPABILITIES:
        success = pcie_tdisp_get_response_tdisp_capabilities(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    case PCI_TDISP_REQUEST_CODE_LOCK_INTERFACE_REQUEST:
        success = pcie_tdisp_get_response_lock_interface_response(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
/*
 *  case PCI_TDISP_REQUEST_CODE_GET_DEVICE_INTERFACE_REPORT:
 *      success = pcie_tdisp_get_response_device_interface_report(
 *          dev, session_id, request_message, request_size, response_message,
 *          response_size, error_code);
 *      break;
 */
    case PCI_TDISP_REQUEST_CODE_GET_DEVICE_INTERFACE_STATE:
        success = pcie_tdisp_get_response_device_interface_state(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    case PCI_TDISP_REQUEST_CODE_START_INTERFACE_REQUEST:
        success = pcie_tdisp_get_response_start_interface_response(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    case PCI_TDISP_REQUEST_CODE_STOP_INTERFACE_REQUEST:
        success = pcie_tdisp_get_response_stop_interface_response(
            dev, session_id, request_message, request_size, response_message,
            response_size, error_code);
        break;
    case PCI_TDISP_REQUEST_CODE_BIND_P2P_STREAM_REQUEST:
        /* INVALID_REQUEST if P2P streams unsupported unsupported */
        return pcie_tdisp_get_response_tdisp_error(
            dev, request_message->tdisp_version,
            TDISP_ERROR_CODE_INVALID_REQUEST, request_message->message_type,
            response_message, response_size);
    case PCI_TDISP_REQUEST_CODE_UNBIND_P2P_STREAM_REQUEST:
        /* INVALID_REQUEST if P2P streams unsupported unsupported */
        return pcie_tdisp_get_response_tdisp_error(
            dev, request_message->tdisp_version,
            TDISP_ERROR_CODE_INVALID_REQUEST, request_message->message_type,
            response_message, response_size);
    case PCI_TDISP_REQUEST_CODE_SET_MMIO_ATTRIBUTE_REQUEST:
        /* INVALID_REQUEST if updateable MMIO settings unsupported */
        return pcie_tdisp_get_response_tdisp_error(
            dev, request_message->tdisp_version,
            TDISP_ERROR_CODE_INVALID_REQUEST, request_message->message_type,
            response_message, response_size);
    case PCI_TDISP_REQUEST_CODE_VDM_REQUEST:
        return pcie_tdisp_get_response_tdisp_error(
            dev, request_message->tdisp_version,
            TDISP_ERROR_CODE_UNSUPPORTED_REQUEST,
            request_message->message_type, response_message, response_size);
    default:
        return pcie_tdisp_get_response_tdisp_error(
            dev, request_message->tdisp_version,
            TDISP_ERROR_CODE_UNSUPPORTED_REQUEST,
            request_message->message_type, response_message, response_size);
    }

    return success;
}
