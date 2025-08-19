#include "USBCDC.h"
#include "PN532.h"
#include "WS2812.h"
#include "AccessCode.h"
#include "USBconstant.h"

#define DEVICE_MODE_API 0x00
#define DEVICE_MODE_HID 0x01
#define DEVICE_MODE_CDC 0x02

#define INS_REBOOT_TO_DFU 0xFF
#define INS_SET_LED 0x01
#define INS_DECRYPT_ACCESS_CODE 0x02
#define INS_READ_CONFIG 0x03
#define INS_WRITE_CONFIG 0x04
#define INS_POLLING 0x05
#define INS_READ_MIFARE_BLOCK 0x06
#define INS_WRITE_MIFARE_BLOCK 0x07
#define INS_READ_FELICA_BLOCK 0x08

#define CFG_BOOT_LIGHT_SW 0x00
#define CFG_BOOT_MODE 0x01

#define SW_SUCCESS 0x90
#define SW_UNKNOWN_INS 0x70
#define SW_NO_CARD 0x80
#define SW_AUTH_FAILED 0x81
#define SW_READ_FAILED 0x82
#define SW_WRITE_FAILED 0x83

#define CARD_TYPE_FELICA 0x02
#define CARD_TYPE_ISO14443A 0x03
#define CARD_TYPE_NOCARD 0x01

volatile __xdata uint8_t UpPoint3_Busy = 0;
volatile __xdata uint8_t SYSTEM_MODE = 0;
volatile __xdata uint8_t API_ASYNC_FLAG = 0;
extern __xdata uint8_t pn532_packetbuffer[64];
extern __xdata uint8_t pn532_felica_cmd[15];
extern __xdata uint8_t pn532_felica_data[16];
extern __xdata __at(EP3_ADDR)
uint8_t Ep3Buffer[];

__xdata uint8_t LED_DATA[18];


void rgb_show(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < 6; i++) {
    LED_DATA[3 * i] = g;
    LED_DATA[3 * i + 1] = r;
    LED_DATA[3 * i + 2] = b;
  }
  neopixel_show_P1_5(LED_DATA, 18);
}

void USB_EP3_IN() {
  UEP3_T_LEN = 0;
  UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
  UpPoint3_Busy = 0;
}

uint8_t USB_EP3_send(uint8_t size) {
  __data uint16_t waitWriteCount = 0;
  waitWriteCount = 0;
  while (UpPoint3_Busy) {
    waitWriteCount++;
    delayMicroseconds(5);
    if (waitWriteCount >= 50000)
      return 0;
  }

  UEP3_T_LEN = size;
  UpPoint3_Busy = 1;
  UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;

  return 1;
}

void rebootToDFU() {
  USB_CTRL = 0;
  EA = 0;
  TMOD = 0;
  delayMicroseconds(50000);
  delayMicroseconds(50000);
  __asm__("lcall #0x3800");
  while (1)
    ;
}

void hid_loop() {
  pn532_packetbuffer[4] = 0xFF;
  pn532_packetbuffer[5] = 0xFF;
  pn532_packetbuffer[6] = 0x00;
  if (pn532_felica_Polling(200) && pn532_packetbuffer[4] != 0xFF)
    memcpy(Ep3Buffer + 65, pn532_packetbuffer + 4, 8);
  else if (pn532_readPassiveTargetID(200)) {
    memset(Ep3Buffer + 64, 0, 9);
    memcpy(Ep3Buffer + 73 - pn532_packetbuffer[5], pn532_packetbuffer + 6, pn532_packetbuffer[5]);
    Ep3Buffer[65] = 0x10;
    Ep3Buffer[66] = 0x04;
  } else return;
  Ep3Buffer[64] = 0x02;
  USB_EP3_send(9);
}

void cdc_loop() {
  while (USBSerial_available())
    Serial1_write(USBSerial_read());

  while (Serial1_available())
    USBSerial_write(Serial1_read());
  USBSerial_flush();
}

void api_loop_sync() {
  memset(Ep3Buffer + 64, 0, 64);
  switch (Ep3Buffer[1]) {
    case INS_REBOOT_TO_DFU:
      {
        rebootToDFU();
        break;
      }
    case INS_DECRYPT_ACCESS_CODE:
      {
        memcpy(ACCESS_CODE_CACHE, Ep3Buffer + 2, 16);
        DECRYPT_ACCESSCODE();
        memcpy(Ep3Buffer + 65, ACCESS_CODE_CACHE, 16);
        break;
      }
    case INS_SET_LED:
      {
        rgb_show(Ep3Buffer[2], Ep3Buffer[3], Ep3Buffer[4]);
        Ep3Buffer[65] = SW_SUCCESS;
        return;
      }
    case INS_READ_CONFIG:
      {
        for (__data uint8_t i = 0; i < 32; i++)
          Ep3Buffer[65 + i] = eeprom_read_byte(i);
        break;
      }
    case INS_WRITE_CONFIG:
      {
        for (__data uint8_t i = 0; i < 32; i++)
          eeprom_write_byte(i, Ep3Buffer[2 + i]);
        Ep3Buffer[65] = SW_SUCCESS;
        break;
      }
    default:
      {
        API_ASYNC_FLAG = 1;
        return;
      }
  }
  Ep3Buffer[64] = 0x03;
  USB_EP3_send(64);
}

void api_loop_async() {
  memset(Ep3Buffer + 64, 0, 64);
  switch (Ep3Buffer[1]) {
    case INS_POLLING:
      {
        memcpy(pn532_packetbuffer + 4, Ep3Buffer + 2, 3);
        if (pn532_felica_Polling(200) && pn532_packetbuffer[4] != 0xFF) {
          Ep3Buffer[65] = CARD_TYPE_FELICA;
          Ep3Buffer[66] = 0x12;
          memcpy(Ep3Buffer + 67, pn532_packetbuffer + 4, 18);
        } else if (pn532_readPassiveTargetID(200)) {
          Ep3Buffer[65] = CARD_TYPE_ISO14443A;
          Ep3Buffer[66] = pn532_packetbuffer[5];
          memcpy(Ep3Buffer + 67, pn532_packetbuffer + 6, pn532_packetbuffer[5]);
        } else {
          Ep3Buffer[65] = CARD_TYPE_NOCARD;
        };
        break;
      }
    case INS_READ_MIFARE_BLOCK:  //06 60 04 FF FF FF FF FF FF 04 BA A7 9D E3
      {
        memcpy(pn532_packetbuffer + 2, Ep3Buffer + 2, 8);
        for (__data uint8_t i = 0; i < Ep3Buffer[10]; i++) pn532_packetbuffer[10 + i] = Ep3Buffer[11 + i];
        if (pn532_mifareclassic_AuthenticateBlock(Ep3Buffer[10])) {
          pn532_packetbuffer[3] = Ep3Buffer[3];
          if (pn532_mifareclassic_ReadDataBlock()) {
            memcpy(Ep3Buffer + 66, pn532_packetbuffer + 1, 16);
            Ep3Buffer[65] = SW_SUCCESS;
          } else Ep3Buffer[65] = SW_READ_FAILED;
        } else Ep3Buffer[65] = SW_AUTH_FAILED;
        break;
      }
    case INS_WRITE_MIFARE_BLOCK:  //07 60 04 FF FF FF FF FF FF 04 BA A7 9D E3 [DATA]
      {
        memcpy(pn532_packetbuffer + 2, Ep3Buffer + 2, 8);
        for (__data uint8_t i = 0; i < Ep3Buffer[10]; i++) pn532_packetbuffer[10 + i] = Ep3Buffer[11 + i];
        if (pn532_mifareclassic_AuthenticateBlock(Ep3Buffer[10])) {
          pn532_packetbuffer[3] = Ep3Buffer[3];
          memcpy(pn532_packetbuffer + 4, Ep3Buffer + 11 + Ep3Buffer[10], 16);
          if (pn532_mifareclassic_WriteDataBlock()) {
            Ep3Buffer[65] = SW_SUCCESS;
          } else Ep3Buffer[65] = SW_WRITE_FAILED;
        } else Ep3Buffer[65] = SW_AUTH_FAILED;
        break;
      }
    case INS_READ_FELICA_BLOCK:
      {
        memcpy(pn532_felica_cmd + 1, Ep3Buffer + 2, 8);
        pn532_felica_cmd[10] = Ep3Buffer[10];
        pn532_felica_cmd[11] = Ep3Buffer[11];
        pn532_felica_cmd[13] = Ep3Buffer[12];
        pn532_felica_cmd[14] = Ep3Buffer[13];
        if (pn532_felica_ReadWithoutEncryption() > 0) {
          if (Ep3Buffer[14] != 0x00) {
            memcpy(ACCESS_CODE_CACHE, pn532_felica_data, 16);
            DECRYPT_ACCESSCODE();
            memcpy(Ep3Buffer + 65, ACCESS_CODE_CACHE, 16);
          } else memcpy(Ep3Buffer + 65, pn532_felica_data, 16);
          Ep3Buffer[65] = SW_SUCCESS;
        } else {
          Ep3Buffer[65] = SW_READ_FAILED;
        }
        break;
      }
    default:
      {
        Ep3Buffer[64] = SW_UNKNOWN_INS;
        break;
      }
  }
  Ep3Buffer[64] = 0x03;
  USB_EP3_send(64);
  API_ASYNC_FLAG = 0;
}

void USB_EP3_OUT() {
  if (Ep3Buffer[0] == 0x01) {
    if (SYSTEM_MODE != DEVICE_MODE_HID)
      SYSTEM_MODE = DEVICE_MODE_HID;
    rgb_show(Ep3Buffer[1], Ep3Buffer[2], Ep3Buffer[3]);
  }
  if (Ep3Buffer[0] == 0x03) {
    if (SYSTEM_MODE != DEVICE_MODE_API)
      SYSTEM_MODE = DEVICE_MODE_API;
    api_loop_sync();
  }
}

void setup() {
  Serial1_begin(115200);
  USBInit();
  pn532_wakeup();
  pn532_setPassiveActivationRetries(0x10);
  pn532_SAMConfig();
  rgb_show(255, 255, 255);
  delay(1000);

  SYSTEM_MODE = eeprom_read_byte(CFG_BOOT_MODE);

  if (eeprom_read_byte(CFG_BOOT_LIGHT_SW))
    rgb_show(255, 255, 255);
  else rgb_show(0, 0, 0);
}

void loop() {
  if (SYSTEM_MODE != DEVICE_MODE_CDC && USBSerial_available())  //SWITCH CDC
  {
    SYSTEM_MODE = DEVICE_MODE_CDC;
    rgb_show(255, 255, 0);
  }
  switch (SYSTEM_MODE) {
    case DEVICE_MODE_HID:
      {
        hid_loop();
        break;
      }
    case DEVICE_MODE_CDC:
      {
        cdc_loop();
        break;
      }
    case DEVICE_MODE_API:
      {
        if (API_ASYNC_FLAG) api_loop_async();
        break;
      }
  }
}
/*
void cdc_loop() {
  while (USBSerial_available())
    Serial1_write(USBSerial_read());

  while (Serial1_available())
    USBSerial_write(Serial1_read());
  USBSerial_flush();
}

void hid_loop() {
  if (EP3_OUT_FLAG == 0x01) {
    rgb_show(Ep3Buffer[1], Ep3Buffer[2], Ep3Buffer[3]);
    EP3_OUT_FLAG = 0;
  }
  if (pn532_felica_Polling(0x00, 200)) {
    memcpy(Ep3Buffer + 65, pn532_felica_IDm, 8);
    USB_EP3_send(9);
  }
  Ep3Buffer[64] = 0x02;
  if (pn532_felica_Polling(0x00, 200))
    memcpy(Ep3Buffer + 65, pn532_felica_IDm, 8);
  else if (pn532_readPassiveTargetID(PN532_MIFARE_ISO14443A, 1000)) {

    memset(Ep3Buffer, 0, 9);
    if (pn532_uidLen > 8)
      memcpy(Ep3Buffer + 65, pn532_uid + (pn532_uidLen - 8), 8);
    else
      memcpy(Ep3Buffer + 73 - pn532_uidLen, pn532_uid, pn532_uidLen);

    Ep3Buffer[65] = 0x10;
    Ep3Buffer[66] = 0x04;
  } else {
    return;
  }
  

}

void setup() {
  pinMode(15, OUTPUT);
  Serial1_begin(115200);
  USBInit();
  pn532_wakeup();
  delay(10);
  if (pn532_getFirmwareVersion() <= 0)
    while (true) rgb_show(100, 0, 0);
  else rgb_show(0, 100, 0);
  pn532_setPassiveActivationRetries(0x10);
  pn532_SAMConfig();
}
void loop() {
  //hid_loop();
  delay(100);

  if (SYSTEM_MODE != DEVICE_MODE_HID && EP3_OUT_FLAG == 0x01)  //SWITCH HID
  {
    SYSTEM_MODE = DEVICE_MODE_HID;
    rgb_show(255, 0, 255);
  }

  if (SYSTEM_MODE != DEVICE_MODE_API && EP3_OUT_FLAG == 0x03)  //SWITCH API
  {
    SYSTEM_MODE = DEVICE_MODE_API;
    rgb_show(0, 255, 255);
  }

  if (SYSTEM_MODE != DEVICE_MODE_CDC && USBSerial_available())  //SWITCH CDC
  {
    SYSTEM_MODE = DEVICE_MODE_CDC;
    rgb_show(255, 255, 0);
  }
  switch (SYSTEM_MODE) {
    case DEVICE_MODE_HID:
      {
        hid_loop();
        break;
      }
    case DEVICE_MODE_API:
      {
        api_loop();
        break;
      }
    case DEVICE_MODE_CDC:
      {
        cdc_loop();
        break;
      }
  }

}
*/