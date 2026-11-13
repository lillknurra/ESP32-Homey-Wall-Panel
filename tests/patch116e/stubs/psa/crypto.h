#pragma once
#include <stddef.h>
#include <stdint.h>
typedef int32_t psa_status_t;
#define PSA_SUCCESS ((psa_status_t)0)
#define PSA_ERROR_BAD_STATE ((psa_status_t)-137)
#define PSA_ALG_SHA_256 UINT32_C(0x02000009)
typedef struct { uint32_t hash[8]; uint64_t count; } psa_hash_operation_t;
#define PSA_HASH_OPERATION_INIT { {0}, 0 }
static inline psa_hash_operation_t psa_hash_operation_init(void)
{ return (psa_hash_operation_t)PSA_HASH_OPERATION_INIT; }
static inline psa_status_t psa_crypto_init(void) { return PSA_SUCCESS; }
static inline psa_status_t psa_hash_setup(psa_hash_operation_t *op, uint32_t alg)
{
    if (op == NULL || alg != PSA_ALG_SHA_256) return PSA_ERROR_BAD_STATE;
    for (size_t i = 0; i < 8U; ++i) op->hash[i] = UINT32_C(2166136261) ^ (uint32_t)i;
    op->count = 1U;
    return PSA_SUCCESS;
}
static inline psa_status_t psa_hash_update(psa_hash_operation_t *op,
                                            const uint8_t *data, size_t len)
{
    if (op == NULL || op->count == 0U || (data == NULL && len != 0U))
        return PSA_ERROR_BAD_STATE;
    for (size_t i = 0; i < len; ++i) {
        for (size_t j = 0; j < 8U; ++j) {
            op->hash[j] ^= (uint32_t)data[i] + (uint32_t)((op->count + i) & 0xffU);
            op->hash[j] *= UINT32_C(16777619) + (uint32_t)(2U * j);
        }
    }
    op->count += len;
    return PSA_SUCCESS;
}
static inline psa_status_t psa_hash_finish(psa_hash_operation_t *op, uint8_t *out,
                                             size_t cap, size_t *len)
{
    if (op == NULL || out == NULL || len == NULL || op->count == 0U || cap < 32U)
        return PSA_ERROR_BAD_STATE;
    for (size_t i = 0; i < 8U; ++i) {
        out[4U*i] = (uint8_t)(op->hash[i] >> 24);
        out[4U*i+1U] = (uint8_t)(op->hash[i] >> 16);
        out[4U*i+2U] = (uint8_t)(op->hash[i] >> 8);
        out[4U*i+3U] = (uint8_t)op->hash[i];
    }
    *len = 32U;
    op->count = 0U;
    return PSA_SUCCESS;
}
static inline psa_status_t psa_hash_abort(psa_hash_operation_t *op)
{
    if (op == NULL) return PSA_ERROR_BAD_STATE;
    op->count = 0U;
    return PSA_SUCCESS;
}
