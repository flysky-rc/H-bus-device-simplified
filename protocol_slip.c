#include <stdlib.h>
#include <string.h>

#include "protocol_slip.h"

/**
 * @brief 打包数据，转义特殊字节
 * @param src 原始数据指针
 * @param src_len 原始数据长度
 * @param dst 打包后数据输出指针（需要预先分配足够空间）
 * @return 打包后的数据长度
 */
U32 escape_data(const U8 *src, U32 src_len, U8 *dst) {
    U32 dst_len = 0;
    
    for (U32 i = 0; i < src_len; i++) {
        switch (src[i]) {
            case 0xC0:
                dst[dst_len++] = 0xDB;
                dst[dst_len++] = 0xDC;
                break;
            case 0xDB:
                dst[dst_len++] = 0xDB;
                dst[dst_len++] = 0xDD;
                break;
            default:
                dst[dst_len++] = src[i];
                break;
        }
    }
    dst[dst_len++] = 0xC0;
    return dst_len;
}

/**
 * @brief 完整打包函数（添加头并转义数据）
 * @param src 原始数据指针
 * @param src_len 原始数据长度
 * @param dst 打包后数据输出指针（需要预先分配足够空间）
 * @return 打包后的总长度
 */
U32 pack_data(const U8 *src, U32 src_len, U8 *dst) {
    // 添加协议头
    dst[0] = 0xC0;
    U32 total_len = 1;
    
    // 转义数据
    U32 escaped_len = escape_data(src, src_len, dst + 1);
    total_len += escaped_len;
    
    return total_len;
}


/**
 * @brief 解析数据，还原转义字节
 * @param src 接收到的数据指针（不含协议头）
 * @param src_len 接收到的数据长度
 * @param dst 解析后数据输出指针（需要预先分配足够空间）
 * @return 解析后的数据长度，-1表示解析错误
 */
U32 unescape_data(const U8 *src, U32 src_len, U8 *dst) {
    U32 dst_len = 0;
    
    for (U32 i = 0; i < src_len; ) {
        if (src[i] == 0xDB) {
            // 检查是否有足够的字节可以读取
            if (i + 1 >= src_len) {
                return 0; // 数据不完整
            }
            
            switch (src[i+1]) {
                case 0xDC:
                    dst[dst_len++] = 0xC0;
                    break;
                case 0xDD:
                    dst[dst_len++] = 0xDB;
                    break;
                default:
                    return 0; // 无效的转义序列
            }
            i += 2;
        } else {
            // 普通字节直接复制
            dst[dst_len++] = src[i++];
        }
    }
    
    return dst_len;
}

/**
 * @brief 完整解析函数（检查头并解析数据）
 * @param src 接收到的完整数据指针
 * @param src_len 接收到的数据长度
 * @param dst 解析后数据输出指针（需要预先分配足够空间）
 * @return 解析后的数据长度，-1表示解析错误
 */
U32 parse_data(const U8 *src, U32 src_len, U8 *dst) {
    // 检查最小长度和协议头
    if (src_len < 1 || src[0] != 0xC0 || src[src_len-1] != 0xC0) {
        return 0;
    }
    
    // 解析转义数据（跳过头部和尾部）
    return unescape_data(src + 1, src_len - 2, dst);
}