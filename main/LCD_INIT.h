

enum LCD_ACC
{
    LCD_0degree = 0x08,
    LCD_180degree = 0xC8,
    LCD_90degree = 0x78,
    LCD_270degree = 0xA8,
};
#define LCD_MEMORY_ACC LCD_90degree
#if (LCD_MEMORY_ACC == 0x08) || (LCD_MEMORY_ACC == 0xC8)
#define LCD_W 320
#define LCD_H 480
#else
#define LCD_W 480
#define LCD_H 320
#endif
#define LCD_PIXEL_CLOCK_HZ (50 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL
#define PIN_NUM_SCLK 8
#define PIN_NUM_MOSI 18
#define PIN_NUM_MISO 7
#define PIN_NUM_LCD_DC 16
#define PIN_NUM_LCD_RST 17
#define PIN_NUM_LCD_CS 6
#define PIN_NUM_BK_LIGHT 15
#define PIN_NUM_TOUCH_CS 5
#define PIN_NUM_TOUCH_IRQ 4
#define DISP_BUF_SIZE LCD_W * 25

typedef enum
{
    LCD_COMMAND = 0,
    LCD_DATA = 1,

} LCD_type_DC_t;

typedef struct
{
    LCD_type_DC_t LCD_type_DC;
    uint8_t len;
    uint8_t data[16];
    uint8_t delay_ms;
} LCDinitDATA_t;

DRAM_ATTR static const LCDinitDATA_t init_data[] = {
    {
        LCD_COMMAND,
        1,
        {0x11},
        120,
    },
    {
        LCD_COMMAND,
        1,
        {0XF2},
        0,
    },
    {
        LCD_DATA,
        9,
        {0x18,
         0xA3,
         0x12,
         0x02,
         0XB2,
         0x12,
         0xFF,
         0x10,
         0x00},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0XF8},
        0,
    },
    {
        LCD_DATA,
        2,
        {0x21,
         0x04},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0x36},
        0,
    },
    {
        LCD_DATA,
        1,
        {LCD_MEMORY_ACC},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0xB4},
        0,
    },
    {
        LCD_DATA,
        1,
        {0x02},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0xB6},
        0,
    },
    {
        LCD_DATA,
        2,
        {0x02,
         0x22},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0xC1},
        0,
    },
    {
        LCD_DATA,
        1,
        {0x41},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0xC5},
        0,
    },
    {
        LCD_DATA,
        2,
        {0x00,
         0x18},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0x3a},
        0,
    },
    {
        LCD_DATA,
        1,
        {0x66},
        50,
    },
    {
        LCD_COMMAND,
        1,
        {0xE0},
        0,
    },
    {
        LCD_DATA,
        15,
        {0x0F,
         0x1F,
         0x1C,
         0x0C,
         0x0F,
         0x08,
         0x48,
         0x98,
         0x37,
         0x0A,
         0x13,
         0x04,
         0x11,
         0x0D,
         0x00},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0xE1},
        0,
    },
    {
        LCD_DATA,
        15,
        {0x0F,
         0x32,
         0x2E,
         0x0B,
         0x0D,
         0x05,
         0x47,
         0x75,
         0x37,
         0x06,
         0x10,
         0x03,
         0x24,
         0x20,
         0x00},
        0,
    },
    {
        LCD_COMMAND,
        1,
        {0x11},
        120,
    },
    {
        LCD_COMMAND,
        2,
        {0x29, 0x2C},
        0,
    },
    {
        LCD_COMMAND,
        0xff,
        {0x29, 0x2C},
        0,
    },
};
