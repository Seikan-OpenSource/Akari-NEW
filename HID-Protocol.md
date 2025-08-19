# Config Hex Map

| | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | BOOT LIGHT SW | BOOT MODE | FeliCa Direct Read
| 1 |

# Verb Table

| NAME | VALUE |
| --- | --- |
| USB_VID | 0x303A |
| USB_PID | 0x81F0 |
| DEVICE_MODE_API |0x00|
| DEVICE_MODE_HID |0x01|
| DEVICE_MODE_CDC |0x02|
| INS_REBOOT_TO_DFU |0xFF|
| INS_SET_LED |0x01|
| INS_DECRYPT_ACCESS_CODE |0x02|
| INS_READ_CONFIG |0x03|
| INS_WRITE_CONFIG |0x04|
| INS_POLLING |0x05|
| INS_READ_MIFARE_BLOCK |0x06|
| INS_WRITE_MIFARE_BLOCK |0x07|
| INS_READ_FELICA_BLOCK |0x08|
| CFG_BOOT_LIGHT_SW |0x00|
| CFG_BOOT_MODE |0x01|
| SW_SUCCESS |0x90|
| SW_NO_CARD |0x80|
| SW_AUTH_FAILED |0x81|
| SW_READ_FAILED |0x82|
| SW_WRITE_FAILED |0x83|
| SW_UNKNOWN_INS |0x70|
| CARD_TYPE_FELICA |0x02|
| CARD_TYPE_ISO14443A |0x03|
| CARD_TYPE_NOCARD |0x01|

# HID APDU Commands

| Report ID | IN | OUT |
| --- | --- | --- |
 | 0x03 | URB_INTERRUPT In | HID OUT |
## INS_READ_CONFIG

### Request:
| POS | 00 | 01 |
| --- | --- | --- |
| HEX | Report ID | INS |

### Response:
| POS | 02-33 (32Byte) |
| --- | --- |
| HEX | CONFIG DATA |

## INS_WRITE_CONFIG

### Request:
| POS | 00 | 01 | 02-33 (32Byte) |
| --- | --- | --- | --- |
| HEX | Report ID | INS | NEW CONFIG DATA |


### Response:
| POS | 00 | 01 |
| --- | --- | -- |
| HEX | 90 | 00 |

## INS_DECRYPT_ACCESS_CODE

### Request:
| POS | 00 | 01 | 02-17 (16Byte) |
| --- | --- | --- | --- |
| HEX | Report ID | INS | Encrypted Access Code |

### Response:
| POS | 00-15 (16Byte) |
| --- | --- |
| HEX | Access Code |

## INS_REBOOT_TO_DFU

### Request:
| POS | 00 | 01 |
| --- | --- | --- |
| HEX | Report ID | INS |

### Response:
No Response (Device will restart into DFU Mode immediately)

## INS_SET_LED

### Request:
| POS | 00 | 01 | 02 | 03 | 04
| --- | --- | --- | --- | --- | --- |
| HEX | Report ID | INS | R | G | B |

### Response:
| POS | 00 | 01 |
| --- | --- | --- |
| HEX | 90 | 00 |

## INS_POLLING

### Request:
| POS | 00 | 01 | 02 | 03 | 04 |
| --- | --- | --- | --- | --- | --- |
| HEX | Report ID | INS | FELICA SYSTEM CODE LOW | FELICA SYSTEM CODE HIGH | FELICA REQUEST CODE |
| e.g | 03 | INS | FF | FF | 01

### Response:
| POS | 00 | 01 | 02 >> |
| --- | --- | --- | --- |
| HEX | STATUS | LEN | DATA |

#### STATUS:
| NO_CARD | FELICA | ISO14443A |
| --- | --- | --- |
| 0x01 | 0x02 | 0x03 |
#### FELICA DATA:

| POS | 00 | 01 | 02-09 | 10-17 | 18-19 |
| --- | --- | --- | --- | --- | --- |
| HEX | STATUS | LEN(0x12) | IDm | PMm | System Code (when Request Code = 0x01) |

#### ISO14443A DATA:

| POS | 00 | 01 | 02 >> |
| --- | --- | --- | --- |
| HEX | STATUS | LEN | UID |

## INS_READ_MIFARE_BLOCK

> ⚠️ Require: Send INS_POLLING Command first & within 100ms 

### Request:
| POS | 00 | 01 | 02 | 03 | 04-09 | 10 | 11 >> |
| --- | --- | --- | --- | --- | --- | --- | --- |
| HEX | Report ID | INS | KEY TYPE | BLOCK NUMBER | KEY | UID LEN | UID |

#### KEY TYPE:
| KEY_A | KEY_B |
| --- | --- |
| 0x60 | 0x61 | 

### Response:
| POS | 00 | 01-16 |
| --- | --- | --- |
| HEX | SW | DATA |


## INS_WRITE_MIFARE_BLOCK

> ⚠️ Require: Send INS_POLLING Command first & within 100ms 

### Request:
| POS | 00 | 01 | 02 | 03 | 04-09 | 10 | 11 >> |
| --- | --- | --- | --- | --- | --- | --- | --- |
| HEX | Report ID | INS | KEY TYPE | BLOCK NUMBER | KEY | UID LEN | UID |

#### KEY TYPE:
| KEY_A | KEY_B |
| --- | --- |
| 0x60 | 0x61 | 

### Response:
| POS | 00 |
| --- | --- |
| HEX | SW |

## INS_READ_FELICA_BLOCK

> ⚠️ Require: Send INS_POLLING Command first & within 100ms  

### Request:
| POS | 00 | 01 | 02-09 | 10 | 11 | 12 | 13 | 14 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| HEX | Report ID | INS | IDm | Service Code Low | Service Code High | Block High | Block Low | Auto Decrypt Access Code |
| e.g | 0x03 | INS | 0x01 0x2e 0x4c 0xe4 0x45 0x01 0x64 0xa1 | 0x0B | 0x00 |  0x80 | 0x00 | 0xFF | 0xFF | 0x01 | 0x01 |

#### For AIC Access Code Aera:
| Service Code | Block |
| --- | --- |
| 0x000B | 0x8000 |

### Response:
| POS | 00 | 01-16 |
| --- | --- | --- |
| HEX | SW | DATA |