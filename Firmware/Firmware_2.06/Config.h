#include <Arduino.h>

#define SW_MAJOR_VER 2
#define SW_MINUS_VER 6
#define SW_RELSE_VER 0

typedef union
{
  uint8_t bytes[16];
  struct
  {
    uint8_t hwMajorVer;
    uint8_t hwMinusVer;
    uint8_t swMajorVer;
    uint8_t swMinusVer;
    uint8_t swRelVer;
    uint8_t status;
    bool alwaysScan;
    bool disableDefaultLED;
  };
} _config;
_config config;

void config_flush();

void config_read()
  {
    memset(config.bytes, 0, 16);
    for (int i = 0; i < 16; i++)
    {
      config.bytes[i] = eeprom_read_byte(i);
    }
    if (config.bytes[0] == 0 || config.bytes[0] == 255)
    {
      // Initialize
      config.hwMajorVer = 1;
      config.hwMinusVer = 0;
      config.swMajorVer = 1;
      config.swMinusVer = 0;
      config.swRelVer = 0;
      config.alwaysScan = 0;
      config.disableDefaultLED = 0;
      config_flush();
    }
    config.swMajorVer = SW_MAJOR_VER;
    config.swMinusVer = SW_MINUS_VER;
    config.swRelVer = SW_RELSE_VER;
    config.status = 0;
  }

  void config_flush()
  {
    for (int i = 0; i < 16; i++)
    {
      eeprom_write_byte(i, config.bytes[i]);
    }
  }