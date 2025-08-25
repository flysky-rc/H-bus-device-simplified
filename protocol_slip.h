#ifndef __PROTOCOL_SLIP_H__
#define __PROTOCOL_SLIP_H__


#include "System definitions.h"

U32 pack_data(const U8 *src, U32 src_len, U8 *dst);
U32 parse_data(const U8 *src, U32 src_len, U8 *dst);


#endif