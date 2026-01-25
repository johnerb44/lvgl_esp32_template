# Install script for directory: G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/lvgl_porting")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "G:/vscode/lvgl/.espressif/tools/xtensa-esp32s3-elf/esp-12.2.0_20230208/xtensa-esp32s3-elf/bin/xtensa-esp32s3-elf-objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mbedtls" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/aes.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/aria.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/asn1.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/asn1write.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/base64.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/bignum.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/block_cipher.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/build_info.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/camellia.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ccm.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/chacha20.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/chachapoly.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/check_config.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/cipher.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/cmac.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/compat-2.x.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_legacy_crypto.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_legacy_from_psa.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_psa_from_legacy.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_psa_superset_legacy.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_ssl.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_x509.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_psa.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/constant_time.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ctr_drbg.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/debug.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/des.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/dhm.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecdh.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecdsa.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecjpake.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecp.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/entropy.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/error.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/gcm.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/hkdf.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/hmac_drbg.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/lms.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/mbedtls_config.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md5.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/memory_buffer_alloc.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/net_sockets.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/nist_kw.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/oid.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pem.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pk.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs12.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs5.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs7.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform_time.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform_util.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/poly1305.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/private_access.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/psa_util.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ripemd160.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/rsa.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha1.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha256.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha3.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha512.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_cache.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_ciphersuites.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_cookie.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_ticket.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/threading.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/timing.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/version.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_crl.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_crt.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_csr.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/psa" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/build_info.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_adjust_auto_enabled.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_dependencies.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_key_pair_types.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_synonyms.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_builtin_composites.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_builtin_key_derivation.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_builtin_primitives.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_compat.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_config.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_driver_common.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_composites.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_key_derivation.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_primitives.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_extra.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_legacy.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_platform.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_se_driver.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_sizes.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_struct.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_types.h"
    "G:/vscode/lvgl/esp/v5.1.6/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_values.h"
    )
endif()

