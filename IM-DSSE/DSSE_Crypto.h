#include <tomcrypt.h>
#include "config.h"
#if defined (INTEL_AES_NI)
#include <iaes_asm_interface.h>
#include <iaesni.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef __SIZE_TYPE__ size_t;
#define BLOCK_CIPHER_SIZE 16


int ccm_128_enc_dec(int cipher, const unsigned char *key, unsigned long keylen, symmetric_key *uskey, const unsigned char *nonce,
                    unsigned long noncelen, const unsigned char *header, unsigned long headerlen, unsigned char *pt, unsigned long ptlen,
                    unsigned char *ct, unsigned char *tag, unsigned long *taglen, int  direction);


//Intel AES NI functions
int aes128_ctr_encdec( unsigned char *pt, unsigned char *ct,  unsigned char *key,  unsigned char *ctr,  size_t numBlocks);

int omac_aes128(unsigned char *omac_out, int omac_length, const unsigned char *data, int datalen, unsigned char *key);

#if defined(INTEL_AES_NI)
int omac_aesni_init(omac_state *omac, int cipher, unsigned char *key);
int omac_aesni_process(omac_state *omac, unsigned char *key, const unsigned char *in, unsigned long inlen);
int omac_aesni_done(omac_state *omac, unsigned char *key, unsigned char *out, int outlen);
#endif

// PRNG using Fortuna

int invokeFortuna_prng(unsigned char *seed, unsigned char *key, int seedlen, int keylen);

#ifdef __cplusplus
}
#endif
