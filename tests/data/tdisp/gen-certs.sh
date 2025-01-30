#!/bin/bash

set -e -o pipefail

pushd rsa3072
openssl req -nodes -x509 -days 3650 -newkey rsa:3072 -keyout ca.key \
    -out ca.cert -sha384 -subj "/CN=QEMU spdm-responder RSA CA"
openssl rsa -in ca.key -outform der -out ca.key.der
openssl req -nodes -newkey rsa:3072 -keyout intermediate.key \
    -out intermediate.req -sha384 -batch \
    -subj "/CN=QEMU spdm-responder RSA intermediate cert"
openssl req -nodes -newkey rsa:3072 -keyout device.key -out device.req \
    -sha384 -batch -subj "/CN=QEMU spdm-responder RSA device cert"
openssl x509 -req -in intermediate.req -out intermediate.cert -CA ca.cert \
    -CAkey ca.key -sha384 -days 3650 -set_serial 1 -extensions v3_inter \
    -extfile ../openssl.cnf
openssl x509 -req -in device.req -out device.cert -CA intermediate.cert \
    -CAkey intermediate.key -sha384 -days 3650 -set_serial 3 \
    -extensions v3_end -extfile ../openssl.cnf
openssl asn1parse -in ca.cert -out ca.cert.der
openssl asn1parse -in intermediate.cert -out intermediate.cert.der
openssl asn1parse -in device.cert -out device.cert.der
cat ca.cert.der intermediate.cert.der device.cert.der > device.certchain.der
popd

pushd ecp256
openssl genpkey -genparam -out param.pem -algorithm EC \
    -pkeyopt ec_paramgen_curve:P-256
openssl req -nodes -x509 -days 3650 -newkey ec:param.pem -keyout ca.key \
    -out ca.cert -sha256 -subj "/CN=QEMU spdm-responder ECP256 CA"
openssl pkey -in ca.key -outform der -out ca.key.der
openssl req -nodes -newkey ec:param.pem -keyout intermediate.key \
    -out intermediate.req -sha256 -batch \
    -subj "/CN=QEMU spdm-responder ECP256 intermediate cert"
openssl req -nodes -newkey ec:param.pem -keyout device.key -out device.req \
    -sha256 -batch -subj "/CN=QEMU spdm-responder ECP256 device cert"
openssl x509 -req -in intermediate.req -out intermediate.cert -CA ca.cert \
    -CAkey ca.key -sha256 -days 3650 -set_serial 1 -extensions v3_inter \
    -extfile ../openssl.cnf
openssl x509 -req -in device.req -out device.cert -CA intermediate.cert \
    -CAkey intermediate.key -sha256 -days 3650 -set_serial 3 \
    -extensions v3_end -extfile ../openssl.cnf
openssl asn1parse -in ca.cert -out ca.cert.der
openssl asn1parse -in intermediate.cert -out intermediate.cert.der
openssl asn1parse -in device.cert -out device.cert.der
cat ca.cert.der intermediate.cert.der device.cert.der > device.certchain.der
popd

pushd ecp384
openssl genpkey -genparam -out param.pem -algorithm EC \
    -pkeyopt ec_paramgen_curve:P-384
openssl req -nodes -x509 -days 3650 -newkey ec:param.pem -keyout ca.key \
    -out ca.cert -sha384 -subj "/CN=QEMU spdm-responder ECP384 CA"
openssl pkey -in ca.key -outform der -out ca.key.der
openssl req -nodes -newkey ec:param.pem -keyout intermediate.key \
    -out intermediate.req -sha384 -batch \
    -subj "/CN=QEMU spdm-responder ECP384 intermediate cert"
openssl req -nodes -newkey ec:param.pem -keyout device.key -out device.req \
    -sha384 -batch -subj "/CN=QEMU spdm-responder ECP384 device cert"
openssl x509 -req -in intermediate.req -out intermediate.cert -CA ca.cert \
    -CAkey ca.key -sha384 -days 3650 -set_serial 1 -extensions v3_inter \
    -extfile ../openssl.cnf
openssl x509 -req -in device.req -out device.cert -CA intermediate.cert \
    -CAkey intermediate.key -sha384 -days 3650 -set_serial 3 \
    -extensions v3_end -extfile ../openssl.cnf
openssl asn1parse -in ca.cert -out ca.cert.der
openssl asn1parse -in intermediate.cert -out intermediate.cert.der
openssl asn1parse -in device.cert -out device.cert.der
cat ca.cert.der intermediate.cert.der device.cert.der > device.certchain.der
popd
