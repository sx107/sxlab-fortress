/**
  ******************************************************************************
  * @file    usbd_cdc.c
  * @author  MCD Application Team
  * @brief   This file provides the high layer firmware functions to manage the
  *          following functionalities of the USB CDC Class:
  *           - Initialization and Configuration of high and low layer
  *           - Enumeration as CDC Device (and enumeration for each implemented memory interface)
  *           - OUT/IN data transfer
  *           - Command IN transfer (class requests management)
  *           - Error management
  *
  *  @verbatim
  *
  *          ===================================================================
  *                                CDC Class Driver Description
  *          ===================================================================
  *           This driver manages the "Universal Serial Bus Class Definitions for Communications Devices
  *           Revision 1.2 November 16, 2007" and the sub-protocol specification of "Universal Serial Bus
  *           Communications Class Subclass Specification for PSTN Devices Revision 1.2 February 9, 2007"
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as CDC device with 2 data endpoints (IN and OUT) and 1 command endpoint (IN)
  *             - Requests management (as described in section 6.2 in specification)
  *             - Abstract Control Model compliant
  *             - Union Functional collection (using 1 IN endpoint for control)
  *             - Data interface class
  *
  *           These aspects may be enriched or modified for a specific user application.
  *
  *            This driver doesn't implement the following aspects of the specification
  *            (but it is possible to manage these features with some modifications on this driver):
  *             - Any class-specific aspect relative to communication classes should be managed by user application.
  *             - All communication classes other than PSTN are not managed
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc.h"
#include "usbd_ctlreq.h"
#include <drv/midi.h>

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_CDC
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_CDC_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_CDC_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_CDC_Private_Macros
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_CDC_Private_FunctionPrototypes
  * @{
  */


static uint8_t  USBD_CDC_Init(USBD_HandleTypeDef *pdev,
                              uint8_t cfgidx);

static uint8_t  USBD_CDC_DeInit(USBD_HandleTypeDef *pdev,
                                uint8_t cfgidx);

static uint8_t  USBD_CDC_Setup(USBD_HandleTypeDef *pdev,
                               USBD_SetupReqTypedef *req);

static uint8_t  USBD_CDC_DataIn(USBD_HandleTypeDef *pdev,
                                uint8_t epnum);

static uint8_t  USBD_CDC_DataOut(USBD_HandleTypeDef *pdev,
                                 uint8_t epnum);

static uint8_t  USBD_CDC_EP0_RxReady(USBD_HandleTypeDef *pdev);

static uint8_t  *USBD_CDC_GetFSCfgDesc(uint16_t *length);

static uint8_t  *USBD_CDC_GetHSCfgDesc(uint16_t *length);

static uint8_t  *USBD_CDC_GetOtherSpeedCfgDesc(uint16_t *length);

static uint8_t  *USBD_CDC_GetOtherSpeedCfgDesc(uint16_t *length);

uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor(uint16_t *length);

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CDC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/**
  * @}
  */

/** @defgroup USBD_CDC_Private_Variables
  * @{
  */


/* CDC interface class callbacks structure */
USBD_ClassTypeDef  USBD_CDC =
{
  USBD_CDC_Init,
  USBD_CDC_DeInit,
  USBD_CDC_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_CDC_EP0_RxReady,
  USBD_CDC_DataIn,
  USBD_CDC_DataOut,
  NULL,
  NULL,
  NULL,
  USBD_CDC_GetHSCfgDesc,
  USBD_CDC_GetFSCfgDesc,
  USBD_CDC_GetOtherSpeedCfgDesc,
  USBD_CDC_GetDeviceQualifierDescriptor,
};

#define USBD_MAX_POWER 0x32

/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_CDC_CfgHSDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
		  /* B.2 Configuration Descriptor */
		  0x09,                                       /* bLength: Configuration Descriptor size */
		  USB_DESC_TYPE_CONFIGURATION,                /* bDescriptorType: CONFIGURATION descripto */  // - 0x02
		  LOBYTE(USB_CDC_CONFIG_DESC_SIZ),            /* wTotalLength */ // посчитать
		  HIBYTE(USB_CDC_CONFIG_DESC_SIZ),
		  0x02,                                       /* bNumInterfaces: 2 interfaces */
		  0x01,                                       /* bConfigurationValue: ID of this configuration */
		  0x00,                                       /* iConfiguration: Unused */
		  0x80,                                       /* bmAttributes: Bus Powered device, not Self Powered, no Remote wakeup capability */
		  USBD_MAX_POWER,                             /* MaxPower: 100 mA */ // - 0x32
		  /* 09 byte */

		  /*---------------------------------------------------------------------------*/

		  /* B.3.1 Standard AC Interface Descriptor */
		  0x09,                                       /* bLength: Size of this descriptor, in bytes */
		  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: INTERFACE descriptor */ // - 0x04
		  0x00,                                       /* bInterfaceNumber: Index of this interface */
		  0x00,                                       /* bAlternateSetting: Index of this setting */
		  0x00,                                       /* bNumEndpoints: 0 endpoints */
		  0x01,                                       /* bInterfaceClass: AUDIO */
		  0x01,                                       /* bInterfaceSubClass: AUDIO_CONTROL */
		  0x00,                                       /* bInterfaceProtocol: Unused */
		  0x00,                                       /* iInterface: Unused */
		  /* 09 byte */

		  /* B.3.2 Class-specific AC Interface Descriptor */
		  0x09,                                       /* bLength: Size of this descriptor, in bytes */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE */
		  0x01,                                       /* bDescriptorSubtype: HEADER subtype */
		  0x00,                                       /* bcdCDC: Revision of class specification - 1.0 */
		  0x01,
		  0x09,										  /* wTotalLength: Total size of class specific descriptors */
		  0x00,
		  0x01,                                 	  /* bInCollection: Number of streaming interfaces */
		  0x01,                                 	  /* baInterfaceNr: MIDIStreaming interface 1 belongs to this AudioControl interface */
		  /* 09 byte */

		  /*B.4.1 Standard MS Interface Descriptor*/
		  0x09,                                       /* bLength */
		  0x04,                                       /* bDescriptorType: INTERFACE descriptor */
		  0x01,                                       /* bInterfaceNumber: Index of this interface */
		  0x00,                                       /* bAlternateSetting:Index of this alternate setting */
		  0x02,                                       /* bNumEndpoints: 2 endpoints */
		  0x01,							      		  /* bInterfaceClass: AUDIO */
		  0x03,										  /* bInterfaceSubClass: MIDISTREAMING */
		  0x00,										  /* bInterfaceProtocol: Unused */
		  0x00,										  /* iInterface: Unused */
		  /* 09 byte */


		  /*================================================================================================*/
		  /* B.4.2 Class-specific MS Interface Descriptor */
		  0x07,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE */
		  0x01,                                       /* bDescriptorSubtype: MS_HEADER subtype */
		  0x00,                                       /* bcdADC: Revision of this class specification - 1.0 */
		  0x01,
		  0x37,										  /* wTotalLength: */ // !41 or 37! ??????????????????????????????????????????
		  0x00,										  /* -Total size of class-specific descriptor (37) */
		  /* 07 byte */

		  /* B.4.3 MIDI Adapter MIDI IN Jack Descriptor (Embedded) */
		  0x06,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x02,                                       /* bDescriptorSubtype: MIDI_IN_JACK subtype */
		  0x01,                                       /* bJackType: EMBEDDED */
		  0x01,                                       /* bJackID: ID of this Jack */
		  0x00,										  /* iJack: Unused */
		  /* 06 byte */

		  /* B.4.3 MIDI Adapter MIDI IN Jack Descriptor (External) */
		  0x06,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x02,                                       /* bDescriptorSubtype: MIDI_IN_JACK subtype */
		  0x02,                                       /* bJackType: EXTERNAL */
		  0x02,                                       /* bJackID: ID of this Jack */
		  0x00,										  /* iJack: Unused */
		  /* 06 byte */

		  /* B.4.4 MIDI Adapter MIDI OUT Jack Descriptor (Embedded) */
		  0x09,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x03,                                       /* bDescriptorSubtype: MIDI_OUT_JACK subtype */
		  0x01,                                       /* bJackType: EMBEDDED */
		  0x03,                                       /* bJackID: ID of this Jack */
		  0x01,										  /* bNrInputPins: Number of Input Pins of this Jack */
		  0x02,										  /* BaSourceID(1): ID of the Entity to which this Pin is connected*/
		  0x01,										  /* BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected */
		  0x00,										  /* iJack: Unused */
		  /* 09 byte */

		  /* B.4.4 MIDI Adapter MIDI OUT Jack Descriptor (External) */
		  0x09,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x03,                                       /* bDescriptorSubtype: MIDI_OUT_JACK subtype */
		  0x02,                                       /* bJackType: EXTERNAL */
		  0x04,                                       /* bJackID: ID of this Jack */
		  0x01,										  /* bNrInputPins: Number of Input Pins of this Jack */
		  0x01,										  /* BaSourceID(1): ID of the Entity to which this Pin is connected*/
		  0x01,										  /* BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected */
		  0x00,										  /* iJack: Unused */
		  /* 09 byte */

		  /* 37 byte */ /* "wTotalLength" in B.4.2 Class-specific MS Interface Descriptor */
		  /*================================================================================================*/

		  /* B.5.1 Standard Bulk OUT Endpoint Descriptor */
		  0x09,                                       /* bLength */
		  0x05,                                       /* bDescriptorType: ENDPOINT descriptor */
		  0x01,                              	      /* bEndpointAddress: OUT Endpoint 1 */
		  0x02,                                       /* bmAttributes: Bulk, not shared */
		  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize */ // 64
		  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
		  0x00,										  /* bInterval: Ignored for Bulk. Set to zero */
		  0x00,										  /* bRefresh: Unused */
		  0x00,										  /* bSynchAddress: Unused */
		  /* 09 byte */

		  /* B.5.2 Class-specific MS Bulk OUT Endpoint Descriptor */
		  0x05,                                       /* bLength */
		  0x25,                                       /* bDescriptorType: CS_ENDPOINT descriptor */
		  0x01,                                       /* bDescriptorSubtype: MS_GENERAL subtype */
		  0x01,										  /* bNumEmbMIDIJack: Number of embedded MIDI IN Jacks */
		  0x01,										  /* BaAssocJackID(1): ID of the Embedded MIDI IN Jack */
		  /* 05 byte */

		  /* B.6.1 Standard Bulk IN Endpoint Descriptor*/
		  0x09,                                       /* bLength */
		  0x05,                                       /* bDescriptorType: ENDPOINT descriptor */
		  0x81,                              	      /* bEndpointAddress: IN Endpoint 1 */
		  0x02,                                       /* bmAttributes: Bulk, not shared */
		  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize */ // 64
		  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
		  0x00,										  /* bInterval: Ignored for Bulk. Set to zero */
		  0x00,										  /* bRefresh: Unused */
		  0x00,										  /* bSynchAddress: Unused */
		  /* 09 byte */

		  /* B.6.2 Class-specific MS Bulk IN Endpoint Descriptor */
		  0x05,                                       /* bLength */
		  0x25,                                       /* bDescriptorType: CS_ENDPOINT descriptor */
		  0x01,                                       /* bDescriptorSubtype: MS_GENERAL subtype */
		  0x01,										  /* bNumEmbMIDIJack: Number of embedded MIDI OUT Jacks */
		  0x03,										  /* BaAssocJackID(1): ID of the Embedded MIDI OUT Jack */
		  /* 05 byte */

} ;


/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_CDC_CfgFSDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
		  /* B.2 Configuration Descriptor */
		  0x09,                                       /* bLength: Configuration Descriptor size */
		  USB_DESC_TYPE_CONFIGURATION,                /* bDescriptorType: CONFIGURATION descripto */  // - 0x02
		  LOBYTE(USB_CDC_CONFIG_DESC_SIZ),            /* wTotalLength */ // посчитать
		  HIBYTE(USB_CDC_CONFIG_DESC_SIZ),
		  0x02,                                       /* bNumInterfaces: 2 interfaces */
		  0x01,                                       /* bConfigurationValue: ID of this configuration */
		  0x00,                                       /* iConfiguration: Unused */
		  0x80,                                       /* bmAttributes: Bus Powered device, not Self Powered, no Remote wakeup capability */
		  USBD_MAX_POWER,                             /* MaxPower: 100 mA */ // - 0x32
		  /* 09 byte */

		  /*---------------------------------------------------------------------------*/

		  /* B.3.1 Standard AC Interface Descriptor */
		  0x09,                                       /* bLength: Size of this descriptor, in bytes */
		  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: INTERFACE descriptor */ // - 0x04
		  0x00,                                       /* bInterfaceNumber: Index of this interface */
		  0x00,                                       /* bAlternateSetting: Index of this setting */
		  0x00,                                       /* bNumEndpoints: 0 endpoints */
		  0x01,                                       /* bInterfaceClass: AUDIO */
		  0x01,                                       /* bInterfaceSubClass: AUDIO_CONTROL */
		  0x00,                                       /* bInterfaceProtocol: Unused */
		  0x00,                                       /* iInterface: Unused */
		  /* 09 byte */

		  /* B.3.2 Class-specific AC Interface Descriptor */
		  0x09,                                       /* bLength: Size of this descriptor, in bytes */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE */
		  0x01,                                       /* bDescriptorSubtype: HEADER subtype */
		  0x00,                                       /* bcdCDC: Revision of class specification - 1.0 */
		  0x01,
		  0x09,										  /* wTotalLength: Total size of class specific descriptors */
		  0x00,
		  0x01,                                 	  /* bInCollection: Number of streaming interfaces */
		  0x01,                                 	  /* baInterfaceNr: MIDIStreaming interface 1 belongs to this AudioControl interface */
		  /* 09 byte */

		  /*B.4.1 Standard MS Interface Descriptor*/
		  0x09,                                       /* bLength */
		  0x04,                                       /* bDescriptorType: INTERFACE descriptor */
		  0x01,                                       /* bInterfaceNumber: Index of this interface */
		  0x00,                                       /* bAlternateSetting:Index of this alternate setting */
		  0x02,                                       /* bNumEndpoints: 2 endpoints */
		  0x01,							      		  /* bInterfaceClass: AUDIO */
		  0x03,										  /* bInterfaceSubClass: MIDISTREAMING */
		  0x00,										  /* bInterfaceProtocol: Unused */
		  0x00,										  /* iInterface: Unused */
		  /* 09 byte */


		  /*================================================================================================*/
		  /* B.4.2 Class-specific MS Interface Descriptor */
		  0x07,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE */
		  0x01,                                       /* bDescriptorSubtype: MS_HEADER subtype */
		  0x00,                                       /* bcdADC: Revision of this class specification - 1.0 */
		  0x01,
		  0x37,										  /* wTotalLength: */ // !41 or 37! ??????????????????????????????????????????
		  0x00,										  /* -Total size of class-specific descriptor (37) */
		  /* 07 byte */

		  /* B.4.3 MIDI Adapter MIDI IN Jack Descriptor (Embedded) */
		  0x06,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x02,                                       /* bDescriptorSubtype: MIDI_IN_JACK subtype */
		  0x01,                                       /* bJackType: EMBEDDED */
		  0x01,                                       /* bJackID: ID of this Jack */
		  0x00,										  /* iJack: Unused */
		  /* 06 byte */

		  /* B.4.3 MIDI Adapter MIDI IN Jack Descriptor (External) */
		  0x06,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x02,                                       /* bDescriptorSubtype: MIDI_IN_JACK subtype */
		  0x02,                                       /* bJackType: EXTERNAL */
		  0x02,                                       /* bJackID: ID of this Jack */
		  0x00,										  /* iJack: Unused */
		  /* 06 byte */

		  /* B.4.4 MIDI Adapter MIDI OUT Jack Descriptor (Embedded) */
		  0x09,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x03,                                       /* bDescriptorSubtype: MIDI_OUT_JACK subtype */
		  0x01,                                       /* bJackType: EMBEDDED */
		  0x03,                                       /* bJackID: ID of this Jack */
		  0x01,										  /* bNrInputPins: Number of Input Pins of this Jack */
		  0x02,										  /* BaSourceID(1): ID of the Entity to which this Pin is connected*/
		  0x01,										  /* BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected */
		  0x00,										  /* iJack: Unused */
		  /* 09 byte */

		  /* B.4.4 MIDI Adapter MIDI OUT Jack Descriptor (External) */
		  0x09,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x03,                                       /* bDescriptorSubtype: MIDI_OUT_JACK subtype */
		  0x02,                                       /* bJackType: EXTERNAL */
		  0x04,                                       /* bJackID: ID of this Jack */
		  0x01,										  /* bNrInputPins: Number of Input Pins of this Jack */
		  0x01,										  /* BaSourceID(1): ID of the Entity to which this Pin is connected*/
		  0x01,										  /* BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected */
		  0x00,										  /* iJack: Unused */
		  /* 09 byte */

		  /* 37 byte */ /* "wTotalLength" in B.4.2 Class-specific MS Interface Descriptor */
		  /*================================================================================================*/

		  /* B.5.1 Standard Bulk OUT Endpoint Descriptor */
		  0x09,                                       /* bLength */
		  0x05,                                       /* bDescriptorType: ENDPOINT descriptor */
		  0x01,                              	      /* bEndpointAddress: OUT Endpoint 1 */
		  0x02,                                       /* bmAttributes: Bulk, not shared */
		  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize */ // 64
		  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
		  0x00,										  /* bInterval: Ignored for Bulk. Set to zero */
		  0x00,										  /* bRefresh: Unused */
		  0x00,										  /* bSynchAddress: Unused */
		  /* 09 byte */

		  /* B.5.2 Class-specific MS Bulk OUT Endpoint Descriptor */
		  0x05,                                       /* bLength */
		  0x25,                                       /* bDescriptorType: CS_ENDPOINT descriptor */
		  0x01,                                       /* bDescriptorSubtype: MS_GENERAL subtype */
		  0x01,										  /* bNumEmbMIDIJack: Number of embedded MIDI IN Jacks */
		  0x01,										  /* BaAssocJackID(1): ID of the Embedded MIDI IN Jack */
		  /* 05 byte */

		  /* B.6.1 Standard Bulk IN Endpoint Descriptor*/
		  0x09,                                       /* bLength */
		  0x05,                                       /* bDescriptorType: ENDPOINT descriptor */
		  0x81,                              	      /* bEndpointAddress: IN Endpoint 1 */
		  0x02,                                       /* bmAttributes: Bulk, not shared */
		  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize */ // 64
		  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
		  0x00,										  /* bInterval: Ignored for Bulk. Set to zero */
		  0x00,										  /* bRefresh: Unused */
		  0x00,										  /* bSynchAddress: Unused */
		  /* 09 byte */

		  /* B.6.2 Class-specific MS Bulk IN Endpoint Descriptor */
		  0x05,                                       /* bLength */
		  0x25,                                       /* bDescriptorType: CS_ENDPOINT descriptor */
		  0x01,                                       /* bDescriptorSubtype: MS_GENERAL subtype */
		  0x01,										  /* bNumEmbMIDIJack: Number of embedded MIDI OUT Jacks */
		  0x03,										  /* BaAssocJackID(1): ID of the Embedded MIDI OUT Jack */
		  /* 05 byte */

} ;

__ALIGN_BEGIN uint8_t USBD_CDC_OtherSpeedCfgDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
		  /* B.2 Configuration Descriptor */
		  0x09,                                       /* bLength: Configuration Descriptor size */
		  USB_DESC_TYPE_CONFIGURATION,                /* bDescriptorType: CONFIGURATION descripto */  // - 0x02
		  LOBYTE(USB_CDC_CONFIG_DESC_SIZ),            /* wTotalLength */ // посчитать
		  HIBYTE(USB_CDC_CONFIG_DESC_SIZ),
		  0x02,                                       /* bNumInterfaces: 2 interfaces */
		  0x01,                                       /* bConfigurationValue: ID of this configuration */
		  0x00,                                       /* iConfiguration: Unused */
		  0x80,                                       /* bmAttributes: Bus Powered device, not Self Powered, no Remote wakeup capability */
		  USBD_MAX_POWER,                             /* MaxPower: 100 mA */ // - 0x32
		  /* 09 byte */

		  /*---------------------------------------------------------------------------*/

		  /* B.3.1 Standard AC Interface Descriptor */
		  0x09,                                       /* bLength: Size of this descriptor, in bytes */
		  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: INTERFACE descriptor */ // - 0x04
		  0x00,                                       /* bInterfaceNumber: Index of this interface */
		  0x00,                                       /* bAlternateSetting: Index of this setting */
		  0x00,                                       /* bNumEndpoints: 0 endpoints */
		  0x01,                                       /* bInterfaceClass: AUDIO */
		  0x01,                                       /* bInterfaceSubClass: AUDIO_CONTROL */
		  0x00,                                       /* bInterfaceProtocol: Unused */
		  0x00,                                       /* iInterface: Unused */
		  /* 09 byte */

		  /* B.3.2 Class-specific AC Interface Descriptor */
		  0x09,                                       /* bLength: Size of this descriptor, in bytes */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE */
		  0x01,                                       /* bDescriptorSubtype: HEADER subtype */
		  0x00,                                       /* bcdCDC: Revision of class specification - 1.0 */
		  0x01,
		  0x09,										  /* wTotalLength: Total size of class specific descriptors */
		  0x00,
		  0x01,                                 	  /* bInCollection: Number of streaming interfaces */
		  0x01,                                 	  /* baInterfaceNr: MIDIStreaming interface 1 belongs to this AudioControl interface */
		  /* 09 byte */

		  /*B.4.1 Standard MS Interface Descriptor*/
		  0x09,                                       /* bLength */
		  0x04,                                       /* bDescriptorType: INTERFACE descriptor */
		  0x01,                                       /* bInterfaceNumber: Index of this interface */
		  0x00,                                       /* bAlternateSetting:Index of this alternate setting */
		  0x02,                                       /* bNumEndpoints: 2 endpoints */
		  0x01,							      		  /* bInterfaceClass: AUDIO */
		  0x03,										  /* bInterfaceSubClass: MIDISTREAMING */
		  0x00,										  /* bInterfaceProtocol: Unused */
		  0x00,										  /* iInterface: Unused */
		  /* 09 byte */


		  /*================================================================================================*/
		  /* B.4.2 Class-specific MS Interface Descriptor */
		  0x07,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE */
		  0x01,                                       /* bDescriptorSubtype: MS_HEADER subtype */
		  0x00,                                       /* bcdADC: Revision of this class specification - 1.0 */
		  0x01,
		  0x37,										  /* wTotalLength: */ // !41 or 37! ??????????????????????????????????????????
		  0x00,										  /* -Total size of class-specific descriptor (37) */
		  /* 07 byte */

		  /* B.4.3 MIDI Adapter MIDI IN Jack Descriptor (Embedded) */
		  0x06,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x02,                                       /* bDescriptorSubtype: MIDI_IN_JACK subtype */
		  0x01,                                       /* bJackType: EMBEDDED */
		  0x01,                                       /* bJackID: ID of this Jack */
		  0x00,										  /* iJack: Unused */
		  /* 06 byte */

		  /* B.4.3 MIDI Adapter MIDI IN Jack Descriptor (External) */
		  0x06,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x02,                                       /* bDescriptorSubtype: MIDI_IN_JACK subtype */
		  0x02,                                       /* bJackType: EXTERNAL */
		  0x02,                                       /* bJackID: ID of this Jack */
		  0x00,										  /* iJack: Unused */
		  /* 06 byte */

		  /* B.4.4 MIDI Adapter MIDI OUT Jack Descriptor (Embedded) */
		  0x09,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x03,                                       /* bDescriptorSubtype: MIDI_OUT_JACK subtype */
		  0x01,                                       /* bJackType: EMBEDDED */
		  0x03,                                       /* bJackID: ID of this Jack */
		  0x01,										  /* bNrInputPins: Number of Input Pins of this Jack */
		  0x02,										  /* BaSourceID(1): ID of the Entity to which this Pin is connected*/
		  0x01,										  /* BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected */
		  0x00,										  /* iJack: Unused */
		  /* 09 byte */

		  /* B.4.4 MIDI Adapter MIDI OUT Jack Descriptor (External) */
		  0x09,                                       /* bLength */
		  0x24,                                       /* bDescriptorType: CS_INTERFACE descriptor */
		  0x03,                                       /* bDescriptorSubtype: MIDI_OUT_JACK subtype */
		  0x02,                                       /* bJackType: EXTERNAL */
		  0x04,                                       /* bJackID: ID of this Jack */
		  0x01,										  /* bNrInputPins: Number of Input Pins of this Jack */
		  0x01,										  /* BaSourceID(1): ID of the Entity to which this Pin is connected*/
		  0x01,										  /* BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected */
		  0x00,										  /* iJack: Unused */
		  /* 09 byte */

		  /* 37 byte */ /* "wTotalLength" in B.4.2 Class-specific MS Interface Descriptor */
		  /*================================================================================================*/

		  /* B.5.1 Standard Bulk OUT Endpoint Descriptor */
		  0x09,                                       /* bLength */
		  0x05,                                       /* bDescriptorType: ENDPOINT descriptor */
		  0x01,                              	      /* bEndpointAddress: OUT Endpoint 1 */
		  0x02,                                       /* bmAttributes: Bulk, not shared */
		  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize */ // 64
		  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
		  0x00,										  /* bInterval: Ignored for Bulk. Set to zero */
		  0x00,										  /* bRefresh: Unused */
		  0x00,										  /* bSynchAddress: Unused */
		  /* 09 byte */

		  /* B.5.2 Class-specific MS Bulk OUT Endpoint Descriptor */
		  0x05,                                       /* bLength */
		  0x25,                                       /* bDescriptorType: CS_ENDPOINT descriptor */
		  0x01,                                       /* bDescriptorSubtype: MS_GENERAL subtype */
		  0x01,										  /* bNumEmbMIDIJack: Number of embedded MIDI IN Jacks */
		  0x01,										  /* BaAssocJackID(1): ID of the Embedded MIDI IN Jack */
		  /* 05 byte */

		  /* B.6.1 Standard Bulk IN Endpoint Descriptor*/
		  0x09,                                       /* bLength */
		  0x05,                                       /* bDescriptorType: ENDPOINT descriptor */
		  0x81,                              	      /* bEndpointAddress: IN Endpoint 1 */
		  0x02,                                       /* bmAttributes: Bulk, not shared */
		  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize */ // 64
		  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
		  0x00,										  /* bInterval: Ignored for Bulk. Set to zero */
		  0x00,										  /* bRefresh: Unused */
		  0x00,										  /* bSynchAddress: Unused */
		  /* 09 byte */

		  /* B.6.2 Class-specific MS Bulk IN Endpoint Descriptor */
		  0x05,                                       /* bLength */
		  0x25,                                       /* bDescriptorType: CS_ENDPOINT descriptor */
		  0x01,                                       /* bDescriptorSubtype: MS_GENERAL subtype */
		  0x01,										  /* bNumEmbMIDIJack: Number of embedded MIDI OUT Jacks */
		  0x03,										  /* BaAssocJackID(1): ID of the Embedded MIDI OUT Jack */
		  /* 05 byte */

};

/**
  * @}
  */

/** @defgroup USBD_CDC_Private_Functions
  * @{
  */

/**
  * @brief  USBD_CDC_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  midi_usb_connect();
  uint8_t ret = 0U;
  USBD_CDC_HandleTypeDef   *hcdc;

  if (pdev->dev_speed == USBD_SPEED_HIGH)
  {
    /* Open EP IN */
    USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK,
                   CDC_DATA_HS_IN_PACKET_SIZE);

    pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U;

    /* Open EP OUT */
    USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK,
                   CDC_DATA_HS_OUT_PACKET_SIZE);

    pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U;

  }
  else
  {
    /* Open EP IN */
    USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK,
                   CDC_DATA_FS_IN_PACKET_SIZE);

    pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U;

    /* Open EP OUT */
    USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK,
                   CDC_DATA_FS_OUT_PACKET_SIZE);

    pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U;
  }
  /* Open Command IN EP */
  USBD_LL_OpenEP(pdev, CDC_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 1U;

  pdev->pClassData = USBD_malloc(sizeof(USBD_CDC_HandleTypeDef));

  if (pdev->pClassData == NULL)
  {
    ret = 1U;
  }
  else
  {
    hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

    /* Init  physical Interface components */
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Init();

    /* Init Xfer states */
    hcdc->TxState = 0U;
    hcdc->RxState = 0U;

    if (pdev->dev_speed == USBD_SPEED_HIGH)
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, hcdc->RxBuffer,
                             CDC_DATA_HS_OUT_PACKET_SIZE);
    }
    else
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, hcdc->RxBuffer,
                             CDC_DATA_FS_OUT_PACKET_SIZE);
    }
  }
  return ret;
}

/**
  * @brief  USBD_CDC_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  midi_usb_disconnect();
  uint8_t ret = 0U;

  /* Close EP IN */
  USBD_LL_CloseEP(pdev, CDC_IN_EP);
  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 0U;

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, CDC_OUT_EP);
  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 0U;

  /* Close Command IN EP */
  USBD_LL_CloseEP(pdev, CDC_CMD_EP);
  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 0U;

  /* DeInit  physical Interface components */
  if (pdev->pClassData != NULL)
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->DeInit();
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_CDC_Setup(USBD_HandleTypeDef *pdev,
                               USBD_SetupReqTypedef *req)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;
  uint8_t ifalt = 0U;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS :
      if (req->wLength)
      {
        if (req->bmRequest & 0x80U)
        {
          ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest,
                                                            (uint8_t *)(void *)hcdc->data,
                                                            req->wLength);

          USBD_CtlSendData(pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
        }
        else
        {
          hcdc->CmdOpCode = req->bRequest;
          hcdc->CmdLength = (uint8_t)req->wLength;

          USBD_CtlPrepareRx(pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
        }
      }
      else
      {
        ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest,
                                                          (uint8_t *)(void *)req, 0U);
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_STATUS:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, (uint8_t *)(void *)&status_info, 2U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, &ifalt, 1U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (pdev->dev_state != USBD_STATE_CONFIGURED)
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;

    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;
  PCD_HandleTypeDef *hpcd = pdev->pData;

  if (pdev->pClassData != NULL)
  {
    if ((pdev->ep_in[epnum].total_length > 0U) && ((pdev->ep_in[epnum].total_length % hpcd->IN_ep[epnum].maxpacket) == 0U))
    {
      /* Update the packet total length */
      pdev->ep_in[epnum].total_length = 0U;

      /* Send ZLP */
      USBD_LL_Transmit(pdev, epnum, NULL, 0U);
    }
    else
    {
      hcdc->TxState = 0U;
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  /* Get the received data length */
  hcdc->RxLength = USBD_LL_GetRxDataSize(pdev, epnum);

  /* USB data will be immediately processed, this allow next USB traffic being
  NAKed till the end of the application Xfer */
  if (pdev->pClassData != NULL)
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Receive(hcdc->RxBuffer, &hcdc->RxLength);

    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_CDC_EP0_RxReady
  *         Handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_CDC_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  if ((pdev->pUserData != NULL) && (hcdc->CmdOpCode != 0xFFU))
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(hcdc->CmdOpCode,
                                                      (uint8_t *)(void *)hcdc->data,
                                                      (uint16_t)hcdc->CmdLength);
    hcdc->CmdOpCode = 0xFFU;

  }
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_GetFSCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_CDC_GetFSCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_CDC_CfgFSDesc);
  return USBD_CDC_CfgFSDesc;
}

/**
  * @brief  USBD_CDC_GetHSCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_CDC_GetHSCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_CDC_CfgHSDesc);
  return USBD_CDC_CfgHSDesc;
}

/**
  * @brief  USBD_CDC_GetCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_CDC_GetOtherSpeedCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_CDC_OtherSpeedCfgDesc);
  return USBD_CDC_OtherSpeedCfgDesc;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor(uint16_t *length)
{
  *length = sizeof(USBD_CDC_DeviceQualifierDesc);
  return USBD_CDC_DeviceQualifierDesc;
}

/**
* @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t  USBD_CDC_RegisterInterface(USBD_HandleTypeDef   *pdev,
                                    USBD_CDC_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if (fops != NULL)
  {
    pdev->pUserData = fops;
    ret = USBD_OK;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_SetTxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Tx Buffer
  * @retval status
  */
uint8_t  USBD_CDC_SetTxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff,
                              uint16_t length)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  hcdc->TxBuffer = pbuff;
  hcdc->TxLength = length;

  return USBD_OK;
}


/**
  * @brief  USBD_CDC_SetRxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Rx Buffer
  * @retval status
  */
uint8_t  USBD_CDC_SetRxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  hcdc->RxBuffer = pbuff;

  return USBD_OK;
}

/**
  * @brief  USBD_CDC_TransmitPacket
  *         Transmit packet on IN endpoint
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  if (pdev->pClassData != NULL)
  {
    if (hcdc->TxState == 0U)
    {
      /* Tx Transfer in progress */
      hcdc->TxState = 1U;

      /* Update the packet total length */
      pdev->ep_in[CDC_IN_EP & 0xFU].total_length = hcdc->TxLength;

      /* Transmit next packet */
      USBD_LL_Transmit(pdev, CDC_IN_EP, hcdc->TxBuffer,
                       (uint16_t)hcdc->TxLength);

      return USBD_OK;
    }
    else
    {
      return USBD_BUSY;
    }
  }
  else
  {
    return USBD_FAIL;
  }
}


/**
  * @brief  USBD_CDC_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef *) pdev->pClassData;

  /* Suspend or Resume USB Out process */
  if (pdev->pClassData != NULL)
  {
    if (pdev->dev_speed == USBD_SPEED_HIGH)
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev,
                             CDC_OUT_EP,
                             hcdc->RxBuffer,
                             CDC_DATA_HS_OUT_PACKET_SIZE);
    }
    else
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev,
                             CDC_OUT_EP,
                             hcdc->RxBuffer,
                             CDC_DATA_FS_OUT_PACKET_SIZE);
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
