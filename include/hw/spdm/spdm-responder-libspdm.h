#ifndef HW_SPDM_SPDM_RESPONDER_LIBSPDM_H
#define HW_SPDM_SPDM_RESPONDER_LIBSPDM_H

#include "qemu/osdep.h"
#include "qom/object_interfaces.h"
#include "hw/spdm/spdm-responder.h"

#define TYPE_SPDM_RESPONDER_LIBSDPM "spdm-responder-libspdm"
OBJECT_DECLARE_SIMPLE_TYPE(SPDMResponderLibspdm, SPDM_RESPONDER_LIBSDPM)

extern bool libspdm_asym_get_private_key_from_pem(
    uint32_t base_asym_algo, const uint8_t *pem_data, size_t pem_size,
    const char *password, void **context);

#endif /* HW_SPDM_SPDM_RESPONDER_LIBSPDM_H */
