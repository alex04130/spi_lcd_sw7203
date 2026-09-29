
#include <bq4050.h>
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

i2c_master_dev_handle_t bq4050_I2C_handle;

void bq_Init(i2c_master_bus_handle_t I2C_bus_handle, uint8_t I2C_address)
{
    BQ4050_muTex = 0;
    i2c_device_config_t bq4050_i2c_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_address,
        .scl_speed_hz = 100000};
    ESP_ERROR_CHECK(i2c_master_bus_add_device(I2C_bus_handle, &bq4050_i2c_dev_config, &bq4050_I2C_handle));
}

esp_err_t bq4050_register_read(uint8_t reg_addr, uint8_t *data)
{
    esp_err_t errcode;
    while (BQ4050_muTex != 0)
    {
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    BQ4050_muTex = 1;
    errcode = i2c_master_transmit_receive(bq4050_I2C_handle, &reg_addr, 1, data, 1, -1);
    BQ4050_muTex = 0;
    return errcode;
}

uint8_t bq_GetRSOC()
{ // Unit: %
    uint8_t battBuf;
    ESP_ERROR_CHECK_WITHOUT_ABORT(bq4050_register_read(0x0D, &battBuf));
    return battBuf;
}