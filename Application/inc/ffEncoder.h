/**
 * @file ffEncoder.h
 *
 *
 * @brief модуль ffEncoder
 *
 *
 *
 * @see
 */

/** @addtogroup ffEncoder
 *
 * @brief модуль кодирования пакета данных
 *
 *
 *  @{
 */

#ifndef MODULES_FFENCODER_H_
#define MODULES_FFENCODER_H_

#include "main.h"

uint8_t ffEncoder_encode(uint8_t *src, uint8_t len, uint8_t *dst);

#endif /* MODULES_FFENCODER_H_ */

/** @} */
