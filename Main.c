#include "stm32g0xx.h"
#include <string.h>

#include "System definitions.h"
#include "System.h"

#include "GPIO.h"
#include "H-bus.h"
#include "HB timer.h"
#include "HB UART.h"
#include "LED.h"
#include "UART.h"	

#include "Main.h"

#include "protocol_slip.h"

#define HB_UART_SPEED   115200
#define HB_ENUMERATION_NB_TIME_SLOTS 80
#define HB_ENUMERATION_SLOT_TIME     100

#define APP_EEPPROM_SIZE 0x200



typedef enum
{
	HB_S_RESET,
	HB_S_ENUMERATING,
	HB_S_CONFIGURED,
} eHB_State;

U32  ibus_lite_timeout;

static U8 APP_RXBuffer[APP_RX_BUFFER_SIZE];
static U8 APP_RXBuffer1[APP_RX_BUFFER_SIZE];
U8 APP_TXBuffer[APP_TX_BUFFER_SIZE];
static sHB_Packet APP_ResponsePacket;

static U16 APP_Channels[HB_NB_MAX_CHANNELS];

static eHB_State APP_HBState;
static U32 APP_HBSerialNb;
static U8 APP_HBDeviceID;

static const sUART_Config APP_UARTConfig={
	.BitDuration=UART_BAUDRATE_TO_BIT_DURATION(HB_UART_SPEED),
	.Parity=UART_PARITY_NONE,
	.StopBits=UART_STOPBIT_1,
	.Inverted=FALSE,
	.StrongDriveTXOnly=FALSE,
	.Reserved=0
};

static void HB_RXActivityCallback(void);
static void HB_StartReception(void);
static void HB_UART_ReceiveCallback(U32 NbReceivedBytes);
static void HB_UART_TransmitCallback(void);
static void HB_TIM_TimerCompareIRQHandler(void);
static U16 WS_CalcHBusCRC16(const void *pData,U32 DataLength);

U16 rx_test_cnt = 0;
U16 rx_test_len = 0;
U32  enum_delay;
sHB_PacketResponse  test;

static void HB_UART_ReceiveCallback(U32 NbReceivedBytes)
{
	const sHB_Packet *pCommandPacket;
#ifdef SLIP_EN	
    NbReceivedBytes = parse_data( APP_RXBuffer, NbReceivedBytes, APP_RXBuffer1 );
    
    if( !NbReceivedBytes ) return;
	pCommandPacket=(const sHB_Packet *)APP_RXBuffer1;
#else    
    pCommandPacket=(const sHB_Packet *)APP_RXBuffer;
#endif
	APP_ResponsePacket.Header.Length=0;
	if (NbReceivedBytes>=sizeof(sHB_PacketCommandMin) &&
		NbReceivedBytes<=sizeof(sHB_PacketCommandMax) &&
		pCommandPacket->Header.Length==NbReceivedBytes &&
		SYS_LoadUShort((const U8 *)pCommandPacket+NbReceivedBytes-sizeof(U16))==WS_CalcHBusCRC16(pCommandPacket,NbReceivedBytes-sizeof(U16)))
	{
		switch (pCommandPacket->Header.PacketType)
		{
			case HB_PT_COMMAND_FROM_HOST:
				switch (pCommandPacket->Command.CommandCode)
				{
					case HB_CC_RESET:
						
                        if( APP_HBState !=HB_S_RESET )
                        {
                            LED_SetPattern(&LED_RESET);
                        }
                        APP_HBState=HB_S_RESET;
						break;
					case HB_CC_ENUMERATE:
						if (APP_HBState==HB_S_CONFIGURED || APP_HBState==HB_S_ENUMERATING ||
							pCommandPacket->Header.DeviceID!=HB_DEVICE_ID_BROADCAST)
						{
							break;
						}
                        
                        enum_delay = 1000+(SYS_GetSimpleRandomNumber()%HB_ENUMERATION_NB_TIME_SLOTS)*HB_ENUMERATION_SLOT_TIME;
                        
						HB_TIM_SetNextCompareIRQTimeFromNow( enum_delay, HB_TIM_TimerCompareIRQHandler);
						APP_ResponsePacket.Header.Length=sizeof(sHB_PacketResponseEnumerate);
						APP_ResponsePacket.Header.PacketType=HB_PT_RESPONSE_FROM_DEVICE;
						APP_ResponsePacket.Header.DeviceID=HB_DEVICE_ID_BROADCAST;
						APP_ResponsePacket.ResponseEnumerate.CommandCode=HB_CC_ENUMERATE;
						APP_ResponsePacket.ResponseEnumerate.DeviceSerialNb=APP_HBSerialNb;
						GPIO_EnablePA1Interrupt(HB_RXActivityCallback);
						break;
					case HB_CC_ASSIGN_ID:
						if ( //APP_HBState!=HB_S_ENUMERATING ||
							pCommandPacket->CommandEnumerate.DeviceSerialNb!=APP_HBSerialNb)
						{
							break;
						}
						APP_HBDeviceID=pCommandPacket->CommandEnumerate.ID&0x3F;
                        
                        if( APP_HBState!=HB_S_CONFIGURED )
                        {
                            LED_SetPattern(&LED_CONFIGURED);
                        }
                        
						APP_HBState=HB_S_CONFIGURED;
                        ibus_lite_timeout = GET_TICK_1MS();
						APP_ResponsePacket.Header.Length=sizeof(sHB_PacketResponseAssignID);
						APP_ResponsePacket.Header.PacketType=HB_PT_RESPONSE_FROM_DEVICE;
						APP_ResponsePacket.Header.DeviceID=APP_HBDeviceID;
						APP_ResponsePacket.ResponseAssignID.CommandCode=HB_CC_ASSIGN_ID;
						APP_ResponsePacket.ResponseAssignID.DeviceSerialNb=APP_HBSerialNb;
						break;
					case HB_CC_TEST:
					{
						U32 DataLength;

						if (APP_HBState!=HB_S_CONFIGURED)
							break;
                         if( APP_HBDeviceID !=pCommandPacket->Header.DeviceID ) //不是发给自己的数据
                             break;

                        ibus_lite_timeout = GET_TICK_1MS();
                         
						DataLength=pCommandPacket->Header.Length-sizeof(sHB_PacketCommandMin);
						APP_ResponsePacket.Header.Length=sizeof(sHB_PacketResponseMin)+DataLength;
						APP_ResponsePacket.Header.PacketType=HB_PT_RESPONSE_FROM_DEVICE;
						APP_ResponsePacket.Header.DeviceID=APP_HBDeviceID;
						APP_ResponsePacket.Command.CommandCode=HB_CC_TEST;
                        
                        rx_test_cnt++;
                        rx_test_len +=DataLength;
                         
                        memcpy(APP_ResponsePacket.Response.Arguments,pCommandPacket->Command.Arguments,DataLength);
                        
						break;
					}
				}
				break;
			case HB_PT_RESPONSE_FROM_DEVICE:
				break;
			case HB_PT_CHANNELS_DATA:
			case HB_PT_CHANNELS_DATA_WITH_COMMAND:
			{
				U32 NbRemainingChannels;
				const U8 *pSourceChannelsData;
				U16 *pTargetChannel;
				
				pSourceChannelsData=(pCommandPacket->Header.PacketType==HB_PT_CHANNELS_DATA)?pCommandPacket->ChannelsData.ChannelsData:pCommandPacket->ChannelsDataWithCommand.ChannelsDataAndArguments;
				pTargetChannel=APP_Channels;
				NbRemainingChannels=pCommandPacket->Header.NbChannels;
				while (1)
				{
					*pTargetChannel=SYS_LoadUShort(pSourceChannelsData) & 0x0FFF;
					pSourceChannelsData++;
					pTargetChannel++;
					NbRemainingChannels--;
					if (NbRemainingChannels==0)
						break;
					*pTargetChannel=SYS_LoadUShort(pSourceChannelsData) >> 4;
					pSourceChannelsData+=2;
					pTargetChannel++;
					NbRemainingChannels--;
					if (NbRemainingChannels==0)
						break;
				}
				break;
			}
		}
	}
	if (APP_ResponsePacket.Header.Length!=0)
	{
		SYS_StoreUShort((U8 *)&APP_ResponsePacket+APP_ResponsePacket.Header.Length-sizeof(U16),WS_CalcHBusCRC16(&APP_ResponsePacket,APP_ResponsePacket.Header.Length-sizeof(U16)));
		HB_UART_TransmitSetup(&APP_ResponsePacket,APP_ResponsePacket.Header.Length,HB_UART_TransmitCallback);
		if (APP_ResponsePacket.Header.PacketType!=HB_PT_RESPONSE_FROM_DEVICE ||
			APP_ResponsePacket.Response.CommandCode!=HB_CC_ENUMERATE)
		{
			HB_UART_TransmitStart();
		}
	}
	else
		HB_StartReception();
	return;
}

static void HB_RXActivityCallback(void)
{
	GPIO_DisablePA1Interrupt();
	HB_TIM_FreezeCompareIRQ();
	UB_UART_TransmitStop();
	HB_StartReception();
	return;
}

static void HB_StartReception(void)
{
	HB_UART_ReceiveSetup(APP_RXBuffer,APP_RX_BUFFER_SIZE,FALSE,HB_UART_ReceiveCallback);
	HB_UART_ReceiveStart();
	return;
}

static void HB_UART_TransmitCallback(void)
{
	HB_StartReception();
	return;
}

static void HB_TIM_TimerCompareIRQHandler(void)
{
    if( APP_HBState!=HB_S_ENUMERATING )
    {
        LED_SetPattern(&LED_ENUMERATING);
    }
    
    APP_HBState=HB_S_ENUMERATING;
	GPIO_DisablePA1Interrupt();
	HB_UART_TransmitStart();
	HB_TIM_FreezeCompareIRQ();
	return;
}

static const U16 WS_CalcHBusCRC16Table[256]={
	0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
	0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
	0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
	0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
	0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
	0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
	0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
	0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
	0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
	0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
	0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
	0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
	0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
	0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
	0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
	0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
	0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
	0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
	0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
	0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
	0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
	0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
	0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
	0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
	0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
	0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
	0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
	0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
	0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
	0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
	0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
	0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

static U16 WS_CalcHBusCRC16(const void *pData,U32 DataLength)
{
	U16 CRC16;
	
	CRC16=0xFFFF;
	while (DataLength)
	{
		CRC16=WS_CalcHBusCRC16Table[(CRC16^*(const U8 *)pData)&0xFF]^(CRC16>>8);
		pData=(const U8 *)pData+1;
		DataLength--;
   }
   return CRC16;

}

U32 enumerating_start_time;
int main(void)
{
	SYS_Init();
	GPIO_Init();
	LED_Init();
	APP_HBState=HB_S_RESET;
	APP_HBSerialNb=SYS_GetSimpleRandomNumberFromSeed(*(const U32 *)UID_BASE ^ *(const U32 *)(UID_BASE+sizeof(U32)) ^ *(const U32 *)(UID_BASE+sizeof(U32)*2));
	HB_TIM_Init();
	HB_UART_Init(&APP_UARTConfig);
	HB_UART_ReceiveSetup(APP_RXBuffer,APP_RX_BUFFER_SIZE,FALSE,HB_UART_ReceiveCallback);
	HB_UART_ReceiveStart();
	while (1)
	{
        
        if (APP_HBState !=HB_S_ENUMERATING)
        {
            enumerating_start_time = GET_TICK_1MS();
        }
        else
        {
            if( GET_TICK_1MS() - enumerating_start_time > 5000 )
            {
                if( APP_HBState !=HB_S_RESET )
                {
                    LED_SetPattern(&LED_RESET);
                }
                APP_HBState = HB_S_RESET;
            }
        }
        
        if(  GET_TICK_1MS() - ibus_lite_timeout > 10000 )
        {
            ibus_lite_timeout = GET_TICK_1MS();
            if( APP_HBState !=HB_S_RESET )
            {
                LED_SetPattern(&LED_RESET);
            }
            APP_HBState=HB_S_RESET;
        }
	}
}
