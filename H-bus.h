#if !defined(H_BUS_H)
#define H_BUS_H

#include "System definitions.h"

#define HB_MAX_COMMAND_ARGUMENTS_LENGTH       200
#define HB_MAX_RESPONSE_ARGUMENTS_LENGTH      200
#define HB_NB_MAX_CHANNELS                     32
#define HB_MAX_CHANNELS_DATA_LENGTH           ((HB_NB_MAX_CHANNELS*12+7)/8)
#define HB_MAX_CHANNELS_DATA_ARGUMENTS_LENGTH 150
#define HB_CHANNELS_DATA_TIME_GAP_US          500 // Time gap after HobbyWing channels data in microseconds
#define HB_DEVICE_ID_BROADCAST                0x3F

#define HB_CC_TEST      0x0483
#define HB_CC_RESET     0x0480
#define HB_CC_ENUMERATE 0x0481
#define HB_CC_ASSIGN_ID 0x0482

typedef enum
{
	HB_PT_COMMAND_FROM_HOST,
	HB_PT_RESPONSE_FROM_DEVICE,
	HB_PT_CHANNELS_DATA,
	HB_PT_CHANNELS_DATA_WITH_COMMAND
} eHB_PacketType;

typedef struct ATTRIBUTE_PACKED
{
	U8 Length;
	union
	{
		struct
		{
			U8 DeviceID:6;
			U8 PacketType:2;
		};
		U8 NbChannels:6;
	};
} sHB_PacketHeader;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U8 Arguments[];
} sHB_PacketCommand;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U16 CRC16;
} sHB_PacketCommandMin;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U32 DeviceSerialNb;
    U8 ID;
	U16 CRC16;
} sHB_PacketCommandAssignID;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U8 Arguments[HB_MAX_COMMAND_ARGUMENTS_LENGTH];
	U16 CRC16;
} sHB_PacketCommandMax;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U8 Arguments[];
} sHB_PacketResponse;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U16 CRC16;
} sHB_PacketResponseMin;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U8 Arguments[HB_MAX_RESPONSE_ARGUMENTS_LENGTH];
	U16 CRC16;
} sHB_PacketResponseMax;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U32 DeviceSerialNb;
	U16 CRC16;
} sHB_PacketResponseEnumerate;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U16 CommandCode;
	U32 DeviceSerialNb;
    U8 ID;
	U16 CRC16;
} sHB_PacketResponseAssignID;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U8 ChannelsData[];
} sHB_PacketChannelsData;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U8 ChannelsData[HB_MAX_CHANNELS_DATA_LENGTH];
	U16 CRC16;
} sHB_PacketChannelsDataMax;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U8 CommandCode;
	U8 ChannelsDataAndArguments[];
} sHB_PacketChannelsDataWithCommand;

typedef struct ATTRIBUTE_PACKED
{
	sHB_PacketHeader Header;
	U8 CommandCode;
	U8 ChannelsDataAndArguments[HB_MAX_CHANNELS_DATA_LENGTH+HB_MAX_CHANNELS_DATA_ARGUMENTS_LENGTH];
	U16 CRC16;
} sHB_PacketChannelsDataWithCommandMax;

typedef union
{
	sHB_PacketHeader Header;
	sHB_PacketCommand Command;
	sHB_PacketCommandMin CommandMin;
	sHB_PacketCommandAssignID CommandEnumerate;
	sHB_PacketCommandMax CommandMax;
	sHB_PacketResponse Response;
	sHB_PacketResponseMin ResponseMin;
	sHB_PacketResponseMax ResponseMax;
	sHB_PacketResponseEnumerate ResponseEnumerate;
	sHB_PacketResponseAssignID ResponseAssignID;
	sHB_PacketChannelsData ChannelsData;
	sHB_PacketChannelsDataMax ChannelsDataMax;
	sHB_PacketChannelsDataWithCommand ChannelsDataWithCommand;
	sHB_PacketChannelsDataWithCommandMax ChannelsDataWithCommandMax;
} sHB_Packet;

#endif // !defined(H_BUS_H)
