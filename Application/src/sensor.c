/*
 * sensor.c
 *
 *  Created on: Sep 28, 2023
 *      Author: User
 */


#include "sensor.h"
#include "counterModule.h"
#include "protocol.h"
#include "stddef.h"
#include "pcUart.h"
#include "string.h"

static void TEST_exec();
static void TEST_sendSearchSensor();

#define DEVICE_ID	(uint32_t)(0xAABBCCDD)

#define SENSOR_MAX_COUNT			(uint8_t)(10)
#define SENSOR_SEND_DATA_TIM		(uint16_t)(100)	//ms

//#define SENSOR_TABLE_BIND_COUNT		(uint8_t)(10)

typedef enum{
	SENSOR_SEARCH_STATE_NONE = (uint8_t)0,
	SENSOR_SEARCH_STATE_START,
	SENSOR_SEARCH_STATE_ACTION,
	SENSOR_SEARCH_STATE_STOP,
}sensor_searchState_t;

static sensor_searchState_t sensor_searchState = SENSOR_SEARCH_STATE_NONE;
static sensor_searchState_t sensor_searchStateNew = SENSOR_SEARCH_STATE_NONE;

typedef struct{
	uint8_t signalLevel;	//уровень сигнала датчика радиоканала
	uint8_t count;			//длина пакета данных датчика
	uint8_t buff[22];		//данне с датчика
	bool state;				//флаг обновления данных с датчика
}sensor_dataSensor_t;		//структура датчика

typedef struct{
	uint32_t serNumSensor;		//серийный номер датчик
	uint32_t typeSensor;		//тип датчик "EM08"
	uint8_t reserv_1;			//резерв
	uint8_t reserv_2;			//резерв
	bool state;					//флаг наличие датчика
	sensor_dataSensor_t sensor;	//данные с датчика
}sensor_tableBind_t;			//структура таблицы связей

static sensor_tableBind_t sensor_tableBind[SENSOR_MAX_COUNT] = {0};

static bool sensor_sendData_f = false;	//флаг непрерывной отправки данных
static uint64_t sensor_sendDataTim = 0;

static void sensor_resetSensors(){
	uint8_t i = 0;

	for(i = 0; i < SENSOR_MAX_COUNT; i++){
		sensor_tableBind[i].sensor.signalLevel = 0;
		sensor_tableBind[i].sensor.count = 0;
		sensor_tableBind[i].sensor.state = false;
	}
}

static void sensor_resetTableBond(){
	uint8_t i = 0;

	for(i = 0; i < SENSOR_MAX_COUNT; i++){
		sensor_tableBind[i].serNumSensor = 0;
		sensor_tableBind[i].typeSensor = 0;
		sensor_tableBind[i].reserv_1 = 0;
		sensor_tableBind[i].reserv_2 = 0;
		sensor_tableBind[i].state = false;
	}
}

static void sensor_sendPack_searchEnd(){
//	uint8_t buff[11] = {0};
//	uint8_t len = 0;

//	buff[len++] = 0x82;	//команда
//	buff[len++] = 0x20;	//тип датчика
//	buff[len++] = 0x20;
//	buff[len++] = 0x20;
//	buff[len++] = 0x20;
//	buff[len++] = 0;	//год изготовления
//	buff[len++] = 0;
//	buff[len++] = 0;	//серийный номер
//	buff[len++] = 0;
//	buff[len++] = 0;
//	buff[len++] = 0;

	uint8_t buff[11] = {0x82, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t len = 11;

	pcUart_pushSendBuff(buff, len);
}

static void sensor_searchExec(){
	switch(sensor_searchState){
		case SENSOR_SEARCH_STATE_NONE:	//wait
			if(sensor_searchStateNew == SENSOR_SEARCH_STATE_START){
				sensor_searchState = sensor_searchStateNew;
			}
			break;
		case SENSOR_SEARCH_STATE_START:	//start search
			sensor_searchStateNew = SENSOR_SEARCH_STATE_ACTION;
			sensor_searchState = sensor_searchStateNew;
#warning
			//отправить команду на старт поиска датчиков
			break;
		case SENSOR_SEARCH_STATE_ACTION:	//wait search
			if(sensor_searchStateNew == SENSOR_SEARCH_STATE_STOP){
				sensor_searchState = sensor_searchStateNew;
				break;
			}
			//отправить пакет данных, найденого датчика
#warning
			TEST_exec();
			break;
		case SENSOR_SEARCH_STATE_STOP:	//stop search
			//отправить последний пакет, означает окончание поиска
			sensor_sendPack_searchEnd();
			sensor_searchStateNew = SENSOR_SEARCH_STATE_NONE;
			sensor_searchState = sensor_searchStateNew;
			break;
		default:
			break;
	}
}

void sensor_init(){
	sensor_resetSensors();
	sensor_resetTableBond();
}

void sensor_deinit(){
	sensor_resetSensors();
	sensor_resetTableBond();
}

void sensor_exec(){
	if((sensor_sendData_f) && (sensor_sendDataTim <= Counter_GetValueCounter())){
		sensor_sendSensorsDataOne(PROTOCOL_COMM_START_STOP_SEND_DATA_SENSORS);
		sensor_sendDataTim = Counter_GetValueCounter() + Counter_GetSamplesForMilliseconds(SENSOR_SEND_DATA_TIM);
	}
	sensor_searchExec();
}

void sensor_sendSensorsDataOne(uint8_t cmd){	//отправить данные с датчиков один раз
	uint8_t i = 0;
	uint8_t len = 0;
	uint8_t buff[255] = {0};

	buff[len++] = cmd | PROTOCOL_ANSWER_BIT;

	len += 2;	//состояния
	for(i = 0; i < SENSOR_MAX_COUNT; i++){
		if(sensor_tableBind[i].state && sensor_tableBind[i].sensor.state){
			buff[len++] = sensor_tableBind[i].sensor.signalLevel;
			buff[len++] = sensor_tableBind[i].sensor.count;

			*(uint16_t*)&buff[1] |= (1 << i);	//состояние

			memcpy(&buff[len], sensor_tableBind[i].sensor.buff, sensor_tableBind[i].sensor.count);
			len += sensor_tableBind[i].sensor.count;
		}else{
			buff[len++] = 0;	//sensor_tableBind[i].sensor.signalLevel;
			buff[len++] = 0;	//sensor_tableBind[i].sensor.count;
		}
	}
	pcUart_pushSendBuff(buff, len);
	sensor_resetSensors();	//можно просто обнулять состояние: "sensor_tableBind[i].sensor.state = false"
}

bool sensor_sendSensorsDataAlways(uint8_t state){	//отправлять данные с датчиков постоянно
	sensor_sendData_f = (state == 0) ? false : true;
	sensor_sendDataTim = 0;
	return true;
}

bool sensor_addSensorInTableBond(uint8_t n, uint32_t SN, uint32_t type){	//добавить датчик в таблицу связей
	if((n == 0) || (n > SENSOR_MAX_COUNT)){
		return false;
	}
	n--;
	sensor_tableBind[n].serNumSensor = SN;
	sensor_tableBind[n].typeSensor = type;
	sensor_tableBind[n].reserv_1 = 0;
	sensor_tableBind[n].reserv_2 = 0;
	sensor_tableBind[n].state = true;

	sensor_tableBind[n].sensor.signalLevel = 0;
	sensor_tableBind[n].sensor.count = 0;
	sensor_tableBind[n].sensor.state = false;

	return true;
}

bool sensor_delSensorInTableBond(uint8_t n){	//удалить датчик из таблицы связей
	if((n == 0) || (n > SENSOR_MAX_COUNT)){
		return false;
	}
	n--;
	sensor_tableBind[n].serNumSensor = 0;
	sensor_tableBind[n].typeSensor = 0;
	sensor_tableBind[n].reserv_1 = 0;
	sensor_tableBind[n].reserv_2 = 0;
	sensor_tableBind[n].state = false;

	sensor_tableBind[n].sensor.signalLevel = 0;
	sensor_tableBind[n].sensor.count = 0;
	sensor_tableBind[n].sensor.state = false;

	return true;
}

void sensor_sendTableBond(){	//отправить таблицу связей датчиков
	uint8_t i = 0;
	uint8_t len = 0;
	uint8_t buff[105] = {0};	//10*10+5 = 105

	buff[len++] = PROTOCOL_COMM_READ_TABLE | PROTOCOL_ANSWER_BIT;
	buff[len++] = (DEVICE_ID >> 0) & 0xFF;
	buff[len++] = (DEVICE_ID >> 8) & 0xFF;
	buff[len++] = (DEVICE_ID >> 16) & 0xFF;
	buff[len++] = (DEVICE_ID >> 24) & 0xFF;

	for(i = 0; i < SENSOR_MAX_COUNT; i++){
		buff[len++] = (sensor_tableBind[i].serNumSensor >> 0) & 0xFF;
		buff[len++] = (sensor_tableBind[i].serNumSensor >> 8) & 0xFF;
		buff[len++] = (sensor_tableBind[i].serNumSensor >> 16) & 0xFF;
		buff[len++] = (sensor_tableBind[i].serNumSensor >> 24) & 0xFF;

		buff[len++] = (sensor_tableBind[i].typeSensor >> 0) & 0xFF;
		buff[len++] = (sensor_tableBind[i].typeSensor >> 8) & 0xFF;
		buff[len++] = (sensor_tableBind[i].typeSensor >> 16) & 0xFF;
		buff[len++] = (sensor_tableBind[i].typeSensor >> 24) & 0xFF;
		buff[len++] = sensor_tableBind[i].reserv_1;
		buff[len++] = sensor_tableBind[i].reserv_2;
	}
	pcUart_pushSendBuff(buff, len);
}

void sensor_setSensorData(uint8_t n, uint8_t signalLevel, uint8_t *buff, uint8_t count){	//записать новые данные с датчика
	if((n == 0) || (n > SENSOR_MAX_COUNT) || (!buff) || (count == 0) || (count > 22)){
		return;
	}
	n--;
	sensor_tableBind[n].sensor.signalLevel = signalLevel;
	sensor_tableBind[n].sensor.count = count;
	memcpy(sensor_tableBind[n].sensor.buff, buff, count);
	sensor_tableBind[n].sensor.state = true;
}

void sensor_searchSensorsStart(){
	sensor_searchStateNew = SENSOR_SEARCH_STATE_START;
}

void sensor_searchSensorsStop(){
	sensor_searchStateNew = SENSOR_SEARCH_STATE_STOP;
}







//---------------TEST------------------------
#warning УДАЛИТЬ
//---------------TEST------------------------

//static bool TEST_sendSearchSensor_f = false;

static void TEST_exec(){
	static uint64_t tim = 0;


	if(tim < Counter_GetValueCounter()){
		tim = Counter_GetValueCounter() + Counter_GetSamplesForMilliseconds(300);

		TEST_sendSearchSensor();
		sensor_tableBind[3].sensor.state = !sensor_tableBind[3].sensor.state;
	}
}

static void TEST_sendSearchSensor(){
	static uint8_t num = 0;
	uint8_t buff[11] = {0};
	uint8_t len = 0;

	buff[len++] = 0x82;		//команда

	switch(num){
		case 0:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		case 1:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		case 2:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = 0xFF;	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 0xFF;
			buff[len++] = 0xFF;
			buff[len++] = num;
			break;
		case 3:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		case 4:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		case 5:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		case 6:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		case 7:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		case 8:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		case 9:
			buff[len++] = 'E';	//тип датчика
			buff[len++] = 'M';
			buff[len++] = '0';
			buff[len++] = '8';
			buff[len++] = '2';	//год изготовления
			buff[len++] = '3';
			buff[len++] = 1;	//серийный номер
			buff[len++] = 2;
			buff[len++] = 3;
			buff[len++] = num;
			break;
		default:
			num = 0;
			break;
	}
	pcUart_pushSendBuff(buff, len);

	num++;
	if(num > 9){
		num = 0;
		sensor_searchSensorsStop();
	}
}
