/**
 * @file ffDecoder.h
 *
 *
 * @brief модуль ffDecoder
 *
 *
 *
 * @see
 */

/** @addtogroup ffDecoder
 *
 * @brief модуль декодирования пакетов
 *
 *
 *  @{
 */

#ifndef MODULES_FFDECODER_H_
#define MODULES_FFDECODER_H_

#include "main.h"


#define CIRCULAR_BUF_EXT(x, h, s, b) (b[(h + x) % s])
#define FFDECODER_BUFF_SIZE	(255 * 2)

/**
 * \brief Состояния декодирования
 */
typedef enum {
    FF_STATE_BEGIN = (uint8_t)1,  /**< FF_STATE_BEGIN */
    FF_STATE_VERSION = (uint8_t)2,/**< FF_STATE_VERSION */
    FF_STATE_LENGTH = (uint8_t)3, /**< FF_STATE_LENGTH */
    FF_STATE_FFPOS = (uint8_t)4,  /**< FF_STATE_FFPOS */
    FF_STATE_DATA = (uint8_t)5,   /**< FF_STATE_DATA */
    FF_STATE_SUM = (uint8_t)6,    /**< FF_STATE_SUM */
} ffDecoder_stateEnum;

/**
 * \brief Структура декодированного пакета
 */
typedef struct {
    uint16_t packetPayload;
    uint8_t* buffer;
    uint16_t bufferSize;
    uint8_t len;
} ffDecoder_callbackStruct;

/**
 * \brief Структура декодирования
 */
typedef struct {
	uint8_t buffer[FFDECODER_BUFF_SIZE]; // buffer for ff decode processing
	uint16_t bufferSize; // size of buffer
	uint16_t head; // cyclic buffer head
	uint16_t len; // cyclic buffer length
	uint16_t packetLength; // length of packet
	uint16_t packetPos; // number of byte in packet
	uint16_t packetFFPos;
	uint8_t packetSum;
	ffDecoder_stateEnum packetState;
	void (*payloadProcess)(ffDecoder_callbackStruct*);
} ffDecoder_context;

bool ffDecoder_Init(ffDecoder_context * context, void (*payloadProcess)(ffDecoder_callbackStruct*));
void ffDecoder_DeInit(ffDecoder_context * context);
void ffDecoder_pushByte(ffDecoder_context * context, uint8_t b);
bool ffDecoder_process(ffDecoder_context * context);
#endif /* MODULES_FFDECODER_H_ */

/** @} */
