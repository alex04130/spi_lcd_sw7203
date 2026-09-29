#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "hal/spi_types.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "FULLASCII.h"
#include "esp_task_wdt.h"
#include "driver/i2c.h"
#include "LCD_INIT.h"
#include "sw7203.h"
#include "bq4050.h"

#define TAG "main"

esp_err_t errcode;

#define I2C0_PORT 0
#define I2C0_SCL_IO 42
#define I2C0_SDA_IO 41

i2c_master_bus_config_t i2c0_mst_config_1 = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C0_PORT,
    .scl_io_num = I2C0_SCL_IO,
    .sda_io_num = I2C0_SDA_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};
i2c_master_bus_handle_t I2C0_bus_handle;

void SPIpreCB(spi_transaction_t *trans)
{
    if (trans == NULL)
    {
        ESP_LOGW(TAG, "SPI trans call back NULL");
        while (1)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
    LCD_type_DC_t dc = (LCD_type_DC_t)trans->user;

    gpio_set_level(PIN_NUM_LCD_DC, dc);
}

void SPIpostCB(spi_transaction_t *trans)
{
    if (trans == NULL)
    {
        ESP_LOGW(TAG, "SPI trans call back NULL");
        while (1)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

spi_device_handle_t spi2_handle;

esp_err_t lcd_send_bytes(const uint8_t *data, int len, LCD_type_DC_t DC)
{
    if (data == NULL)
    {
        ESP_LOGW(TAG, "LCD send NULL");
        while (1)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
    spi_transaction_ext_t ext = {
        .address_bits = 0,
        .command_bits = 0,
        .dummy_bits = 0,
        .base = {
            .length = len * 8,
            .tx_buffer = data,
            .user = (void *)DC,
            .flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY,
        },
    };
    return spi_device_transmit(spi2_handle, &ext.base);
}

void LCD_init()
{
    int i = 0;
    while (init_data[i].len != 0xff)
    {
        errcode = lcd_send_bytes(init_data[i].data, init_data[i].len, init_data[i].LCD_type_DC);
        if (errcode != ESP_OK)
        {
            ESP_LOGW(TAG, "LCD send bytes wrong");
            ESP_ERROR_CHECK_WITHOUT_ABORT(errcode);
            while (1)
            {
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
        if (init_data[i].delay_ms != 0)
        {
            vTaskDelay(init_data[i].delay_ms / portTICK_PERIOD_MS);
        }
        i++;
    }
}

typedef union
{
    uint8_t data[3];
    struct node
    {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } color;
} Color_t;
Color_t *buffer[2];
const Color_t whitecolor = {.data = {0xFF, 0xFF, 0xFF}};

static uint8_t send_buffer = -1, calc_buffer = 0;

static spi_transaction_t trans[6];

void transbuffer(int point_start_x, int point_start_y, int point_end_x, int point_end_y)
{
    trans[1].tx_data[0] = (point_start_x) >> 8;
    trans[1].tx_data[1] = (point_start_x) & 0xff;
    trans[1].tx_data[2] = (point_end_x) >> 8;
    trans[1].tx_data[3] = (point_end_x) & 0xff;
    trans[3].tx_data[0] = (point_start_y) >> 8;
    trans[3].tx_data[1] = (point_start_y) & 0xff;
    trans[3].tx_data[2] = (point_end_y) >> 8;
    trans[3].tx_data[3] = (point_end_y) & 0xff;
    trans[5].length = (point_end_x - point_start_x + 1) * (point_end_y - point_start_y + 1) * 24;
    trans[5].tx_buffer = buffer[send_buffer];
    trans[5].rxlength = 0;
    // ESP_LOGI(TAG, "Running x(%x:%x)", trans[1].tx_data[0], trans[1].tx_data[1]);
    // ESP_LOGI(TAG, "y(%x:%x)", trans[3].tx_data[0], trans[3].tx_data[1]);
    // ESP_LOGI(TAG, "Running x(%x:%x)", trans[1].tx_data[2], trans[1].tx_data[3]);
    // ESP_LOGI(TAG, "y(%x:%x)", trans[3].tx_data[2], trans[3].tx_data[3]);
    // ESP_LOGI(TAG, "color red(%d) green(%d) blue(%d)", (*(uint8_t *)(trans[5].tx_buffer)), *(uint8_t *)(trans[5].tx_buffer + 1), *(uint8_t *)(trans[5].tx_buffer + 2));
    for (int i = 0; i < 6; i++)
    {
        // ESP_LOGI(TAG, "Running before queue on %x", i);
        errcode = spi_device_queue_trans(spi2_handle, &trans[i], portMAX_DELAY);
        // ESP_LOGI(TAG, "Running after queue");
        if (errcode != ESP_OK)
        {
            ESP_LOGW(TAG, "LCD send bytes wrong");
            ESP_ERROR_CHECK_WITHOUT_ABORT(errcode);
            while (1)
            {
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
    }
}

static void Finish_transfer_buffer(spi_device_handle_t spi)
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    for (int x = 0; x < 6; x++)
    {
        ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "LCD send bytes wrong");
            ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
            while (1)
            {
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
    }
}

// static void drawret(int point_start_x, int point_start_y, int hight, int width, Color_t background_color, Color_t border_color, int thickness)
// {
//     for (int i = 0; i < width; i++)
//     {
//         buffer[calc_buffer][i] = border_color;
//     }
//     for (int i = 1; i < hight - 1; i++)
//     {
//         for (int j = 0; j < thickness; j++)
//         {
//             buffer[calc_buffer][i * width + j] = border_color;
//         }
//         for (int j = thickness; j < width - thickness; j++)
//         {
//             buffer[calc_buffer][i * width + j] = background_color;
//         }
//         for (int j = width - thickness; j < width; j++)
//         {
//             buffer[calc_buffer][i * width + j] = border_color;
//         }
//     }
//     for (int j = 0; j < width; j++)
//     {
//         buffer[calc_buffer][(hight - 1) * width + j] = border_color;
//     }
//     send_buffer = calc_buffer;
//     calc_buffer ^= 1;
//     transbuffer(point_start_x, point_start_y, point_start_x + width - 1, point_start_y + width - 1);
// }

void DecodeASCIILine(char *s, Color_t color, int point_start_x, int point_start_y)
{
    int strlength = 0;
    ESP_LOGI(TAG, "String: %s", s);
    while (s[strlength] != '\n' && s[strlength] != '\0')
    {
        strlength++;
    }
    ESP_LOGI(TAG, "String length: %d", strlength);
    for (int StringPointer = 0; StringPointer < strlength; StringPointer++)
    {
        for (int i = 0; i < 23; i++)
        {
            for (int j = 0; j < 16; j++)
            {
                buffer[calc_buffer][(22 - i) * strlength * 16 + StringPointer * 16 + j] = ((ascll_bitmap_16_23[(s[StringPointer] - ' ') * 23 + i][j / 8] & (1 << (7 - j % 8))) ? color : whitecolor);
            }
        }
    }
    send_buffer = calc_buffer;
    calc_buffer ^= 1;
    ESP_LOGI(TAG, "trans Start(%d,%d) End at(%d,%d)", point_start_x, point_start_y, point_start_x + 20 * strlength - 1, point_start_y - 22);
    transbuffer(point_start_x, point_start_y - 22, point_start_x + 16 * strlength - 1, point_start_y);
}

int int_to_string(int x, char *s)
{
    int i = 0;
    while (x != 0)
    {
        s[i] = '0' + x % 10;
        x /= 10;
        i++;
    }
    for (int j = 0; j < i / 2; j++)
    {
        s[j] += s[i - j - 1];
        s[i - j - 1] = s[j] - s[i - j - 1];
        s[j] -= s[i - j - 1];
    }
    return i;
}

sw7203_Vout_config_t sw7203output = {
    .VBATCurrentLimit_mA = 100,
    .VBATVoltageLimit_mV = 13200,
    .VBATVoltageLimitHysteresis_mV = 400,
    .VBUSCurrentLimit_mA = 500,
    .VBUSVoltage_mV = 5000,
};

Color_t black = {.color.blue = 0, .color.green = 0, .color.red = 0};

sw7203_Vout_config_t *sw7203cb(sw7203_I2C_port port)
{
    if (port == sw7203_port_A1)
    {
        DecodeASCIILine("A1 pluged in!", black, 10, 100);
    }
    else
    {
        DecodeASCIILine("A2 pluged in!", black, 10, 50);
    }
    return &sw7203output;
}

void sw7203init()
{
    BQ4050_muTex = 0;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c0_mst_config_1, &I2C0_bus_handle));
    sw7203_config_t config = {
        .i2cAddress = sw7203_I2c_0x3C,
        .INT_gpio_num = 2,
        .Mutex = &BQ4050_muTex,
        .SW7203_control_mode = {
            .mode = 0,
            .INTGPIOEnable = 0,
            .intMode = 0,
            .gpioInitMode = 0,
            .voltageConflictSwitch = 0,
            .I2CBusBusyEnable = 0,
        },
        .connectDetect.config = {
            .shutdownLowPowerModeEnable = 1,
            .a1PortDetectEnable = 1,
            .a2PortDetectEnable = 1,
        },
        .funcConfig1.config = {
            .switchFrequency = 0,
            .forcePWMModeEnable = 0,
            .NTCProtectEnable = 1,
            .PowerMOSMinConductTime = 0,
            .VBUSVoltageRegulationMethod = 0,
        },
        .funcConfig2.config = {
            .deadzoneM1ClosedM2Open = 3,
            .deadzoneM1OpenM2Closed = 3,
            .deadzoneM3ClosedM4Open = 3,
            .deadzoneM3OpenM4Closed = 3,
        },
        .funcConfig3.config = {
            .M2MOSRdsOn = 0,
            .nductanceValue = 2,
        },
        .funcConfig4.config = {
            .ACOutRegResetControl = 0,
            .stopChargingOnFullEnable = 0,
            .fullChargeCurrent = 0,
            .batteryCellNum = 3,
        },
        .funcConfig5.config = {
            .chipOverTemp = 0,
            .peakCurrentLimit = 3,
        },
        .funcConfig6.config = {
            .func62368Enable = 0,
            .func62368IBUSLimitEnable = 0,
            .chargeOverTimeEnable = 0,
            .trickleChargeTimeout = 3,
            .constantCurrentChargeTimeout = 3,
        },
        .funcConfig7.config = {
            .func62368HighTempCurrentLimit = 0,
            .func62368HighTempHysteresis = 0,
            .func62368HighTempVoltageLimit = 0,
            .func62368LowTempCurrentLimit = 0,
            .func62368LowTempHysteresis = 0,
            .func62368LowTempVoltageLimit = 0,
        },
        .NTCConfig.config = {
            .chargeNTCHighLimit = 2,
            .chargeNTCLowLimit = 2,
            .dischargeNTCHighLimit = 2,
            .dischargeNTCLowLimit = 2,
        },
        .NVDCConfig1.config = {
            .NVDCLearnModeEnable = 0,
            .VSYSOverLoad = 1,
        },
        .trickleConfig.config = {
            .trickleCurrent = 1,
            .trickleCurrentHysteresis = 3,
        },
        .chargeVoltage_mV = 14600,
        .trickleVoltage_mV = 12800,
        .chargeVBUSVoltageLimit_mV = 8000,
        .VSYSMinVoltage_mV = 9000,
        .chargeVBATCurrentLimit_mA = 2500,
        .chargeVBUSCurrentLimit_mA = 6000,
        .chargeLDOCurrentLimit_mA = 100,
    };
    ESP_ERROR_CHECK(sw7203_driver_install(&config, I2C0_bus_handle, sw7203_I2C_address_find(I2C0_bus_handle)));
}

void app_main(void)
{
    memset(trans, 0x00, sizeof(*trans));
    trans[0].length = 8;
    trans[0].tx_data[0] = 0x2a;
    trans[0].user = (void *)LCD_COMMAND;
    trans[0].flags = SPI_TRANS_USE_TXDATA;
    trans[1].length = 32;
    trans[1].user = (void *)LCD_DATA;
    trans[1].flags = SPI_TRANS_USE_TXDATA;
    trans[2].length = 8;
    trans[2].tx_data[0] = 0x2b;
    trans[2].user = (void *)LCD_COMMAND;
    trans[2].flags = SPI_TRANS_USE_TXDATA;
    trans[3].length = 32;
    trans[3].user = (void *)LCD_DATA;
    trans[3].flags = SPI_TRANS_USE_TXDATA;
    trans[4].length = 8;
    trans[4].tx_data[0] = 0x2c;
    trans[4].user = (void *)LCD_COMMAND;
    trans[4].flags = SPI_TRANS_USE_TXDATA;
    trans[5].length = DISP_BUF_SIZE * 24;
    trans[5].tx_buffer = NULL;
    trans[5].user = (void *)LCD_DATA;
    trans[5].flags = 0;
    const gpio_config_t LCD_gpio_config = {
        .pin_bit_mask = 1ull << PIN_NUM_LCD_DC | 1ull << PIN_NUM_LCD_RST | 1ull << PIN_NUM_LCD_CS | 1ull << PIN_NUM_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&LCD_gpio_config);
    ESP_LOGI(TAG, "Finished init gpio");
    const spi_bus_config_t SPI_1_CONF = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_SCLK,
        .max_transfer_sz = DISP_BUF_SIZE * 3 + 8,
    };
    errcode = spi_bus_initialize(SPI2_HOST, &SPI_1_CONF, SPI_DMA_CH_AUTO);
    if (errcode != ESP_OK)
    {
        ESP_LOGW(TAG, "SPI init ERROR");
        while (1)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
    ESP_LOGI(TAG, "Finished init spi bus");
    spi_device_interface_config_t SPI_LCD_CONF = {
        .flags = SPI_DEVICE_NO_DUMMY,
        .mode = 3,
        .clock_speed_hz = LCD_PIXEL_CLOCK_HZ,
        .spics_io_num = PIN_NUM_LCD_CS,
        .duty_cycle_pos = 0,
        .command_bits = 0,
        .address_bits = 0,
        .queue_size = 7,
        .pre_cb = SPIpreCB,
        .post_cb = SPIpostCB,
    };
    errcode = spi_bus_add_device(SPI2_HOST, &SPI_LCD_CONF, &spi2_handle);
    ESP_LOGI(TAG, "Finished init lcd spi");
    gpio_set_level(PIN_NUM_LCD_CS, 1);
    gpio_set_level(PIN_NUM_LCD_DC, 1);
    gpio_set_level(PIN_NUM_LCD_RST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_NUM_LCD_RST, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1);
    ESP_LOGI(TAG, "Finished init lcd backlight");
    LCD_init();
    ESP_LOGI(TAG, "Finished init lcd");
    ESP_LOGI(TAG, "the free heap size is %d --root_write_task(start)", esp_get_free_heap_size());
    vTaskDelay(500 / portTICK_PERIOD_MS);
    buffer[0] = heap_caps_malloc(DISP_BUF_SIZE * sizeof(Color_t) * 2, MALLOC_CAP_DMA);
    buffer[1] = heap_caps_malloc(DISP_BUF_SIZE * sizeof(Color_t) * 2, MALLOC_CAP_DMA);
    ESP_LOGI(TAG, "the free heap size is %d --root_write_task(start)", esp_get_free_heap_size());
    ESP_LOGI(TAG, "the heap size should be %d", DISP_BUF_SIZE * sizeof(Color_t) * 2 * 2);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (buffer[0] == NULL)
    {
        ESP_LOGW(TAG, "DMA Memory Limit Exceeded");
        while (1)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
    if (buffer[1] == NULL)
    {
        ESP_LOGW(TAG, "DMA Memory Limit Exceeded");
        while (1)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
    ESP_LOGI(TAG, "Finished malloc buffer");
    memset(buffer[calc_buffer], 0xffffffff, sizeof(*buffer[calc_buffer]) * DISP_BUF_SIZE);
    send_buffer = 0;
    calc_buffer = 1;
    ESP_LOGI(TAG, "Finished fill buffer white");
    int point_start_x = 0, point_start_y = -10, point_end_x = LCD_W - 1, point_end_y = -1;
    TickType_t xLastWakeTime;
    for (int i = 0; i < (LCD_H / 10); i++)
    {
        point_start_y = (point_start_y + 10) % LCD_H;
        point_end_y = (point_end_y + 10) % LCD_H;
        transbuffer(point_start_x, point_start_y, point_end_x, point_end_y);
        vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
        Finish_transfer_buffer(spi2_handle);
    }
    ESP_LOGI(TAG, "Finished flashing buffer");
    sw7203init();
    bq_Init(I2C0_bus_handle, 0x0D);
    DecodeASCIILine("Battery:", black, 300, 100);
    char *s[10];
    while (1)
    {
        int_to_string(bq_GetRSOC(), s);
        DecodeASCIILine(s, black, 300, 50);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}