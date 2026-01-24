#ifndef __M5HATMINIJOYC_H
#define __M5HATMINIJOYC_H

#include "Arduino.h"
#include "Wire.h"

#define MiniJoyC_ADDR        0x54
#define ADC_VALUE_REG        0x00
#define POS_VALUE_REG_10_BIT 0x10
#define POS_VALUE_REG_8_BIT  0x20
#define BUTTON_REG           0x30
#define RGB_LED_REG          0x40
#define CAL_REG              0x50
#define FIRMWARE_VERSION_REG 0xFE
#define I2C_ADDRESS_REG      0xFF

typedef enum { ADC_X = 0, ADC_Y } minijoyc_adc_index_t;
typedef enum { POS_X = 0, POS_Y } minijoyc_pos_index_t;
typedef enum { _8bit = 0, _10bit } minijoyc_pos_read_mode_t;
typedef enum {
    Cal_X_min = 0,
    Cal_X_max,
    Cal_Y_min,
    Cal_Y_max,
    Cal_X_cen,
    Cal_Y_cen
} minijoyc_cal_index_t;

class M5HatMiniJoyC {
   private:
    uint8_t _addr;
    TwoWire* _wire;
    uint8_t _scl;
    uint8_t _sda;
    uint8_t _speed;
    void writeBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);
    void readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);

   public:
    bool begin(TwoWire* wire = &Wire, uint8_t addr = MiniJoyC_ADDR,
               uint8_t sda = 0, uint8_t scl = 26,
               uint32_t speed = 200000L);  // Default StickC Plus
    uint16_t getADCValue(minijoyc_adc_index_t index);
    uint16_t getPOSValue(minijoyc_pos_index_t index,
                         minijoyc_pos_read_mode_t bit);
    void setOneCalValue(minijoyc_cal_index_t index, uint16_t data);
    void setAllCalValue(uint16_t* data);
    uint16_t getCalValue(minijoyc_cal_index_t index);
    bool getButtonStatus();
    void setLEDColor(uint32_t rgb888color);
    uint8_t setI2CAddress(uint8_t addr);
    uint8_t getI2CAddress(void);
    uint8_t getFirmwareVersion(void);
};

#endif
