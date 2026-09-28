#include "se050_driver.h"
#include <stdio.h>
#include "nxScp03_Types.h"

/* Global SSS Context Structures */
static sss_connect_ctx_t g_connect_ctx;
static sss_session_t     g_session_ctx;
static sss_key_store_t   g_host_ks;

bool SE050_Init(void) 
{
    sss_status_t status;

    /* 1. เชื่อมต่อกับ SE050 ผ่าน I2C (T=1 over I2C Protocol) */
    status = sss_session_open(&g_session_ctx, kType_SSS_SE_SE05x, 0, kSSS_ConnectionType_Encrypted, &g_connect_ctx);
    if (status != kStatus_SSS_Success) {
        return false;
    }

    /* 2. Key Store Init */
    status = sss_key_store_context_init(&g_host_ks, &g_session_ctx);
    if (status != kStatus_SSS_Success) {
        sss_session_close(&g_session_ctx);
        return false;
    }

    return true;
}

bool SE050_GenerateKeyPair_Secp256k1(uint32_t key_id)
{
    sss_status_t status;
    sss_object_t key_obj;

    status = sss_key_object_init(&key_obj, &g_host_ks);
    if (status != kStatus_SSS_Success) return false;

    status = sss_key_object_allocate_handle(&key_obj,
                                            key_id,
                                            kSSS_KeyPart_Pair,
                                            kSSS_CipherType_EC_NIST_P,
                                            256,
                                            kKeyObject_Mode_Persistent);
    if (status != kStatus_SSS_Success) return false;

    /* สั่ง SE050 ให้สุ่มสร้าง Key Pair แบบ Hardware-based TRNG */
    status = sss_key_store_generate_key(&g_host_ks, &key_obj, 256, NULL);
    return (status == kStatus_SSS_Success);
}

bool SE050_SignDigest(uint32_t key_id, const uint8_t *digest, size_t digest_len, uint8_t *signature, size_t *sig_len)
{
    sss_status_t status;
    sss_object_t key_obj;
    sss_asymmetric_t asymmetric_ctx;

    status = sss_key_object_init(&key_obj, &g_host_ks);
    if (status != kStatus_SSS_Success) return false;

    status = sss_key_object_get_handle(&key_obj, key_id);
    if (status != kStatus_SSS_Success) return false;

    /* เซ็ตอัลกอริทึม ECDSA SHA256 */
    status = sss_asymmetric_context_init(&asymmetric_ctx,
                                         &g_session_ctx,
                                         &key_obj,
                                         kAlgorithm_SSS_SHA256,
                                         kMode_SSS_Sign);
    if (status != kStatus_SSS_Success) return false;

    /* สั่ง Sign ข้อมูล Transaction Digest ปลอดภัยภายในตัว Secure Element */
    status = sss_asymmetric_sign_digest(&asymmetric_ctx, (uint8_t *)digest, digest_len, signature, sig_len);
    
    sss_asymmetric_context_free(&asymmetric_ctx);
    return (status == kStatus_SSS_Success);
}