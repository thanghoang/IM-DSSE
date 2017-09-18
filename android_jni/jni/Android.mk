LOCAL_PATH := $(call my-dir)

# NDK_DEBUG_IMPORTS := 1
# libtom library

#########################################################
# GNUport library
include $(CLEAR_VARS)

GNUPORT_INCL     := /scratch/android/android-ndk/sources/cxx-stl/gnu-libstdc++/4.9/include
GNUPORT_INCL     += /scratch/android/android-ndk/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH_ABI)/include
GNUPORT_LIB      := /scratch/android/android-ndk/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH_ABI)
LOCAL_MODULE := gnustd_shared
LOCAL_SRC_FILES := $(GNUPORT_LIB)/libgnustl_shared.so
LOCAL_CPP_FEATURES += rtti exceptions

LOCAL_EXPORT_CPPFLAGS :=
LOCAL_EXPORT_C_INCLUDES := $(GNUPORT_INCL)

include $(PREBUILT_SHARED_LIBRARY)

LOCAL_SHARED_LIBRARIES  := gnustd_shared


#########################################################
#libtomcrypt
include $(CLEAR_VARS)

LOCAL_MODULE := tomcrypt
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/headers $(LOCAL_PATH)/..
APP_STL         := gnustl_shared

TOM_INCL   := $(LOCAL_PATH)/src/headers/
TOM_LIB    := $(LOCAL_PATH)/obj/local/$(TARGET_ARCH_ABI)/

LOCAL_CFLAGS += -DDROPBEAR_CLIENT
#LOCAL_CPPFLAGS  += -std=c++11 
#LOCAL_CFLAGS  += -std=c++11 -lgnu_shared
LOCAL_CPPFLAGS  += -std=c++11 -DLTC_OMAC -DLTC_CTR_MODE -DLTC_CBC_MODE -DRIJNDAEL -DCCM_MODE -DSHA256 -DFORTUNA
LOCAL_CFLAGS  += -std=c++11 -DLTC_OMAC -DLTC_CTR_MODE -DLTC_CBC_MODE -DRIJNDAEL -DCCM_MODE -DSHA256 -DFORTUNA -lgnu_shared

LOCAL_SRC_FILES := \
src/ciphers/aes/aes.c src/ciphers/anubis.c src/ciphers/blowfish.c \
src/ciphers/cast5.c src/ciphers/des.c src/ciphers/kasumi.c src/ciphers/khazad.c src/ciphers/kseed.c \
src/ciphers/noekeon.c src/ciphers/rc2.c src/ciphers/rc5.c src/ciphers/rc6.c src/ciphers/safer/safer.c \
src/ciphers/safer/safer_tab.c src/ciphers/safer/saferp.c src/ciphers/skipjack.c \
src/ciphers/twofish/twofish.c src/ciphers/xtea.c src/encauth/ccm/ccm_memory.c \
src/encauth/ccm/ccm_test.c src/encauth/eax/eax_addheader.c src/encauth/eax/eax_decrypt.c \
src/encauth/eax/eax_decrypt_verify_memory.c src/encauth/eax/eax_done.c src/encauth/eax/eax_encrypt.c \
src/encauth/eax/eax_encrypt_authenticate_memory.c src/encauth/eax/eax_init.c \
src/encauth/eax/eax_test.c src/encauth/gcm/gcm_add_aad.c src/encauth/gcm/gcm_add_iv.c \
src/encauth/gcm/gcm_done.c src/encauth/gcm/gcm_gf_mult.c src/encauth/gcm/gcm_init.c \
src/encauth/gcm/gcm_memory.c src/encauth/gcm/gcm_mult_h.c src/encauth/gcm/gcm_process.c \
src/encauth/gcm/gcm_reset.c src/encauth/gcm/gcm_test.c src/encauth/ocb/ocb_decrypt.c \
src/encauth/ocb/ocb_decrypt_verify_memory.c src/encauth/ocb/ocb_done_decrypt.c \
src/encauth/ocb/ocb_done_encrypt.c src/encauth/ocb/ocb_encrypt.c \
src/encauth/ocb/ocb_encrypt_authenticate_memory.c src/encauth/ocb/ocb_init.c src/encauth/ocb/ocb_ntz.c \
src/encauth/ocb/ocb_shift_xor.c src/encauth/ocb/ocb_test.c src/encauth/ocb/s_ocb_done.c \
src/hashes/chc/chc.c src/hashes/helper/hash_file.c src/hashes/helper/hash_filehandle.c \
src/hashes/helper/hash_memory.c src/hashes/helper/hash_memory_multi.c src/hashes/md2.c src/hashes/md4.c \
src/hashes/md5.c src/hashes/rmd128.c src/hashes/rmd160.c src/hashes/rmd256.c src/hashes/rmd320.c \
src/hashes/sha1.c src/hashes/sha2/sha256.c src/hashes/sha2/sha512.c src/hashes/tiger.c \
src/hashes/whirl/whirl.c src/mac/f9/f9_done.c src/mac/f9/f9_file.c src/mac/f9/f9_init.c \
src/mac/f9/f9_memory.c src/mac/f9/f9_memory_multi.c src/mac/f9/f9_process.c src/mac/f9/f9_test.c \
src/mac/hmac/hmac_done.c src/mac/hmac/hmac_file.c src/mac/hmac/hmac_init.c src/mac/hmac/hmac_memory.c \
src/mac/hmac/hmac_memory_multi.c src/mac/hmac/hmac_process.c src/mac/hmac/hmac_test.c \
src/mac/omac/omac_done.c src/mac/omac/omac_file.c src/mac/omac/omac_init.c src/mac/omac/omac_memory.c \
src/mac/omac/omac_memory_multi.c src/mac/omac/omac_process.c src/mac/omac/omac_test.c \
src/mac/pelican/pelican.c src/mac/pelican/pelican_memory.c src/mac/pelican/pelican_test.c \
src/mac/pmac/pmac_done.c src/mac/pmac/pmac_file.c src/mac/pmac/pmac_init.c src/mac/pmac/pmac_memory.c \
src/mac/pmac/pmac_memory_multi.c src/mac/pmac/pmac_ntz.c src/mac/pmac/pmac_process.c \
src/mac/pmac/pmac_shift_xor.c src/mac/pmac/pmac_test.c src/mac/xcbc/xcbc_done.c \
src/mac/xcbc/xcbc_file.c src/mac/xcbc/xcbc_init.c src/mac/xcbc/xcbc_memory.c \
src/mac/xcbc/xcbc_memory_multi.c src/mac/xcbc/xcbc_process.c src/mac/xcbc/xcbc_test.c \
src/math/fp/ltc_ecc_fp_mulmod.c src/math/gmp_desc.c src/math/ltm_desc.c src/math/multi.c \
src/math/rand_prime.c src/math/tfm_desc.c src/misc/base64/base64_decode.c \
src/misc/base64/base64_encode.c src/misc/burn_stack.c src/misc/crypt/crypt.c \
src/misc/crypt/crypt_argchk.c src/misc/crypt/crypt_cipher_descriptor.c \
src/misc/crypt/crypt_cipher_is_valid.c src/misc/crypt/crypt_find_cipher.c \
src/misc/crypt/crypt_find_cipher_any.c src/misc/crypt/crypt_find_cipher_id.c \
src/misc/crypt/crypt_find_hash.c src/misc/crypt/crypt_find_hash_any.c \
src/misc/crypt/crypt_find_hash_id.c src/misc/crypt/crypt_find_hash_oid.c \
src/misc/crypt/crypt_find_prng.c src/misc/crypt/crypt_fsa.c src/misc/crypt/crypt_hash_descriptor.c \
src/misc/crypt/crypt_hash_is_valid.c src/misc/crypt/crypt_ltc_mp_descriptor.c \
src/misc/crypt/crypt_prng_descriptor.c src/misc/crypt/crypt_prng_is_valid.c \
src/misc/crypt/crypt_register_cipher.c src/misc/crypt/crypt_register_hash.c \
src/misc/crypt/crypt_register_prng.c src/misc/crypt/crypt_unregister_cipher.c \
src/misc/crypt/crypt_unregister_hash.c src/misc/crypt/crypt_unregister_prng.c \
src/misc/error_to_string.c src/misc/pkcs5/pkcs_5_1.c src/misc/pkcs5/pkcs_5_2.c src/misc/zeromem.c \
src/modes/cbc/cbc_decrypt.c src/modes/cbc/cbc_done.c src/modes/cbc/cbc_encrypt.c \
src/modes/cbc/cbc_getiv.c src/modes/cbc/cbc_setiv.c src/modes/cbc/cbc_start.c \
src/modes/cfb/cfb_decrypt.c src/modes/cfb/cfb_done.c src/modes/cfb/cfb_encrypt.c \
src/modes/cfb/cfb_getiv.c src/modes/cfb/cfb_setiv.c src/modes/cfb/cfb_start.c \
src/modes/ctr/ctr_decrypt.c src/modes/ctr/ctr_done.c src/modes/ctr/ctr_encrypt.c \
src/modes/ctr/ctr_getiv.c src/modes/ctr/ctr_setiv.c src/modes/ctr/ctr_start.c src/modes/ctr/ctr_test.c \
src/modes/ecb/ecb_decrypt.c src/modes/ecb/ecb_done.c src/modes/ecb/ecb_encrypt.c \
src/modes/ecb/ecb_start.c src/modes/f8/f8_decrypt.c src/modes/f8/f8_done.c src/modes/f8/f8_encrypt.c \
src/modes/f8/f8_getiv.c src/modes/f8/f8_setiv.c src/modes/f8/f8_start.c src/modes/f8/f8_test_mode.c \
src/modes/lrw/lrw_decrypt.c src/modes/lrw/lrw_done.c src/modes/lrw/lrw_encrypt.c \
src/modes/lrw/lrw_getiv.c src/modes/lrw/lrw_process.c src/modes/lrw/lrw_setiv.c \
src/modes/lrw/lrw_start.c src/modes/lrw/lrw_test.c src/modes/ofb/ofb_decrypt.c src/modes/ofb/ofb_done.c \
src/modes/ofb/ofb_encrypt.c src/modes/ofb/ofb_getiv.c src/modes/ofb/ofb_setiv.c \
src/modes/ofb/ofb_start.c \
src/prngs/fortuna.c \
src/prngs/rc4.c \
src/prngs/rng_get_bytes.c \
src/prngs/rng_make_prng.c \
src/prngs/sprng.c \
src/prngs/yarrow.c


include $(BUILD_STATIC_LIBRARY)

#########################################################
#zmq
include $(CLEAR_VARS)

LOCAL_MODULE          := zmq
LOCAL_MODULE_FILENAME := zmq_static
LOCAL_SRC_FILES := libzmq.a

include $(PREBUILT_STATIC_LIBRARY)
#############################################
# SEU-DSSE
include $(CLEAR_VARS)

APP_STL         := gnustl_shared
APP_MODULES     := dsse_client gnustd_shared

# My ass... LOCAL_EXPORT_C_INCLUDES is useless
LOCAL_C_INCLUDES   := $(GNUPORT_INCL) $(TOM_INCL) $(ZMQ_INCL) $(LOCAL_PATH) $(ZMQPP_INCL) $(ZMQ3_INCL)
LOCAL_C_INCLUDES   += $(LOCAL_PATH)/SEU-DSSE/ $(LOCAL_PATH)/src/headers/

LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_CPP_FLAGS    := -Wl,--exclude-libs,ALL -DLTC_OMAC

LOCAL_LDLIBS            := -llog -landroid -lgnustl_shared -L$(LOCAL_PATH)/obj/local/$(TARGET_ARCH_ABI)/tomcrypt
#LOCAL_SHARED_LIBRARIES  := stlport_shared
LOCAL_SHARED_LIBRARIES  := gnustl_shared
LOCAL_STATIC_LIBRARIES  := zmq tomcrypt
LOCAL_LDLIBS      += -L/scratch/android/android-ndk/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH_ABI) -L/home/daniellin/Documents/SEU-DSSE_ANDROID/jni
LOCAL_WHOLE_STATIC_LIBRARIES :=  zmq

LOCAL_MODULE    := dsse_client
LOCAL_SRC_FILES := SEU-DSSE/Client_DSSE.cpp \
SEU-DSSE/DSSE.cpp \
SEU-DSSE/DSSE_Crypto.cpp \
SEU-DSSE/DSSE_FileCrypt.cpp \
SEU-DSSE/DSSE_Hashmap_Key_Class.cpp \
SEU-DSSE/DSSE_KeyGen.cpp \
SEU-DSSE/DSSE_Trapdoor.cpp \
SEU-DSSE/Keyword_Extraction.cpp \
SEU-DSSE/Krawczyk128_KDF.cpp \
SEU-DSSE/main_client.cpp \
SEU-DSSE/MasterKey.cpp \
SEU-DSSE/Miscellaneous.cpp \
SEU-DSSE/struct_thread_precompute_aeskey.cpp\
SEU-DSSE/struct_thread_getData.cpp\
LOCAL_CPPFLAGS  += -std=c++11 -DLTC_OMAC -DLTC_CTR_MODE -DLTC_CBC_MODE -DRIJNDAEL -DCCM_MODE -DSHA256 -DFORTUNA
LOCAL_CPPFLAGS  += -std=c++11

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
include $(BUILD_EXECUTABLE)
