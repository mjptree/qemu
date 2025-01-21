#ifndef HW_SPDM_SPDM_RESPONDER_H
#define HW_SPDM_SPDM_RESPONDER_H

#include "qemu/osdep.h"
#include "qom/object_interfaces.h"

#define TYPE_SPDM_RESPONDER "spdm-responder"
OBJECT_DECLARE_TYPE(SPDMResponder, SPDMResponderClass, SPDM_RESPONDER)

typedef bool SPDMResponderSendMessageFunc(
    DeviceState *dev, size_t message_size, const void *message);
typedef bool SPDMResponderReceiveMessageFunc(
    DeviceState *dev, size_t *message_size, void **message);
typedef bool SPDMResponderGetResponseFunc(
    DeviceState *dev, const uint32_t *session_id, size_t request_size,
    const void *request, size_t *response_size, void *response);

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

#endif /* HW_SPDM_SPDM_RESPONDER_H */
