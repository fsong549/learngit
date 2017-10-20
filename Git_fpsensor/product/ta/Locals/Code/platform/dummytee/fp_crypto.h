#ifndef __FP_CRYPTO_H__
#define __FP_CRYPTO_H__

#include <stdint.h>
#include <stddef.h>

uint32_t fp_get_wrapped_size(uint32_t data_size);
int32_t fp_wrap_crypto(uint8_t *data, uint32_t data_size, uint8_t *enc_data,
                       uint32_t *enc_data_size);
int32_t fp_unwrap_crypto(uint8_t *enc_data, uint32_t enc_data_size, uint8_t  **data,
                         uint32_t *data_size);

#endif // __FP_CRYPTO_H__

