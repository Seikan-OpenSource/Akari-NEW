/*
 created by Deqing Sun for use with CH55xduino
 */

#include "USBconstant.h"

// Device descriptor
__code USB_Descriptor_Device_t DeviceDescriptor = {
  .Header = { .Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device },

  .USBSpecification = VERSION_BCD(1, 1, 0),
  .Class = 0xEF,  // Miscellaneous
  .SubClass = 0x02,
  .Protocol = 0x01,  // Interface Association Descriptor

  .Endpoint0Size = DEFAULT_ENDP0_SIZE,

  .VendorID = 0x303a,
  .ProductID = 0x81f0,
  .ReleaseNumber = VERSION_BCD(1, 0, 1),

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
              .TotalInterfaces = 3,

              .ConfigurationNumber = 1,
              .ConfigurationStrIndex = NO_DESCRIPTOR,

              .ConfigAttributes = (USB_CONFIG_ATTR_RESERVED),

              .MaxPowerConsumption = USB_CONFIG_POWER_MA(200) },

  .CDC_IAD = { .Header = { .Size =
                             sizeof(USB_Descriptor_Interface_Association_t),
                           .Type = DTYPE_InterfaceAssociation },

               .FirstInterfaceIndex = INTERFACE_ID_CDC_CCI,
               .TotalInterfaces = 2,

               .Class = CDC_CSCP_CDCClass,
               .SubClass = CDC_CSCP_ACMSubclass,
               .Protocol = CDC_CSCP_ATCommandProtocol,

               .IADStrIndex = 4 },

  .CDC_CCI_Interface = { .Header = { .Size = sizeof(USB_Descriptor_Interface_t),
                                     .Type = DTYPE_Interface },

                         .InterfaceNumber = INTERFACE_ID_CDC_CCI,
                         .AlternateSetting = 0,

                         .TotalEndpoints = 1,

                         .Class = CDC_CSCP_CDCClass,
                         .SubClass = CDC_CSCP_ACMSubclass,
                         .Protocol = CDC_CSCP_ATCommandProtocol,

                         .InterfaceStrIndex = 4 },
  // refer to usbcdc11.pdf
  .CDC_Functional_Header = {
    .Header = { .Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
                .Type = CDC_DTYPE_CSInterface },
    .Subtype = CDC_DSUBTYPE_CSInterface_Header,

    .CDCSpecification = VERSION_BCD(1, 1, 0),
  },
  // Todo: check CDC_DSUBTYPE_CSInterface_CallManagement difference?
  .CDC_Functional_ACM = {
    .Header = { .Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t), .Type = CDC_DTYPE_CSInterface }, .Subtype = CDC_DSUBTYPE_CSInterface_ACM,

    .Capabilities = 0x02,  // No Send_Break, Yes  Set_Line_Coding,
                           // Set_Control_Line_State, Get_Line_Coding,
                           // and the notification Serial_State.
  },

  .CDC_Functional_Union = {
    .Header = { .Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t), .Type = CDC_DTYPE_CSInterface },
    .Subtype = CDC_DSUBTYPE_CSInterface_Union,

    .MasterInterfaceNumber = INTERFACE_ID_CDC_CCI,
    .SlaveInterfaceNumber = INTERFACE_ID_CDC_DCI,
  },

  .CDC_NotificationEndpoint = { .Header = { .Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint },

                                .EndpointAddress = CDC_NOTIFICATION_EPADDR,
                                .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                                .EndpointSize = CDC_NOTIFICATION_EPSIZE,
                                .PollingIntervalMS = 0x40 },

  .CDC_DCI_Interface = { .Header = { .Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface },

                         .InterfaceNumber = INTERFACE_ID_CDC_DCI,
                         .AlternateSetting = 0,

                         .TotalEndpoints = 2,

                         .Class = CDC_CSCP_CDCDataClass,
                         .SubClass = CDC_CSCP_NoDataSubclass,
                         .Protocol = CDC_CSCP_NoDataProtocol,

                         .InterfaceStrIndex = 4 },

  .CDC_DataOutEndpoint = { .Header = { .Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint },

                           .EndpointAddress = CDC_RX_EPADDR,
                           .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                           .EndpointSize = CDC_TXRX_EPSIZE,
                           .PollingIntervalMS = 0x00 },

  .CDC_DataInEndpoint = { .Header = { .Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint },

                          .EndpointAddress = CDC_TX_EPADDR,
                          .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                          .EndpointSize = CDC_TXRX_EPSIZE,
                          .PollingIntervalMS = 0x00 },

  .HID_Interface = { .Header = { .Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface },

                     .InterfaceNumber = INTERFACE_ID_HID,
                     .AlternateSetting = 0x00,

                     .TotalEndpoints = 2,

                     .Class = HID_CSCP_HIDClass,
                     .SubClass = HID_CSCP_BootSubclass,
                     .Protocol = HID_CSCP_KeyboardBootProtocol,

                     .InterfaceStrIndex = NO_DESCRIPTOR },

  .HID_Device = { .Header = { .Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID },

                  .HIDSpec = VERSION_BCD(1, 1, 0),
                  .CountryCode = 0x00,
                  .TotalReportDescriptors = 2,
                  .HIDReportType = HID_DTYPE_Report,
                  .HIDReportLength = sizeof(ReportDescriptor) },

  .HID_ReportINEndpoint = { .Header = { .Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint },

                            .EndpointAddress = HID_TX_EPADDR,
                            .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                            .EndpointSize = HID_EPSIZE,
                            .PollingIntervalMS = 10 },

  .HID_ReportOUTEndpoint = { .Header = { .Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint },

                             .EndpointAddress = HID_RX_EPADDR,
                             .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                             .EndpointSize = HID_EPSIZE,
                             .PollingIntervalMS = 10 },
};

// String Descriptors
__code uint8_t LanguageDescriptor[] = { 0x04, 0x03, 0x09,
                                        0x04 };  // Language Descriptor
__code uint16_t SerialDescriptor[] = {
  // Serial String Descriptor
  (((6 + 1) * 2) | (DTYPE_String << 8)),
  'S',
  'E',
  'R',
  'I',
  'A',
  'L',
};
__code uint16_t ProductDescriptor[] = {
  // Produce String Descriptor
  (((10 + 1) * 2) | (DTYPE_String << 8)),
  'A',
  'k',
  'a',
  'r',
  'i',
  ' ',
  'N',
  'E',
  'W',
  '2',
};

__code uint16_t CDCDescriptor[] = {
  (((11 + 1) * 2) | (DTYPE_String << 8)),
  'P',
  'N',
  '5',
  '3',
  '2',
  'D',
  'i',
  'r',
  'e',
  'c',
  't',
};

__code uint16_t ManufacturerDescriptor[] = {
  // SDCC is little endian
  (((6 + 1) * 2) | (DTYPE_String << 8)),
  'S',
  'E',
  'I',
  'K',
  'A',
  'N',
};

__code uint8_t ReportDescriptor[] = {
  0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
  0x09, 0x03,        // Usage (0x03)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x03,        //   Report ID (3)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (-1)
  0x75, 0x08,        //   Report Size (8)
  0x09, 0x01,        //   Usage (0x01)
  0x95, 0x3F,        //   Report Count (7)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x09, 0x02,        //   Usage (0x02)
  0x95, 0x3F,        //   Report Count (63)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection

  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x00,        // Usage (Undefined)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x05, 0x0C,        //   Usage Page (Consumer)
  0x19, 0x69,        //   Usage Minimum (0x69)
  0x29, 0x6B,        //   Usage Maximum (0x6B)
  0x95, 0x03,        //   Report Count (3)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x19, 0x01,        //   Usage Minimum (Consumer Control)
  0x29, 0x03,        //   Usage Maximum (Programmable Buttons)
  0x95, 0x03,        //   Report Count (3)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection

  0x06, 0xCA, 0xFF,  // Usage Page (Vendor Defined 0xFFCA)
  0x09, 0x01,        // Usage (0x01)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x02,        //   Report ID (2)
  0x09, 0x41,        //   Usage (0x41)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (-1)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
};

/*
__code uint8_t ReportDescriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x00,        // Usage (Undefined)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x05, 0x0C,  //   Usage Page (Ordinal)
  0x19, 0x69,  //   Usage Minimum (0x69)
  0x29, 0x6B,  //   Usage Maximum (0x6B)
  0x95, 0x03,  //   Report Count (3)
  0x91, 0x02,  //   Output (Data,Var,Abs)
  0x19, 0x01,  //   Usage Minimum (0x01)
  0x29, 0x03,  //   Usage Maximum (0x03)
  0x95, 0x03,  //   Report Count (3)
  0x81, 0x02,  //   Input (Data,Var,Abs)
  0xC0,        // End Collection
  0x06, 0xCA, 0xFF,  // Usage Page (Vendor Defined 0xFFCA)
  0x09, 0x01,        // Usage (0x01)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x02,        //   Report ID (2)
  0x09, 0x41,        //   Usage (0x41)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0xC0,              // End Collection
  0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFFCA)
  0x09, 0x03,        // Usage (0x03) - 自定义功能3
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x03,        //   Report ID (3)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x09, 0x01,  //   Usage (0x01) - 自定义输出Usage
  0x95, 0x07,  //   Report Count (7)
  0x91, 0x02,  //   Output (Data,Var,Abs)
  0x09, 0x02,  //   Usage (0x02) - 自定义输入Usage
  0x95, 0x3F,  //   Report Count (63)
  0x81, 0x02,  //   Input (Data,Var,Abs)
  0xC0,        // End Collection
};
*/