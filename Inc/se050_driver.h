#ifndef SE050_DRIVER_H
#define SE050_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fsl_sss_api.h"

/* Key Object IDs สำหรับเก็บใน Secure Element */
#define SE050_KEY_ID_SECP256K1   0xF0000001  /* สำหรับ Bitcoin / Ethereum */
#define SE050_KEY_ID_ED25519     0xF0000002  /* สำหรับ Solana / Card/EdDSA */

/* Function Prototypes */
bool SE050_Init(void);
bool SE050_GenerateKeyPair_Secp256k1(uint32_t key_id);
bool SE050_SignDigest(uint32_t key_id, const uint8_t *digest, size_t digest_len, uint8_t *signature, size_t *sig_len);

#endif /* SE050_DRIVER_H */