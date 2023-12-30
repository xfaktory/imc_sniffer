/**
 * @file ffEncoder.c
 *
 * @see
 */

/** @addtogroup ffEncoder
 *  @{
 */

#include "ffEncoder.h"
//#include "utils.h"
#include "string.h"

/**
 * \brief Функция кодирования пакета данных
 * \param src Указатель на входной буфер данных
 * \param len Количество байт данных в входном буфере
 * \param dst Указатель на выходной буфер данных
 * \return Количество байт данных в выходном буфере
 */
uint8_t ffEncoder_encode(uint8_t *src, uint8_t len, uint8_t *dst){
	uint8_t i;
	uint8_t ffIndex = 3;
	uint8_t sum = 0;
	dst[0] = 0xff;
	dst[1] = 0x01;
	dst[2] = len + 5;
	dst[3] = 0;
//	memcopy(dst + 4, src, len);
	memcpy(dst + 4, src, len);
	len += 4;
    for (i = 0; i < len; i++){
        sum += dst[i];
    }
	dst[len] = sum;
	len ++;
	for (i = ffIndex + 1; i < len; i++){
		if (dst[i] == 0xff){
			dst[ffIndex] = i - ffIndex;
			ffIndex = i;
		}
	}
	dst[ffIndex] = 0;
	return len;
}

/** @} */
