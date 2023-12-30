/*
 * sensor.h
 *
 *  Created on: Sep 28, 2023
 *      Author: User
 */

#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_

#include "main.h"

void sensor_init();
void sensor_deinit();
void sensor_exec();

void sensor_sendSensorsDataOne(uint8_t cmd);
bool sensor_sendSensorsDataAlways(uint8_t state);

bool sensor_addSensorInTableBond(uint8_t n, uint32_t SN, uint32_t type);
bool sensor_delSensorInTableBond(uint8_t n);
void sensor_sendTableBond();
void sensor_setSensorData(uint8_t n, uint8_t signalLevel, uint8_t *buff, uint8_t count);
void sensor_searchSensorsStart();
void sensor_searchSensorsStop();

#endif /* INC_SENSOR_H_ */
