#include "aimeio.h"

Device device(0x303A, 0x81F0, 0x03);
Device_Input device_input;
Device_Output device_output;
uint8_t accessCode[10] = { 0 };
uint8_t buffer[64] = { 0 };
bool felica_direct = false;
time_t beginTime = clock();

void CloseAllLights()
{
    device_output.reportID = 0x03;
    device_output.command = 0x01;
    device_output.Light[0] = 0;
    device_output.Light[1] = 0;
    device_output.Light[2] = 0;
    device.write(device_output.bytes, sizeof(device_output.bytes));
}

uint16_t aime_io_get_api_version(void)
{
    return 0x0100;
}

uint8_t command_getConfig[] = { 0x03, 0x03 };
HRESULT aime_io_init(void)
{
    Engine::Print("[INFO][Engine] SeikanIO_chuniio Version 2.06.00 | Updated on 2025/8/19");
    memset(device_output.bytes, 0, sizeof(device_output.bytes));
    device.connect();
    device.write(command_getConfig, 2);
    Sleep(50);
    device.read(buffer, 64);
    felica_direct = buffer[3];
    if (felica_direct)
    {
        Engine::Print("[INFO][Engine] Felica access code read enabled");
    }
    CloseAllLights();
    Engine::Print("[INFO][Engine] Initialized");
    return HRESULT(0);
}

uint8_t command_poll[] = { 0x03, 0x05, 0xFF, 0xFF, 0x01 };
uint8_t command_read_mifare_code_aime[64] = { 0x03, 0x06, 0x60, 0x02, 0x57, 0x43, 0x43, 0x46, 0x76, 0x32, 0x00 };
uint8_t command_read_felica_code_aime[64] = { 0x03, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0B, 0x00, 0x80, 0x00, 0x00 };
HRESULT aime_io_nfc_poll(uint8_t unit_no)
{
    Sleep(200);
    memset(buffer, 0, sizeof(buffer));
    memset(device_input.bytes, 0, sizeof(device_input.bytes));
    device.write(command_poll, 5);
    do
    {
        device.read(device_input.bytes, 64);
    } while ((device_input.cardType != 2 && device_input.cardType != 3) && clock() - beginTime <= 1000);
    switch (device_input.cardType)
    {
    case 0x02:
        if (!felica_direct)
        {
            break;
        }
        memcpy(&command_read_felica_code_aime[2], device_input.felica_IDm, 8);
        beginTime = clock();
        command_read_felica_code_aime[14] = felica_direct ? 0xFF : 0x00;
        device.write(command_read_felica_code_aime, sizeof(command_read_felica_code_aime));
        do
        {
            device.read(buffer, 64);
        } while (buffer[1] != 0x90 && clock() - beginTime <= 1000);
        if (buffer[1] == 0x90)
        {
            memcpy(accessCode, buffer + 7, 10);
            device_input.cardType = 0x03;
        }
        break;

    case 0x03:
        command_read_mifare_code_aime[10] = device_input.length;
        memcpy(&command_read_mifare_code_aime[11], device_input.mifare_UID, device_input.length);
        device.write(command_read_mifare_code_aime, sizeof(command_read_mifare_code_aime));
        do
        {
            device.read(buffer, 64);
        } while (buffer[1] != 0x90 && clock() - beginTime <= 1000);
        if (buffer[1] == 0x90)
        {
            memcpy(accessCode, buffer + 8, 10);
        }
        break;
    }
    return HRESULT(0);
}

HRESULT aime_io_nfc_get_aime_id(uint8_t unit_no, uint8_t* luid, size_t luid_size)
{
    if (device_input.cardType == 0x03)
    {
        memcpy(luid, accessCode, 10);
        luid_size = 10;

        char AccessCode[25] = { 0 };
        char* pAccessCode = AccessCode;
        uint8_t wroteCount = 0;
        for (int i = 0; i < 10; i++)
        {
            char temp[3];
            sprintf_s(temp, "%02X", accessCode[i]);
            if (wroteCount == 4 || wroteCount == 8 || wroteCount == 12 || wroteCount == 16 || wroteCount == 20)
            {
                *pAccessCode = ' ';
                pAccessCode++;
            }
            *pAccessCode = temp[0];
            pAccessCode++;
            wroteCount++;
            if (wroteCount == 4 || wroteCount == 8 || wroteCount == 12 || wroteCount == 16 || wroteCount == 20)
            {
                *pAccessCode = ' ';
                pAccessCode++;
            }
            *pAccessCode = temp[1];
            pAccessCode++;
            wroteCount++;
        }
        Engine::Print("[INFO][Card] New Mifare Classic card detected - %s", AccessCode);
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
    return HRESULT(0);
}

HRESULT aime_io_nfc_get_felica_id(uint8_t unit_no, uint64_t* IDm)
{
    if (device_input.cardType == 2)
    {
        for (int i = 0; i < 8; i++) {
            *IDm = (*IDm << 8) | device_input.felica_IDm[i];
        }
        char IDmString[17] = { 0 };
        char* pIDm = IDmString;
        uint8_t wroteCount = 0;
        for (int i = 0; i < 8; i++)
        {
            char temp[3];
            sprintf_s(temp, "%02X", device_input.felica_IDm[i]);
            *pIDm = temp[0];
            pIDm++;
            *pIDm = temp[1];
            pIDm++;
        }
        Engine::Print("[INFO][Card] New FeliCa card detected - %s", IDmString);
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
    return HRESULT(0);
}

void aime_io_led_set_color(uint8_t unit_no, uint8_t r, uint8_t g, uint8_t b)
{
    device_output.reportID = 0x03;
    device_output.command = 0x01;
    device_output.Light[0] = r;
    device_output.Light[1] = g;
    device_output.Light[2] = b;
    device.write(device_output.bytes, sizeof(device_output.bytes));
}