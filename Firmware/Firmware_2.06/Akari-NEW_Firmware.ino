#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
#include "USBhandler.h"
#include "Serial.h"
#include <WS2812.h>
#include "Config.h"

uint8_t AimeKey[6] = { 0x57, 0x43, 0x43, 0x46, 0x76, 0x32 };
uint8_t BanaKey[6] = { 0x60, 0x90, 0xD0, 0x06, 0x32, 0xF5 };
uint8_t MifareKey[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

extern __xdata __at(EP0_ADDR)
uint8_t Ep0Buffer[];
extern __xdata __at(EP1_ADDR)
uint8_t Ep1Buffer[];
uint8_t ledData[21];
uint8_t report[11] = { 0 };
uint8_t report2[8] = { 0 };
#include "PN532.h"

uint8_t USB_EP1_send(uint8_t reportID, uint8_t* buffer, uint8_t length);

volatile __xdata uint8_t UpPoint1_Busy = 0;  // Flag of whether upload pointer is busys

void setup() {
  config_read();
  // put your setup code here, to run once:
  USBDeviceCfg();          // Device mode configuration
  USBDeviceEndPointCfg();  // Endpoint configuration
  USBDeviceIntCfg();       // Interrupt configuration
  UEP0_T_LEN = 0;
  UEP1_T_LEN = 0;  // Pre-use send length must be cleared
  UEP2_T_LEN = 0;

  Serial1_begin(115200);
  pn532_begin();
  delay(10);
  if (pn532_getFirmwareVersion() <= 0)
  {
    config.status = 0x01; // PN532 Error
    for (int i = 0; i < 6; i++) {
      set_pixel_for_GRB_LED(ledData, i, 255, 0, 0);  //Choose the color order depending on the LED you use.
    }
    neopixel_show_P1_5(ledData, 21);  //Possible to use other pins.
  }
  pn532_setPassiveActivationRetries(0x10);
  pn532_SAMConfig();
  if (!config.disableDefaultLED)
  {
    for (int i = 0; i < 6; i++) {
      set_pixel_for_GRB_LED(ledData, i, 255, 255, 255);  //Choose the color order depending on the LED you use.
    }
    neopixel_show_P1_5(ledData, 21);  //Possible to use other pins.
  }

}

uint8_t card_type = 0;  // 1-MIFARE, 2-FELICA
uint8_t mifare_uid[4] = { 0 };
uint8_t mifare_uidlen = 0;
uint8_t mifare_block[16] = { 0 };
uint8_t mifare_id[10] = { 0 };
uint8_t felica_idm[8] = { 0 };
uint8_t felica_pmm[8] = { 0 };
bool has_card;

void card_read() {
  uint16_t SystemCode;
  if (pn532_felica_Polling(0xFFFF, 0x00, felica_idm, felica_pmm, &SystemCode, 200))
  {
    report[0] = 2;
    memcpy(report + 1, felica_idm, 8);
    memcpy(report2, felica_idm, 8);
    has_card = true;
  }
  else if (pn532_readPassiveTargetID(PN532_MIFARE_ISO14443A, mifare_uid, &mifare_uidlen, 1000) && pn532_mifareclassic_AuthenticateBlock(mifare_uid, mifare_uidlen, 1, 0, AimeKey) && pn532_mifareclassic_ReadDataBlock(2, mifare_block))
  {
    report[0] = 1;
    memcpy(report + 1, mifare_block + 6, 10);
    memcpy(report2, mifare_block + 8, 8);
    report2[0] = 0xE0;
    report2[1] = 0x04;
    has_card = true;
  }
  else if (pn532_readPassiveTargetID(PN532_MIFARE_ISO14443A, mifare_uid, &mifare_uidlen, 1000) && pn532_mifareclassic_AuthenticateBlock(mifare_uid, mifare_uidlen, 1, 0, BanaKey) && pn532_mifareclassic_ReadDataBlock(2, mifare_block))
  {
    report[0] = 1;
    memcpy(report + 1, mifare_block + 6, 10);
    memcpy(report2, mifare_block + 8, 8);
    report2[0] = 0xE0;
    report2[1] = 0x04;
    has_card = true;
  }
  else if (pn532_readPassiveTargetID(PN532_MIFARE_ISO14443A, mifare_uid, &mifare_uidlen, 1000) && pn532_mifareclassic_AuthenticateBlock(mifare_uid, mifare_uidlen, 1, 0, MifareKey) && pn532_mifareclassic_ReadDataBlock(2, mifare_block))
  {
    report[0] = 1;
    memcpy(report + 1, mifare_block + 6, 10);
    memcpy(report2, mifare_block + 8, 8);
    report2[0] = 0xE0;
    report2[1] = 0x04;
    has_card = true;
  }
  else
  {
    memset(report, 0, 11);
    memset(report2, 0, 8);
    has_card = false;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (config.alwaysScan || (ledData[0] != 0 || ledData[1] != 0 || ledData[2] != 0))
  {
    card_read();
  }
  else
  {
    memset(report, 0, 11);
    memset(report2, 0, 8);
  }
  delay(1);
  USB_EP1_send(0x01, report, 11);
  if (has_card)
  {
    delay(1);
    USB_EP1_send(0x02, report2, 8);
  }
}

void USB_EP1_IN() {
  UEP1_T_LEN = 0;
  UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;  // Default NAK
  UpPoint1_Busy = 0;                                        // Clear busy flag
}

void USB_EP1_OUT()
{
  if (U_TOG_OK)  // Discard unsynchronized packets
  {
    switch (Ep1Buffer[0])
    {
      case 0x01:
        for (int i = 0; i < 6; i++) {
          set_pixel_for_GRB_LED(ledData, i, Ep1Buffer[1], Ep1Buffer[2], Ep1Buffer[3]);  //Choose the color order depending on the LED you use.
        }
        neopixel_show_P1_5(ledData, 18);  //Possible to use other pins.
        break;

      case 0x03:
        if (Ep1Buffer[1] == 0)
        {
          // Get Config
          //delay(1);
          USB_EP1_send(0x03, config.bytes, 16);
        }
        else
        {
          // Set Config
          for (int i = 0; i < 16; i++)
          {
            config.bytes[i] = Ep1Buffer[i + 1];
          }
          config_flush();
        }
        break;
    }
  }
}

uint8_t USB_EP1_send(uint8_t reportID, uint8_t* buffer, uint8_t length) {
  if (UsbConfig == 0) {
    return 0;
  }

  __data uint16_t waitWriteCount = 0;

  waitWriteCount = 0;
  while (UpPoint1_Busy) {  // wait for 250ms or give up
    waitWriteCount++;
    delayMicroseconds(5);
    if (waitWriteCount >= 50000)
      return 0;
  }
  memset(Ep1Buffer + 64, 0, 64);
  Ep1Buffer[64] = reportID;
  for (__data uint8_t i = 0; i < length; i++) {  // load data for upload
    Ep1Buffer[64 + i + 1] = buffer[i];
  }

  UEP1_T_LEN = length + 1;  // data length
  UpPoint1_Busy = 1;
  UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;  // upload data and respond ACK

  return 1;
}