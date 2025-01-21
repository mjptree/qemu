#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "qapi/qmp/qlist.h"
#include "hw/qdev-core.h"
#include "hw/spdm/spdm-responder-libspdm.h"

#ifndef LIBSPDM_HAL_PASS_SPDM_CONTEXT
#define LIBSPDM_HAL_PASS_SPDM_CONTEXT 1
#endif

/*< libspdm >*/
#include "industry_standard/spdm.h"
#include "hal/library/requester/reqasymsignlib.h"
#include "hal/library/responder/asymsignlib.h"
#include "hal/library/responder/csrlib.h"
#include "hal/library/responder/measlib.h"
#include "hal/library/responder/psklib.h"
#include "hal/library/responder/setcertlib.h"
#include "library/spdm_common_lib.h"
#include "library/spdm_responder_lib.h"
#include "library/spdm_secured_message_lib.h"
#include "library/spdm_transport_pcidoe_lib.h"

#define DATA_TRANSFER_SIZE_DEFAULT 0x1200
#define DATA_TRANSFER_SIZE_PROP    "data-transfer-size"

#define MAX_SPDM_MSG_SIZE_DEFAULT  0x1200
#define MAX_SPDM_MSG_SIZE_PROP     "max-spdm-msg-size"

bool libspdm_requester_data_sign(
    void *spdm_context, spdm_version_number_t spdm_version, uint8_t op_code,
    uint16_t req_base_asym_alg, uint32_t base_hash_algo, bool is_data_hash,
    const uint8_t *message, size_t message_size, uint8_t *signature,
    size_t *sig_size)
{
    return false;
}

bool libspdm_encap_challenge_opaque_data(
    void *spdm_context, spdm_version_number_t spdm_version, uint8_t slot_id,
    uint8_t *measurement_summary_hash, size_t measurement_summary_hash_size,
    void *opaque_data, size_t *opaque_data_size)
{
    return false;
}

bool libspdm_challenge_opaque_data(
    void *spdm_context, spdm_version_number_t spdm_version, uint8_t slot_id,
    uint8_t *measurment_summary_hash, size_t measurement_summary_hash_size,
    void *opaque_data, size_t *opaque_data_size)
{
    return false;
}

bool libspdm_responder_data_sign(
    void *spdm_context, spdm_version_number_t spdm_version, uint8_t op_code,
    uint32_t base_asym_algo, uint32_t base_hash_algo, bool is_data_hash,
    const uint8_t *message, size_t message_size, uint8_t *signature,
    size_t *sig_size)
{
    return false;
}

bool libspdm_gen_csr(
    void *spdm_context, uint32_t base_hash_algo, uint32_t base_asym_algo,
    bool *need_reset, const void *request, size_t request_size,
    uint8_t *requester_info, size_t requester_info_length,
    uint8_t *opaque_data, uint16_t opaque_data_length, size_t *csr_len,
    uint8_t *csr_pointer, bool is_device_cert_model)
{
    return false;
}

bool libspdm_gen_csr_ex(
    void *spdm_context, uint32_t base_hash_algo, uint32_t base_asym_algo,
    bool *need_reset, const void *request, size_t request_size,
    uint8_t *requester_info, size_t requester_info_length,
    uint8_t *opaque_data, uint16_t opaque_data_length, size_t *csr_len,
    uint8_t *csr_pointer, uint8_t req_cert_model,
    uint8_t *req_csr_tracking_tag, uint8_t req_key_pair_id, bool overwrite)
{
    return false;
}

libspdm_return_t libspdm_measurement_collection(
    void *spdm_context, spdm_version_number_t spdm_version,
    uint8_t measurement_specification, uint32_t measurement_hash_algo,
    uint8_t measurement_index, uint8_t request_attribute,
    uint8_t *content_changed, uint8_t *measurements_count, void *measurements,
    size_t *measurements_size)
{
    return LIBSPDM_STATUS_UNSUPPORTED_CAP;
}

bool libspdm_measurement_opaque_data(
    void *spdm_context, spdm_version_number_t spdm_version,
    uint8_t measurement_specification, uint32_t measurement_hash_algo,
    uint8_t measurement_index, uint8_t request_attribute, void *opaque_data,
    size_t *opaque_data_size)
{
    return false;
}

bool libspdm_generate_measurement_summary_hash(
    void *spdm_context, spdm_version_number_t spdm_version,
    uint32_t base_hash_algo, uint8_t measurement_specification,
    uint32_t measurement_hash_algo, uint8_t measurement_summary_hash_type,
    uint8_t *measurement_summary_hash, uint32_t measurement_summary_hash_size)
{
    return false;
}

bool libspdm_psk_handshake_secret_hkdf_expand(
    spdm_version_number_t spdm_version, uint32_t base_hash_algo,
    const uint8_t *psk_hint, size_t psk_hint_size, const uint8_t *info,
    size_t info_size, uint8_t *out, size_t out_size)
{
    return false;
}

bool libspdm_psk_master_secret_hkdf_expand(
    spdm_version_number_t spdm_version, uint32_t base_hash_algo,
    const uint8_t *psk_hint, size_t psk_hint_size, const uint8_t *info,
    size_t info_size, uint8_t *out, size_t out_size)
{
    return false;
}

bool libspdm_is_in_trusted_environment(void *spdm_context)
{
    return false;
}

bool libspdm_write_certificate_to_nvm(
    void *spdm_context, uint8_t slot_id, const void *cert_chain,
    size_t cert_chain_size, uint32_t base_hash_algo, uint32_t base_asym_algo)
{
    return false;
}


static const char *spdm_status_get_pretty(const libspdm_return_t status)
{
    switch (status) {
    case LIBSPDM_STATUS_SUCCESS:
        return "success";
    case LIBSPDM_STATUS_INVALID_PARAMETER:
        return "the function input parameter is invalid";
    case LIBSPDM_STATUS_UNSUPPORTED_CAP:
        return "capability is unsupported by either the caller, the peer, "
                "or both";
    case LIBSPDM_STATUS_INVALID_STATE_LOCAL:
        return "caller's state is invalid";
    case LIBSPDM_STATUS_INVALID_STATE_PEER:
        return "peer's state is invalid";
    case LIBSPDM_STATUS_INVALID_MSG_FIELD:
        return "the received message contains an invalid message field";
    case LIBSPDM_STATUS_INVALID_MSG_SIZE:
        return "the received message's size is invalid";
    case LIBSPDM_STATUS_NEGOTIATION_FAIL:
        return "unable to derive common set of versions, algorithms, etc.";
    case LIBSPDM_STATUS_BUSY_PEER:
        return "peer is busy";
    case LIBSPDM_STATUS_NOT_READY_PEER:
        return "peer is not ready";
    case LIBSPDM_STATUS_ERROR_PEER:
        return "peer encountered an unexpected error";
    case LIBSPDM_STATUS_RESYNCH_PEER:
        return "peer requested a resync";
    case LIBSPDM_STATUS_BUFFER_FULL:
        return "buffer is full";
    case LIBSPDM_STATUS_BUFFER_TOO_SMALL:
        return "buffer is too small";
    case LIBSPDM_STATUS_SESSION_NUMBER_EXCEED:
        return "unable to allocate more sessions";
    case LIBSPDM_STATUS_SESSION_MSG_ERROR:
        return "peer encountered a decrypt error";
    case LIBSPDM_STATUS_ACQUIRE_FAIL:
        return "unable to acquire resource";
    case LIBSPDM_STATUS_SESSION_TRY_DISCARD_KEY_UPDATE:
        return "peer encountered a retryable decrypt error";
    case LIBSPDM_STATUS_RESET_REQUIRED_PEER:
        return "peer requires reset";
    case LIBSPDM_STATUS_PEER_BUFFER_TOO_SMALL:
        return "peer's buffer is too small";
    case LIBSPDM_STATUS_OVERRIDDEN_PARAMETER:
        return "a function paramater was overriden";
    case LIBSPDM_STATUS_CRYPTO_ERROR:
        return "crypto module encountered an unexpected error";
    case LIBSPDM_STATUS_VERIF_FAIL:
        return "verification of signature, digest, or AEAD tag failed";
    case LIBSPDM_STATUS_SEQUENCE_NUMBER_OVERFLOW:
        return "sequence number overflowed";
    case LIBSPDM_STATUS_VERIF_NO_AUTHORITY:
        return "certificate is valid but not authoritative";
    case LIBSPDM_STATUS_FIPS_FAIL:
        return "FIPS test failed";
    case LIBSPDM_STATUS_INVALID_CERT:
        return "certificate is invalid";
    case LIBSPDM_STATUS_SEND_FAIL:
        return "failed to send message to peer";
    case LIBSPDM_STATUS_RECEIVE_FAIL:
        return "failed to receive message from peer";
    case LIBSPDM_STATUS_MEAS_INVALID_INDEX:
        return "measurement index is invalid";
    case LIBSPDM_STATUS_MEAS_INTERNAL_ERROR:
        return "unable to collect measurement due to internal error";
    case LIBSPDM_STATUS_LOW_ENTROPY:
        return "unable to generate random number due to lack of entropy";
    default:
        return "unknown";
    }
}

static SPDMResponderLibspdm *spdm_responder_libspdm_get_from_context(
    void *spdm_context)
{
    libspdm_data_parameter_t parameter = {
        .location = LIBSPDM_DATA_LOCATION_LOCAL
    };
    libspdm_return_t status;
    void *responder = NULL;
    size_t data_size = sizeof(responder);
    status = libspdm_get_data(spdm_context, LIBSPDM_DATA_APP_CONTEXT_DATA,
                              &parameter, &responder, &data_size);
    assert(LIBSPDM_STATUS_IS_SUCCESS(status));
    return SPDM_RESPONDER_LIBSDPM(responder);
}

static libspdm_return_t spdm_responder_libspdm_send_message(
    void *spdm_context, size_t message_size, const void *message,
    uint64_t timeout)
{
    SPDMResponderLibspdm *responder =
        spdm_responder_libspdm_get_from_context(spdm_context);
    assert(responder->send_message);

    if (!responder->send_message(responder->dev, message_size, message)) {
        return LIBSPDM_STATUS_SEND_FAIL;
    }

    return LIBSPDM_STATUS_SUCCESS;
}

static libspdm_return_t spdm_responder_libspdm_receive_message(
    void *spdm_context, size_t *message_size, void **message, uint64_t timeout)
{
    SPDMResponderLibspdm *responder =
        spdm_responder_libspdm_get_from_context(spdm_context);
    assert(responder->receive_message);

    if (!responder->receive_message(responder->dev, message_size, message)) {
        return LIBSPDM_STATUS_RECEIVE_FAIL;
    }

    return LIBSPDM_STATUS_SUCCESS;
}

static libspdm_return_t spdm_responder_libspdm_acquire_sender_buffer(
    void *spdm_context, void **msg_buf_ptr)
{
    SPDMResponderLibspdm *responder =
        spdm_responder_libspdm_get_from_context(spdm_context);
    *msg_buf_ptr = responder->sender_buffer;
    return LIBSPDM_STATUS_SUCCESS;
}

static libspdm_return_t spdm_responder_libspdm_acquire_receiver_buffer(
    void *spdm_context, void **msg_buf_ptr)
{
    SPDMResponderLibspdm *responder =
        spdm_responder_libspdm_get_from_context(spdm_context);
    *msg_buf_ptr = responder->receiver_buffer;
    return LIBSPDM_STATUS_SUCCESS;
}

static void spdm_responder_libspdm_release_buffer(void *spdm_context,
    const void *msg_buf_ptr)
{
}

static libspdm_return_t spdm_responder_libspdm_get_response(
    void *spdm_context, const uint32_t *session_id, bool is_app_message,
    size_t request_size, const void *request, size_t *response_size,
    void *response)
{
    SPDMResponderLibspdm *responder;

    if (is_app_message) {
        responder = spdm_responder_libspdm_get_from_context(spdm_context);
        assert(responder->get_response);
        responder->get_response(responder->dev, session_id, request_size,
            request, response_size, response);
    } else {
        return LIBSPDM_STATUS_UNSUPPORTED_CAP;
    }
}

static void spdm_responder_libspdm_complete(UserCreatable *obj, Error **errp)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);
    libspdm_data_parameter_t parameter;
    libspdm_return_t status;
    size_t scratch_buffer_size;

    if (responder->max_spdm_msg_size < responder->data_transfer_size) {
        error_setg(errp, "%s must be greater than or equal to %s",
                   MAX_SPDM_MSG_SIZE_PROP, DATA_TRANSFER_SIZE_PROP);
        return;
    }

    responder->spdm_context = g_malloc(libspdm_get_context_size());
    status = libspdm_init_context(responder->spdm_context);

    if (!LIBSPDM_STATUS_IS_SUCCESS(status)) {
        error_setg(errp, "libspdm init context failed with error: %s",
                   spdm_status_get_pretty(status));
        return;
    }

    responder->sender_buffer = g_malloc0(responder->buffer_size);
    responder->receiver_buffer = g_malloc0(responder->buffer_size);
    libspdm_register_device_io_func(responder->spdm_context,
        spdm_responder_libspdm_send_message,
        spdm_responder_libspdm_receive_message);
    libspdm_register_transport_layer_func(responder->spdm_context,
        responder->max_spdm_msg_size,
        LIBSPDM_PCI_DOE_TRANSPORT_HEADER_SIZE,
        LIBSPDM_PCI_DOE_TRANSPORT_TAIL_SIZE,
        libspdm_transport_pci_doe_encode_message,
        libspdm_transport_pci_doe_decode_message);
    libspdm_register_device_buffer_func(responder->spdm_context,
        responder->buffer_size, responder->buffer_size,
        spdm_responder_libspdm_acquire_sender_buffer,
        spdm_responder_libspdm_release_buffer,
        spdm_responder_libspdm_acquire_receiver_buffer,
        spdm_responder_libspdm_release_buffer);
    libspdm_register_get_response_func(responder->spdm_context,
        spdm_responder_libspdm_get_response);

    scratch_buffer_size =
        libspdm_get_sizeof_required_scratch_buffer(responder->spdm_context);
    responder->scratch_buffer = g_malloc0(scratch_buffer_size);
    libspdm_set_scratch_buffer(responder->spdm_context,
        responder->scratch_buffer, scratch_buffer_size);

    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
    status = libspdm_set_data(responder->spdm_context,
        LIBSPDM_DATA_CAPABILITY_FLAGS, &parameter, &responder->capabilities,
        sizeof(responder->capabilities));

    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        error_setg(errp, "libspdm set capabilities failed with error: %s",
                   spdm_status_get_pretty(status));
        return;
    }

    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
    status = libspdm_set_data(responder->spdm_context,
        LIBSPDM_DATA_APP_CONTEXT_DATA, &parameter, &responder,
        sizeof(responder));

    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        error_setg(errp, "libspdm set app context data failed with error: %s",
                   spdm_status_get_pretty(status));
        return;
    }

    if (!libspdm_check_context(responder->spdm_context)) {
        error_setg(errp, "libspdm responder context failed validation");
        return;
    }
}

static bool spdm_responder_libspdm_can_be_deleted(UserCreatable *obj)
{
    return OBJECT(obj)->ref == 1;
}

static bool spdm_responder_libspdm_device_init(
    SPDMResponder *obj, DeviceState *dev,
    SPDMResponderSendMessageFunc send_message,
    SPDMResponderReceiveMessageFunc receive_message,
    SPDMResponderGetResponseFunc get_response, Error **errp)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);

    if (responder->dev) {
        error_setg(errp, "cannot bind %s to more than one device",
                   object_get_typename(OBJECT(obj)));
        return false;
    }

    responder->dev = dev;
    responder->send_message = send_message;
    responder->receive_message = receive_message;
    responder->get_response = get_response;
    return true;
}

static bool spdm_responder_libspdm_dispatch_message(
    SPDMResponder *obj, Error **errp)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);
    libspdm_return_t status =
        libspdm_responder_dispatch_message(responder->spdm_context);

    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        error_setg(errp,
                   "spdm responder failed to process message with error: %s",
                   spdm_status_get_pretty(status));
        return false;
    }

    return true;
}

OBJECT_DEFINE_SIMPLE_TYPE_WITH_INTERFACES(
    SPDMResponderLibspdm, spdm_responder_libspdm, SPDM_RESPONDER_LIBSDPM,
    SPDM_RESPONDER, { TYPE_USER_CREATABLE }, { })

static void spdm_responder_libspdm_get_max_spdm_msg_size(
    Object *obj, Visitor *v, const char *name, void *opaque, Error **errp)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);
    visit_type_uint32(v, name, &responder->max_spdm_msg_size, errp);
}

static void spdm_responder_libspdm_set_max_spdm_msg_size(
    Object *obj, Visitor *v, const char *name, void *opaque, Error **errp)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);
    uint32_t max_spdm_msg_size;

    if (responder->spdm_context) {
        error_setg(errp, "cannot change property '%s' of '%s'", name,
                   object_get_typename(obj));
        return;
    }

    if (!visit_type_uint32(v, name, &max_spdm_msg_size, errp)) {
        return;
    }

    responder->max_spdm_msg_size = max_spdm_msg_size;
}

static void spdm_responder_libspdm_get_data_transfer_size(
    Object *obj, Visitor *v, const char *name, void *opaque, Error **errp)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);
    visit_type_uint32(v, name, &responder->data_transfer_size, errp);
}

static void spdm_responder_libspdm_set_data_transfer_size(
    Object *obj, Visitor *v, const char *name, void *opaque, Error **errp)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);
    uint32_t data_transfer_size;

    if (responder->spdm_context) {
        error_setg(errp, "cannot change property '%s' of '%s'", name,
                   object_get_typename(obj));
        return;
    }

    if (!visit_type_uint32(v, name, &data_transfer_size, errp)) {
        return;
    }

    if (data_transfer_size < SPDM_MIN_DATA_TRANSFER_SIZE_VERSION_12) {
        error_setg(errp, "DataTransferSize must be greater than or equal to "
                   "MinDataTransferSize");
        error_append_hint(errp, "MinDataTransferSize=%d",
                          SPDM_MIN_DATA_TRANSFER_SIZE_VERSION_12);
        return;
    }

    responder->data_transfer_size = data_transfer_size;
    responder->buffer_size = LIBSPDM_PCI_DOE_TRANSPORT_HEADER_SIZE
        + responder->data_transfer_size + LIBSPDM_PCI_DOE_TRANSPORT_TAIL_SIZE;
}

static void spdm_responder_libspdm_init(Object *obj)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);
    responder->capabilities = SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CERT_CAP |
        SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CHAL_CAP |
        SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_ENCRYPT_CAP |
        SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_MAC_CAP |
        SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_KEY_EX_CAP |
        SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CHUNK_CAP;
}

static void spdm_responder_libspdm_finalize(Object *obj)
{
    SPDMResponderLibspdm *responder = SPDM_RESPONDER_LIBSDPM(obj);

    libspdm_deinit_context(responder->spdm_context);
    g_free(responder->spdm_context);
    g_free(responder->scratch_buffer);
    g_free(responder->sender_buffer);
    g_free(responder->receiver_buffer);
}

static void spdm_responder_libspdm_class_init(ObjectClass *klass, void *data)
{
    UserCreatableClass *ucc = USER_CREATABLE_CLASS(klass);
    SPDMResponderClass *src = SPDM_RESPONDER_CLASS(klass);
    ObjectProperty *property;

    ucc->complete = spdm_responder_libspdm_complete;
    ucc->can_be_deleted = spdm_responder_libspdm_can_be_deleted;
    src->device_init = spdm_responder_libspdm_device_init;
    src->dispatch_message = spdm_responder_libspdm_dispatch_message;

    property = object_class_property_add(klass, DATA_TRANSFER_SIZE_PROP,
        "uint32", spdm_responder_libspdm_get_data_transfer_size,
        spdm_responder_libspdm_set_data_transfer_size, NULL, NULL);
    object_property_set_default_uint(property, DATA_TRANSFER_SIZE_DEFAULT);
    object_class_property_set_description(klass, DATA_TRANSFER_SIZE_PROP,
        "Buffer size of the responder to receive a single SPDM message "
        "excluding transport headers, padding, etc.");

    property = object_class_property_add(klass, MAX_SPDM_MSG_SIZE_PROP,
        "uint32", spdm_responder_libspdm_get_max_spdm_msg_size,
        spdm_responder_libspdm_set_max_spdm_msg_size, NULL, NULL);
    object_property_set_default_uint(property, MAX_SPDM_MSG_SIZE_DEFAULT);
    object_class_property_set_description(klass, MAX_SPDM_MSG_SIZE_PROP,
        "Buffer size of the responder to reassemble a complete large SPDM "
        "message, must be greater than or equal to 'data-transfer-size'");
}
