#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "hw/spdm/spdm-responder.h"

OBJECT_DEFINE_ABSTRACT_TYPE(
    SPDMResponder, spdm_responder, SPDM_RESPONDER, OBJECT)

bool device_spdm_responder_init(DeviceState *dev, SPDMResponder *responder,
    SPDMResponderSendMessageFunc send_message,
    SPDMResponderReceiveMessageFunc receive_message,
    SPDMResponderGetResponseFunc get_response, Error **errp)
{
    SPDMResponderClass *class = SPDM_RESPONDER_GET_CLASS(responder);
    assert(class->device_init);
    return class->device_init(
        responder, dev, send_message, receive_message, get_response, errp);
}

bool spdm_responder_dispatch_message(SPDMResponder *responder, Error **errp)
{
    SPDMResponderClass *class = SPDM_RESPONDER_GET_CLASS(responder);
    assert(class->dispatch_message);
    return class->dispatch_message(responder, errp);
}

static void spdm_responder_init(Object *obj)
{
}

static void spdm_responder_finalize(Object *obj)
{
}

static void spdm_responder_class_init(ObjectClass *oc, void *data)
{
}
