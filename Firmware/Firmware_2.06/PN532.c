/**************************************************************************/
/*!
    @file     PN532.cpp
    @author   Adafruit Industries & Seeed Studio
    @license  BSD
*/
/**************************************************************************/

#include "Arduino.h"
#include "include/ch5xx.h"
#include "PN532.h"
#include <string.h>
#include <WS2812.h>
uint8_t ledData[21];

uint8_t pn532_packetbuffer[64];
uint8_t _uid[7];       // ISO14443A uid
uint8_t _uidLen;       // uid len
uint8_t _key[6];       // Mifare Classic key
uint8_t inListedTag;   // Tg number of inlisted tag.
uint8_t _felicaIDm[8]; // FeliCa IDm (NFCID2)
uint8_t _felicaPMm[8]; // FeliCa PMm (PAD)

void pn532_begin(void);
bool pn532_SAMConfig(void);

// Generic PN532 functions
uint32_t pn532_getFirmwareVersion(void);
bool pn532_setPassiveActivationRetries(uint8_t maxRetries);

// ISO14443A functions
bool pn532_readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout);

// Mifare Classic functions
uint8_t pn532_mifareclassic_AuthenticateBlock(uint8_t *uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t *keyData);
uint8_t pn532_mifareclassic_ReadDataBlock(uint8_t blockNumber, uint8_t *data);

// FeliCa Functions
int8_t pn532_felica_Polling(uint16_t systemCode, uint8_t requestCode, uint8_t *idm, uint8_t *pmm, uint16_t *systemCodeResponse, uint16_t timeout);

/**************************************************************************/
/*!
    @brief  Setups the HW
*/
/**************************************************************************/
void pn532_begin()
{
    pn532_wakeup();
}

/**************************************************************************/
/*!
    @brief  Checks the firmware version of the PN5xx chip

    @returns  The chip's firmware version and ID
*/
/**************************************************************************/
uint32_t pn532_getFirmwareVersion(void)
{
    uint32_t response;

    pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

    if (pn532_writeCommand(pn532_packetbuffer, 1, 0, 0))
    {
        return 0;
    }

    // read data packet
    int16_t status = pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);
    if (0 > status)
    {
        return 0;
    }

    response = pn532_packetbuffer[0];
    response <<= 8;
    response |= pn532_packetbuffer[1];
    response <<= 8;
    response |= pn532_packetbuffer[2];
    response <<= 8;
    response |= pn532_packetbuffer[3];

    return response;
}

/**************************************************************************/
/*!
    @brief  Configures the SAM (Secure Access Module)
*/
/**************************************************************************/
bool pn532_SAMConfig(void)
{
    pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!

    //DMSG("SAMConfig\n");

    if (pn532_writeCommand(pn532_packetbuffer, 4, 0, 0))
        return false;

    return (0 < pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000));
}

/**************************************************************************/
/*!
    Sets the MxRtyPassiveActivation uint8_t of the RFConfiguration register

    @param  maxRetries    0xFF to wait forever, 0x00..0xFE to timeout
                          after mxRetries

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool pn532_setPassiveActivationRetries(uint8_t maxRetries)
{
    pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
    pn532_packetbuffer[1] = 5;    // Config item 5 (MaxRetries)
    pn532_packetbuffer[2] = 0xFF; // MxRtyATR (default = 0xFF)
    pn532_packetbuffer[3] = 0x01; // MxRtyPSL (default = 0x01)
    pn532_packetbuffer[4] = maxRetries;

    if (pn532_writeCommand(pn532_packetbuffer, 5, 0, 0))
        return 0x0; // no ACK

    return (0 < pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000));
}

/***** ISO14443A Commands ******/

/**************************************************************************/
/*!
    Waits for an ISO14443A target to enter the field

    @param  cardBaudRate  Baud rate of the card
    @param  uid           Pointer to the array that will be populated
                          with the card's UID (up to 7 bytes)
    @param  uidLength     Pointer to the variable that will hold the
                          length of the card's UID.
    @param  timeout       The number of tries before timing out

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool pn532_readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout)
{
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;

    if (pn532_writeCommand(pn532_packetbuffer, 3, 0, 0))
    {
        return 0x0; // command failed
    }

    // read data packet
    if (pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), timeout) < 0)
    {
        return 0x0;
    }

    // check some basic stuff
    /* ISO14443A card response should be in the following format:

      byte            Description
      -------------   ------------------------------------------
      b0              Tags Found
      b1              Tag Number (only one used in this example)
      b2..3           SENS_RES
      b4              SEL_RES
      b5              NFCID Length
      b6..NFCIDLen    NFCID
    */

    if (pn532_packetbuffer[0] != 1)
        return 0;

    uint16_t sens_res = pn532_packetbuffer[2];
    sens_res <<= 8;
    sens_res |= pn532_packetbuffer[3];

    //DMSG("ATQA: 0x");
    //DMSG_HEX(sens_res);
    //DMSG("SAK: 0x");
    //DMSG_HEX(pn532_packetbuffer[4]);
    //DMSG("\n");

    /* Card appears to be Mifare Classic */
    *uidLength = pn532_packetbuffer[5];

    for (uint8_t i = 0; i < pn532_packetbuffer[5]; i++)
    {
        uid[i] = pn532_packetbuffer[6 + i];
    }

    return 1;
}

/**************************************************************************/
/*!
    Tries to authenticate a block of memory on a MIFARE card using the
    INDATAEXCHANGE command.  See section 7.3.8 of the PN532 User Manual
    for more information on sending MIFARE and other commands.

    @param  uid           Pointer to a byte array containing the card UID
    @param  uidLen        The length (in bytes) of the card's UID (Should
                          be 4 for MIFARE Classic)
    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  keyNumber     Which key type to use during authentication
                          (0 = MIFARE_CMD_AUTH_A, 1 = MIFARE_CMD_AUTH_B)
    @param  keyData       Pointer to a byte array containing the 6 bytes
                          key value

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_mifareclassic_AuthenticateBlock(uint8_t *uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t *keyData)
{
    uint8_t i;

    // Hang on to the key and uid data
    memcpy(_key, keyData, 6);
    memcpy(_uid, uid, uidLen);
    _uidLen = uidLen;

    // Prepare the authentication command //
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE; /* Data Exchange Header */
    pn532_packetbuffer[1] = 1;                            /* Max card numbers */
    pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
    pn532_packetbuffer[3] = blockNumber; /* Block Number (1K = 0..63, 4K = 0..255 */
    memcpy(pn532_packetbuffer + 4, _key, 6);
    for (i = 0; i < _uidLen; i++)
    {
        pn532_packetbuffer[10 + i] = _uid[i]; /* 4 bytes card ID */
    }

    if (pn532_writeCommand(pn532_packetbuffer, 10 + _uidLen, 0, 0))
        return 0;

    // Read the response packet
    pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);

    // Check if the response is valid and we are authenticated???
    // for an auth success it should be bytes 5-7: 0xD5 0x41 0x00
    // Mifare auth error is technically byte 7: 0x14 but anything other and 0x00 is not good
    if (pn532_packetbuffer[0] != 0x00)
    {
        //DMSG("Authentification failed\n");
        return 0;
    }

    return 1;
}

/**************************************************************************/
/*!
    Tries to read an entire 16-bytes data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          Pointer to the byte array that will hold the
                          retrieved data (if any)

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_mifareclassic_ReadDataBlock(uint8_t blockNumber, uint8_t *data)
{
    //DMSG("Trying to read 16 bytes from block ");
    //DMSG_INT(blockNumber);

    /* Prepare the command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;               /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
    pn532_packetbuffer[3] = blockNumber;     /* Block Number (0..63 for 1K, 0..255 for 4K) */

    /* Send the command */
    if (pn532_writeCommand(pn532_packetbuffer, 4, 0, 0))
    {
        return 0;
    }

    /* Read the response packet */
    pn532_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);

    /* If byte 8 isn't 0x00 we probably have an error */
    if (pn532_packetbuffer[0] != 0x00)
    {
        return 0;
    }

    /* Copy the 16 data bytes to the output buffer        */
    /* Block content starts at byte 9 of a valid response */
    memcpy(data, pn532_packetbuffer + 1, 16);

    return 1;
}

/***** FeliCa Functions ******/
/**************************************************************************/
/*!
    @brief  Poll FeliCa card. PN532 acting as reader/initiator,
            peer acting as card/responder.
    @param[in]  systemCode             Designation of System Code. When sending FFFFh as System Code,
                                       all FeliCa cards can return response.
    @param[in]  requestCode            Designation of Request Data as follows:
                                         00h: No Request
                                         01h: System Code request (to acquire System Code of the card)
                                         02h: Communication perfomance request
    @param[out] idm                    IDm of the card (8 bytes)
    @param[out] pmm                    PMm of the card (8 bytes)
    @param[out] systemCodeResponse     System Code of the card (Optional, 2bytes)
    @return                            = 1: A FeliCa card has detected
                                       = 0: No card has detected
                                       < 0: error
*/
/**************************************************************************/
int8_t pn532_felica_Polling(uint16_t systemCode, uint8_t requestCode, uint8_t *idm, uint8_t *pmm, uint16_t *systemCodeResponse, uint16_t timeout)
{
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1;
    pn532_packetbuffer[2] = 1;
    pn532_packetbuffer[3] = FELICA_CMD_POLLING;
    pn532_packetbuffer[4] = (systemCode >> 8) & 0xFF;
    pn532_packetbuffer[5] = systemCode & 0xFF;
    pn532_packetbuffer[6] = requestCode;
    pn532_packetbuffer[7] = 0;

    if (pn532_writeCommand(pn532_packetbuffer, 8, 0, 0))
    {
        //DMSG("Could not send Polling command\n");
        return -1;
    }

    int16_t status = pn532_readResponse(pn532_packetbuffer, 22, timeout);
    if (status < 0)
    {
        //DMSG("Could not receive response\n");
        return -2;
    }

    // Check NbTg (pn532_packetbuffer[7])
    if (pn532_packetbuffer[0] == 0)
    {
        //DMSG("No card had detected\n");
        return 0;
    }
    else if (pn532_packetbuffer[0] != 1)
    {
        //DMSG("Unhandled number of targets inlisted. NbTg: ");
        //DMSG_HEX(pn532_packetbuffer[7]);
        //DMSG("\n");
        return -3;
    }

    inListedTag = pn532_packetbuffer[1];
    //DMSG("Tag number: ");
    //DMSG_HEX(pn532_packetbuffer[1]);
    //DMSG("\n");

    // length check
    uint8_t responseLength = pn532_packetbuffer[2];
    if (responseLength != 18 && responseLength != 20)
    {
        //DMSG("Wrong response length\n");
        return -4;
    }

    uint8_t i;
    for (i = 0; i < 8; ++i)
    {
        idm[i] = pn532_packetbuffer[4 + i];
        _felicaIDm[i] = pn532_packetbuffer[4 + i];
        pmm[i] = pn532_packetbuffer[12 + i];
        _felicaPMm[i] = pn532_packetbuffer[12 + i];
    }

    if (responseLength == 20)
    {
        *systemCodeResponse = (uint16_t)((pn532_packetbuffer[20] << 8) + pn532_packetbuffer[21]);
    }

    return 1;
}