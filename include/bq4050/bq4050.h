#ifndef __BQ4050_H__
#define __BQ4050_H__

#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

extern uint8_t BQ4050_muTex;

void bq_Init(i2c_master_bus_handle_t I2C_bus_handle, uint8_t I2C_address);
uint8_t bq_GetRSOC(); // Unit: %

#endif