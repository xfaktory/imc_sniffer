/**
 * @file protocol.h
 *
 *
 * @brief модуль protocol
 *
 *
 *
 * @see
 */

/** @addtogroup protocol
 *
 * @brief модуль обработки пакетов согласно протоколу
 *
 *
 *  @{
 */

#ifndef INC_PROTOCOL_H_
#define INC_PROTOCOL_H_

#include "main.h"

#define PROTOCOL_ANSWER_BIT							(uint8_t)(1 << 7)	//Бит ответа

#define PROTOCOL_COMM_READ_TABLE					(uint8_t)(0x01)		//Прочитать таблицу связей
#define PROTOCOL_COMM_GET_SEARCH_SENSORS			(uint8_t)(0x02)		//Получить список датчиков
#define PROTOCOL_COMM_ADD_SENSOR_IN_TABLE			(uint8_t)(0x03)		//Внести датчик в таблицу связей
#define PROTOCOL_COMM_DEL_SENSOR_IN_TABLE			(uint8_t)(0x04)		//Удалить датчик из таблицы связей
#define PROTOCOL_COMM_GET_DATA_SENSOR				(uint8_t)(0x05)		//Прочитать данные из датчиков однократно
#define PROTOCOL_COMM_START_STOP_SEND_DATA_SENSORS	(uint8_t)(0x10)		//Старт/Стоп получение данных с датчиков

bool protocol_pushComm(uint8_t *buff, uint8_t len);

#endif /* INC_PROTOCOL_H_ */
