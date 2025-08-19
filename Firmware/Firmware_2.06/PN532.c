#include "Arduino.h"
#include "include/ch5xx.h"
#include "PN532.h"
#include <string.h>

__xdata uint8_t pn532_packetbuffer[64];
__xdata uint8_t pn532_felica_cmd[15];
__xdata uint8_t pn532_felica_data[16];
void pn532_begin(void);
bool pn532_SAMConfig(void);

uint32_t pn532_getFirmwareVersion(void);
bool pn532_setPassiveActivationRetries(uint8_t maxRetries);

bool pn532_readPassiveTargetID(uint16_t timeout);

int8_t pn532_mifareclassic_AuthenticateBlock(uint8_t uidLen);
uint8_t pn532_mifareclassic_ReadDataBlock();
uint8_t pn532_mifareclassic_WriteDataBlock();

int8_t pn532_felica_Polling(uint16_t timeout);

bool pn532_SAMConfig(void) {
  pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
  pn532_packetbuffer[1] = 0x01;
  pn532_packetbuffer[2] = 0x14;
  pn532_packetbuffer[3] = 0x01;

  if (pn532_writeCommand(pn532_packetbuffer, 4, 0, 0))
    return false;

  return (0 < pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000));
}

bool pn532_setPassiveActivationRetries(uint8_t maxRetries) {
  pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
  pn532_packetbuffer[1] = 5;
  pn532_packetbuffer[2] = 0xFF;
  pn532_packetbuffer[3] = 0x01;
  pn532_packetbuffer[4] = maxRetries;

  if (pn532_writeCommand(pn532_packetbuffer, 5, 0, 0))
    return 0x0;

  return (0 < pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000));
}

bool pn532_readPassiveTargetID(uint16_t timeout) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;
  pn532_packetbuffer[2] = PN532_MIFARE_ISO14443A;

  if (pn532_writeCommand(pn532_packetbuffer, 3, 0, 0)) {
    return 0x0;
  }

  if (pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), timeout) < 0) {
    return 0x0;
  }

  if (pn532_packetbuffer[0] != 1)
    return 0;
  /*
    uint16_t sens_res = pn532_packetbuffer[2];
    sens_res <<= 8;
    sens_res |= pn532_packetbuffer[3];

    *uidLength = pn532_packetbuffer[5];

    for (uint8_t i = 0; i < pn532_packetbuffer[5]; i++)
    {
        uid[i] = pn532_packetbuffer[6 + i];
    }
*/
  return 1;
}

int8_t pn532_mifareclassic_AuthenticateBlock(uint8_t uidLen) {
  //uint8_t i;
  /*
    memcpy(_key, keyData, 6);
    memcpy(_uid, uid, uidLen);
    _uidLen = uidLen;
*/
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;
  if (pn532_writeCommand(pn532_packetbuffer, 10 + uidLen, 0, 0))
    return 0;

  pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);

  if (pn532_packetbuffer[0] != 0x00) {
    return 0;
  }
  return 1;
  /*
    pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
    pn532_packetbuffer[3] = blockNumber;
    memcpy(pn532_packetbuffer + 4, _key, 6);
    for (i = 0; i < _uidLen; i++)
    {
        pn532_packetbuffer[10 + i] = _uid[i];
    }

    if (pn532_writeCommand(pn532_packetbuffer, 10 + uidLen, 0, 0))
        return 0;

    pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);

    if (pn532_packetbuffer[0] != 0x00)
    {
        return 0;
    }

    return 1;
    */
}

uint8_t pn532_mifareclassic_ReadDataBlock() {
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;
  pn532_packetbuffer[2] = MIFARE_CMD_READ;
  //pn532_packetbuffer[3] = blockNumber;

  if (pn532_writeCommand(pn532_packetbuffer, 4, 0, 0)) {
    return 0;
  }

  pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);

  if (pn532_packetbuffer[0] != 0x00) {
    return 0;
  }

  //memcpy(data, pn532_packetbuffer + 1, 16);

  return 1;
}
uint8_t pn532_mifareclassic_WriteDataBlock() {
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;                /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_WRITE; /* Mifare Write command = 0xA0 */
  //pn532_packetbuffer[3] = blockNumber;      /* Block Number (0..63 for 1K, 0..255 for 4K) */
  //memcpy(pn532_packetbuffer + 4, data, 16); /* Data Payload */

  /* Send the command */
  if (pn532_writeCommand(pn532_packetbuffer, 20, 0, 0)) {
    return 0;
  }

  /* Read the response packet */
  if (0 > pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000)) {
    return 0;
  }

  if (pn532_packetbuffer[0] != 0x00) {
    return 0;
  }

  return 1;
}

int8_t pn532_felica_Polling(uint16_t timeout) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;
  pn532_packetbuffer[2] = 1;
  pn532_packetbuffer[3] = FELICA_CMD_POLLING;

  //pn532_packetbuffer[4] = (a >> 8) & 0xFF;
  //pn532_packetbuffer[5] = a & 0xFF;
  //pn532_packetbuffer[6] = b;

  pn532_packetbuffer[7] = 0;

  if (pn532_writeCommand(pn532_packetbuffer, 8, 0, 0)) {
    return -1;
  }

  int16_t status = pn532_readResponse(pn532_packetbuffer, 22, timeout);
  if (status < 0) {
    return -2;
  }

  if (pn532_packetbuffer[0] == 0) {
    return 0;
  } else if (pn532_packetbuffer[0] != 1) {
    return -3;
  }

  //inListedTag = pn532_packetbuffer[1];

  uint8_t responseLength = pn532_packetbuffer[2];
  if (responseLength != 18 && responseLength != 20) {
    return -4;
  }
  /*
    uint8_t i;

    for (i = 0; i < 8; ++i)
    {
        idm[i] = pn532_packetbuffer[4 + i];
        _felicaIDm[i] = pn532_packetbuffer[4 + i];
        pmm[i] = pn532_packetbuffer[12 + i];
        _felicaPMm[i] = pn532_packetbuffer[12 + i];
    }
*/
  return 1;
}

int8_t pn532_felica_ReadWithoutEncryption() {

  pn532_felica_cmd[0] = FELICA_CMD_READ_WITHOUT_ENCRYPTION;
  pn532_felica_cmd[9] = 0x01;
  pn532_felica_cmd[12] = 0x01;
  pn532_packetbuffer[0] = 0x40;  // PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 0x01;
  pn532_packetbuffer[2] = 16;

  if (pn532_writeCommand(pn532_packetbuffer, 3, pn532_felica_cmd, 15)) {
    //DMSG("Could not send FeliCa command\n");
    return -1;
  }
  // Wait card response
  int16_t status = pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 200);
  if (status < 0) {
    //DMSG("Could not receive response\n");
    return -2;
  }

  // Check status (pn532_packetbuffer[0])
  if ((pn532_packetbuffer[0] & 0x3F) != 0) {
    //DMSG("Status code indicates an error: ");
    //DMSG_HEX(pn532_packetbuffer[0]);
    //DMSG("\n");
    return -3;
  }

  // length check
  if (pn532_packetbuffer[1] != 29) {
    //DMSG("Read Without Encryption command failed (wrong response length)\n");
    return -5;
  }

  // status flag check
  if (pn532_packetbuffer[11] != 0 || pn532_packetbuffer[12] != 0) {
    //DMSG("Read Without Encryption command failed (Status Flag: ");
    //DMSG_HEX(pn532_packetbuffer[9]);
    //DMSG_HEX(pn532_packetbuffer[10]);
    //DMSG(")\n");
    return -6;
  }

  for (uint8_t i = 0; i < 16; i++) {
    pn532_felica_data[i] = pn532_packetbuffer[14 + i];
  }

  return 1;
}