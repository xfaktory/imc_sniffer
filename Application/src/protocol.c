/**
 * @file protocol.c
 *
 * @see
 */

/** @addtogroup protocol
 *  @{
 */

#include "protocol.h"
#include "pcUart.h"
#include "sensor.h"

/**
 * \brief Состояния результата
 */
typedef enum{
	PROTOCOL_RESULT_COMM_OK = (uint8_t)0,	//OK
	PROTOCOL_RESULT_COMM_NOT_COMM,			//Команда отсутствует
	PROTOCOL_RESULT_COMM_ERROR_COMM,		//Ошибка выполнения команды
	PROTOCOL_RESULT_COMM_ERROR_DATA,		//Ошибка данных (некорректные данные)
	PROTOCOL_RESULT_COMM_WAIT,				//Не готов (ожидание)
}protocol_resultComm_t;

/**
 * \brief Функция заполнения стандартного ответа (результат команды)
 * \param comm Команда
 * \param state Состояние
 */
static void protocol_sendAnswerStandard(uint8_t comm, protocol_resultComm_t state){
	uint8_t buff[3] = {0};
	uint8_t len = 0;

	buff[len++] = PROTOCOL_ANSWER_BIT;
	buff[len++] = comm;
	buff[len++] = state;

	pcUart_pushSendBuff(buff, len);
}

/**
 * \brief Функция разбора пакета согласно протоколу
 * \param buff буфер данных
 * \param len количество данных
 * \return Результат
 */
bool protocol_pushComm(uint8_t *buff, uint8_t len){
	if((!buff) || (len < 1)){
		return false;
	}
	uint8_t comm = 0;
	uint8_t pos = 0;

	comm = buff[pos++];

	switch (comm) {
		case PROTOCOL_COMM_READ_TABLE:
			if(len != 1){
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_DATA);
				break;
			}
			protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_OK);
			sensor_sendTableBond();
			break;
		case PROTOCOL_COMM_GET_SEARCH_SENSORS:
			if(len != 1){
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_DATA);
				break;
			}
			protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_OK);
			sensor_searchSensorsStart();
			break;
		case PROTOCOL_COMM_ADD_SENSOR_IN_TABLE:
			if(len != 10){
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_DATA);
				break;
			}
			uint32_t sn = ((uint32_t)buff[pos + 1] << 0) | ((uint32_t)buff[pos + 2] << 8)
						| ((uint32_t)buff[pos + 3] << 16) | ((uint32_t)buff[pos + 4] << 24);
			uint32_t type = ((uint32_t)buff[pos + 5] << 0) | ((uint32_t)buff[pos + 6] << 8)
						| ((uint32_t)buff[pos + 7] << 16) | ((uint32_t)buff[pos + 8] << 24);
			if(!sensor_addSensorInTableBond(buff[pos], sn, type)){
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_DATA);
			}else{
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_OK);
			}
			break;
		case PROTOCOL_COMM_DEL_SENSOR_IN_TABLE:
			if(len != 2){
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_DATA);
				break;
			}
			if(!sensor_delSensorInTableBond(buff[pos])){
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_DATA);
			}else{
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_OK);
			}
			break;
		case PROTOCOL_COMM_GET_DATA_SENSOR:
			if(len != 1){
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_DATA);
				break;
			}
			protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_OK);
			sensor_sendSensorsDataOne(comm);
			break;
		case PROTOCOL_COMM_START_STOP_SEND_DATA_SENSORS:
			if(len != 2){
				protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_DATA);
				break;
			}
			protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_OK);
			sensor_sendSensorsDataAlways(buff[pos]);
			break;
		default:
			protocol_sendAnswerStandard(comm, PROTOCOL_RESULT_COMM_ERROR_COMM);
			break;
	}
	return true;
}

/** @} */
