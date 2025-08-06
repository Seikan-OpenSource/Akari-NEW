#include "USBconstant.h"
#define NUMBER_OF_LIGHT 5

// Device descriptor
__code USB_Descriptor_Device_t DeviceDescriptor = {
  .Header = { .Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device },

  .USBSpecification = VERSION_BCD(1, 1, 0),
  .Class = 0x00,
  .SubClass = 0x00,
  .Protocol = 0x00,

  .Endpoint0Size = KEYBOARD_EPSIZE,

  .VendorID = 0x303A,
  .ProductID = 0x81F0,
  .ReleaseNumber = VERSION_BCD(1, 0, 0),

  .ManufacturerStrIndex = 1,
  .ProductStrIndex = 2,
  .SerialNumStrIndex = 3,

  .NumberOfConfigurations = 1
};

/** Configuration descriptor structure. This descriptor, located in FLASH
 * memory, describes the usage of the device in one of its supported
 * configurations, including information about any device interfaces and
 * endpoints. The descriptor is read out by the USB host during the enumeration
 * process when selecting a configuration so that the host may correctly
 * communicate with the USB device.
 */
__code USB_Descriptor_Configuration_t ConfigurationDescriptor = {
  .Config = { .Header = { .Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration },

              .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
              .TotalInterfaces = 1,

              .ConfigurationNumber = 1,
              .ConfigurationStrIndex = NO_DESCRIPTOR,

              .ConfigAttributes = (USB_CONFIG_ATTR_RESERVED),

              .MaxPowerConsumption = USB_CONFIG_POWER_MA(200) },

  .HID_Interface = { .Header = { .Size = sizeof(USB_Descriptor_Interface_t),
                                 .Type = DTYPE_Interface },

                     .InterfaceNumber = 0,
                     .AlternateSetting = 0x00,

                     .TotalEndpoints = 2,

                     .Class = 0x03,
                     .SubClass = 0x00,
                     .Protocol = 0x00,

                     .InterfaceStrIndex = 0x00 },

  .HID_KeyboardHID = { .Header = { .Size = sizeof(USB_HID_Descriptor_HID_t),
                                   .Type = HID_DTYPE_HID },

                       .HIDSpec = VERSION_BCD(1, 1, 0),
                       .CountryCode = 0x00,
                       .TotalReportDescriptors = 1,
                       .HIDReportType = HID_DTYPE_Report,
                       .HIDReportLength = sizeof(ReportDescriptor) },

  .HID_ReportINEndpoint = { .Header = { .Size =
                                          sizeof(USB_Descriptor_Endpoint_t),
                                        .Type = DTYPE_Endpoint },

                            .EndpointAddress = KEYBOARD_EPADDR,
                            .Attributes =
                              (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                            .EndpointSize = KEYBOARD_EPSIZE,
                            .PollingIntervalMS = 10 },

  .HID_ReportOUTEndpoint = {.Header = {.Size =
                                             sizeof(USB_Descriptor_Endpoint_t),
                                         .Type = DTYPE_Endpoint},

                              .EndpointAddress = KEYBOARD_LED_EPADDR,
                              .Attributes =
                                  (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                                   ENDPOINT_USAGE_DATA),
                              .EndpointSize = KEYBOARD_EPSIZE,
                              .PollingIntervalMS = 10},
};

__code uint8_t ReportDescriptor[] = {

  0x06, 0xCA, 0xFF,  // Usage Page (Vendor Defined 0xFFCA)
  0x09, 0x01,        // Usage (0x01)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x02,        //   Report ID (2)
  0x06, 0xCA, 0xFF,  //   Usage Page (Vendor Defined 0xFFCA)
  0x09, 0x41,        //   Usage (0x41)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (-1)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
  
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x00,        // Usage (Undefined)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x95, 0x0B,        //   Report Count (11)
  0x75, 0x08,        //   Report Size (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x05, 0x0c,        //   Usage Page (Ordinal)
  0x19, 0x69,        //   Usage Minimum (0x01)
  0x29, 0x6B,        //   Usage Maximum (0x03)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x03,        //   Usage Maximum (0x03)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  
  0x85, 0x03,        //   Report ID (3)
  0x95, 0x10,        //   Report Count (16)
  0x75, 0x08,        //   Report Size (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x05, 0x00,        //   Usage Page (Undefined)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x03,        //   Usage Maximum (0x03)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x03,        //   Usage Maximum (0x03)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
};

// String Descriptors
__code uint8_t LanguageDescriptor[] = { 0x04, 0x03, 0x09,
                                        0x04 };  // Language Descriptor
__code uint16_t SerialDescriptor[] = {           // Serial String Descriptor
  (((9 + 1) * 2) | (DTYPE_String << 8)),
  'A',
  'k',
  'a',
  'r',
  'i',
  ' ',
  'N',
  'E',
  'W'
};
__code uint16_t ProductDescriptor[] = {
  // Produce String Descriptor
  (((9 + 1) * 2) | (DTYPE_String << 8)),
  'A', 'k', 'a', 'r', 'i', ' ', 'N', 'E', 'W'
};
__code uint16_t ManufacturerDescriptor[] = {
  // SDCC is little endian
  (((6 + 1) * 2) | (DTYPE_String << 8)), 'S', 'E', 'I', 'K', 'A', 'N'
};