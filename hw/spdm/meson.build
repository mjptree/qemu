spdm_ss = ss.source_set()
spdm_ss.add(files(
  'spdm-responder.c',
))
spdm_ss.add(when: libspdm, if_true: [files('spdm-responder-libspdm.c'), libspdm])
system_ss.add_all(when: 'CONFIG_SPDM', if_true: spdm_ss)
