/*
 created by Deqing Sun for use with CH55xduino
 */

// clang-format off
#include <stdint.h>
#include <stdbool.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
#include "USBconstant.h"
#include "USBhandler.h"
// clang-format on

// clang-format off
extern __xdata __at (EP0_ADDR) uint8_t Ep0Buffer[];
extern __xdata __at (EP2_ADDR) uint8_t Ep2Buffer[];
// clang-format on

#define LINE_CODEING_SIZE 7
__xdata uint8_t LineCoding[LINE_CODEING_SIZE] = {
  0x00, 0xe1, 0x00, 0x00,
  0x00, 0x00, 0x08
};  // Initialize for baudrate 57600, 1 stopbit, No parity,
    // eight data bits

volatile __xdata uint8_t USBByteCountEP2 =
  0;                                             // Bytes of received data on USB endpoint
volatile __xdata uint8_t USBBufOutPointEP2 = 0;  // Data pointer for fetching

volatile __bit UpPoint2BusyFlag = 0;  // Flag of whether upload pointer is busy
volatile __xdata uint8_t controlLineState = 1;

__xdata uint8_t usbWritePointer = 0;

void delayMicroseconds(__data uint16_t us);

void USBInit() {
  USBDeviceCfg();          // Device mode configuration
  USBDeviceEndPointCfg();  // Endpoint configuration
  USBDeviceIntCfg();       // Interrupt configuration
}

void resetCDCParameters() {

  USBByteCountEP2 = 0;  // Bytes of received data on USB endpoint
  UpPoint2BusyFlag = 0;
}

void setLineCodingHandler() {
  for (__data uint8_t i = 0;
       i < ((LINE_CODEING_SIZE <= USB_RX_LEN) ? LINE_CODEING_SIZE : USB_RX_LEN);
       i++) {
    LineCoding[i] = Ep0Buffer[i];
  }

  //!!!!!Config_Uart0(LineCoding);
}

uint16_t getLineCodingHandler() {
  __data uint16_t returnLen;

  returnLen = LINE_CODEING_SIZE;
  for (__data uint8_t i = 0; i < returnLen; i++) {
    Ep0Buffer[i] = LineCoding[i];
  }

  return returnLen;
}

void setControlLineStateHandler() {
  //DTR CHANGED THERE
  //controlLineState = Ep0Buffer[2];
  controlLineState = 1;
  // We check DTR state to determine if host port is open (bit 0 of lineState).
}

uint8_t USBSerial_wait_UpPoint2BusyFlag_clear() {
  __data uint16_t waitWriteCount = 0;
  while (UpPoint2BusyFlag) {  // wait for 250ms or give up, on my mac it takes
                              // about 256us
    waitWriteCount++;
    delayMicroseconds(5);
    if (waitWriteCount >= 50000)
      return 0;
  }
  return 1;
}

bool USBSerial() {
  __data bool result = false;
  if (controlLineState > 0)
    result = true;
  // delay(10); not doing it for now
  return result;
}

void USBSerial_flush(void) {
  if (!UpPoint2BusyFlag && usbWritePointer > 0) {
    UEP2_T_LEN = usbWritePointer;
    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;  // Respond ACK
    UpPoint2BusyFlag = 1;

    if (usbWritePointer == MAX_PACKET_SIZE) {  // write empty packet for end transmission. Needed
                                               // for windows.
      if (USBSerial_wait_UpPoint2BusyFlag_clear()) {
        UEP2_T_LEN = 0;
        UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;  // Respond ACK
        UpPoint2BusyFlag = 1;
      }
    }
    usbWritePointer = 0;
  }
}

uint8_t USBSerial_write(__data char c) {  // 3 bytes generic pointer
  if (controlLineState > 0) {
    while (true) {
      if (USBSerial_wait_UpPoint2BusyFlag_clear() == 0)
        return 0;
      if (usbWritePointer < MAX_PACKET_SIZE) {
        Ep2Buffer[MAX_PACKET_SIZE + usbWritePointer] = c;
        usbWritePointer++;
        return 1;
      } else {
        USBSerial_flush();  // go back to first while
      }
    }
  }
  return 0;
}

uint8_t USBSerial_available() {
  return USBByteCountEP2;
}

char USBSerial_read() {
  if (USBByteCountEP2 == 0)
    return 0;
  __data char data = Ep2Buffer[USBBufOutPointEP2];
  USBBufOutPointEP2++;
  USBByteCountEP2--;
  if (USBByteCountEP2 == 0) {
    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
  }
  return data;
}

void USB_EP2_IN() {
  UEP2_T_LEN = 0;  // No data to send anymore
  UEP2_CTRL =
    UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;  // Respond NAK by default
  UpPoint2BusyFlag = 0;                           // Clear busy flag
}

void USB_EP2_OUT() {
  if (U_TOG_OK)  // Discard unsynchronized packets
  {
    USBByteCountEP2 = USB_RX_LEN;
    USBBufOutPointEP2 = 0;  // Reset Data pointer for fetching
    if (USBByteCountEP2)
      UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;  // Respond NAK after a packet. Let main code
                                                                // change response after handling.
  }
}
