/**
 * @file ffDecoder.c
 *
 * @see
 */

/** @addtogroup ffDecoder
 *  @{
 */

#include <ffDecoder.h>


#define CIRCULAR_BUF(x) (context->buffer[(context->head + x) % context->bufferSize])


/**
 * \brief Инит структуры декодирования
 * \param context Указатель на структуру данных
 * \param payloadProcess Указатель на функцию возврата данных
 * \return Результат
 */
bool ffDecoder_Init(ffDecoder_context * context, void (*payloadProcess)(ffDecoder_callbackStruct*)){
	if (!payloadProcess){
        return false;
	}
	context->bufferSize = FFDECODER_BUFF_SIZE;
	context->len = 0;
	context->head = 0;
	context->packetState = FF_STATE_BEGIN;
	context->packetLength = 0;
	context->packetPos = 0;
	context->payloadProcess = payloadProcess;
	return false;
}

/**
 * \brief Деинит структуры данных
 * \param context Указатель на структуру данных
 */
void ffDecoder_DeInit(ffDecoder_context * context){
	context->len = 0;
	context->head = 0;
}

/**
 * \brief Добавить байт в струтуру декодирования данных
 * \param context Указатель на структуру данных
 * \param b Значение
 */
void ffDecoder_pushByte(ffDecoder_context * context, uint8_t b){
	if (context->len >= context->bufferSize){
		return;
	}
	CIRCULAR_BUF(context->len) = b;
	context->len++;
}

/**
 * \brief Удалить раскодированные данные
 * \param context Указатель на структуру данных
 * \param l Количество байт
 */
static void ff_cutRxBufferHead(ffDecoder_context * context, uint16_t l){
    if (!l){
        return;
    }
//    HAL_NVIC_DisableIRQ(USART6_IRQn);
    __disable_irq ();
    context->len -= l;
    context->head += l;
    context->head %= context->bufferSize;
    __enable_irq ();
//    HAL_NVIC_EnableIRQ(USART6_IRQn);
}

/**
 * \brief Основной процес декодирования
 * \param context Указатель на структуру данных
 * \return Результат
 */
bool ffDecoder_process(ffDecoder_context * context){
	uint16_t i;
	uint8_t b;
    uint16_t cutlen = 0;

    uint16_t startPos = 0;
    if (context->packetState != FF_STATE_BEGIN){
        startPos = context->packetPos + 1;
    }
    for (i = startPos; i < context->len; i++){
        b = CIRCULAR_BUF(i);
        if (b == 0xff){
        	// if found 0xff byte reset ff processing.
            context->packetState = FF_STATE_VERSION;
            context->packetLength = 0;
            context->packetFFPos = 0;
            context->packetPos = 0;
            context->packetSum = 0xff;
            cutlen = i;
        } else {
            if (context->packetState == FF_STATE_BEGIN){
            	cutlen = i;
                continue;
            }
            context->packetPos++;
            if ((context->packetState == FF_STATE_DATA || context->packetState == FF_STATE_SUM) && context->packetFFPos && context->packetPos == context->packetFFPos){
                if (b > context->packetLength - context->packetPos){
                	context->packetLength = 0;
                	cutlen = i;
                	context->packetState = FF_STATE_BEGIN;
                	continue;
                }
                uint8_t tpos = b;
                b = 0xff;
                CIRCULAR_BUF(i) = 0xff;
                context->packetFFPos = tpos + context->packetPos;
            }
            if (context->packetState == FF_STATE_DATA && context->packetPos == context->packetLength - 1){
            	context->packetState = FF_STATE_SUM;
            } else if (context->packetState != FF_STATE_FFPOS){
            	context->packetSum += b;
            }
            switch (context->packetState){
                case FF_STATE_BEGIN:
                    break;
                case FF_STATE_VERSION:
                    if (b != 0x01){
//                    	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                    	context->packetState = FF_STATE_BEGIN;
                    	cutlen = i;
//                    	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
                        break;
                    }
                    context->packetState = FF_STATE_LENGTH;
                    break;
                case FF_STATE_LENGTH:
                    if (b < 5 || b > 255){
                    	context->packetLength = 0;
                    	context->packetState = FF_STATE_BEGIN;
                    	cutlen = i;
                        break;
                    }
                    context->packetLength = b;
                    context->packetState = FF_STATE_FFPOS;
                    break;
                case FF_STATE_FFPOS:
                    if (b > context->packetLength - context->packetPos){
                    	context->packetLength = 0;
                    	context->packetState = FF_STATE_BEGIN;
                    	cutlen = i;
                        break;
                    }
                    context->packetFFPos = b + context->packetPos;
                    CIRCULAR_BUF(i) = 0;
                    context->packetState = FF_STATE_DATA;
                    break;
                case FF_STATE_DATA:
                    break;
                case FF_STATE_SUM:
                    if ((context->packetSum & 0xff) != b){
                    	context->packetLength = 0;
                    	context->packetState = FF_STATE_BEGIN;
                    	cutlen = i;
                        break;
                    }
                    ffDecoder_callbackStruct payload;
                    payload.len = context->packetLength - 5;
                    payload.packetPayload = (i - context->packetLength + 5 + context->head) % context->bufferSize;
                    payload.buffer = context->buffer;
                    payload.bufferSize = context->bufferSize;
                    context->payloadProcess(&payload);
                    ff_cutRxBufferHead(context, i);
                    context->packetLength = 0;
                    context->packetState = FF_STATE_BEGIN;
                    return true;
            }
        }
    }
    ff_cutRxBufferHead(context, cutlen);
    return false;
}

/** @} */
