#ifndef HW_SPDM_SPDM_RESPONDER_LIBSPDM_H
#define HW_SPDM_SPDM_RESPONDER_LIBSPDM_H

#include "qemu/osdep.h"
#include "qom/object_interfaces.h"
#include "hw/spdm/spdm-responder.h"

#define TYPE_SPDM_RESPONDER_LIBSDPM "spdm-responder-libspdm"
OBJECT_DECLARE_SIMPLE_TYPE(SPDMResponderLibspdm, SPDM_RESPONDER_LIBSDPM)

struct SPDMResponderLibspdm {
    /*< private >*/
    SPDMResponder parent_obj;
    /*< public >*/

    DeviceState *dev;

    void *spdm_context;
    void *scratch_buffer;

    SPDMResponderSendMessageFunc *send_message;
    SPDMResponderReceiveMessageFunc *receive_message;
    SPDMResponderGetResponseFunc *get_response;

    uint32_t max_spdm_msg_size;
    uint32_t data_transfer_size;
    uint32_t buffer_size;
    void *sender_buffer, *receiver_buffer;

    uint32_t capabilities;
};

#endif /* HW_SPDM_SPDM_RESPONDER_LIBSPDM_H */
