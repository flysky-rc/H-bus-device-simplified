#include <stdlib.h>
#include <string.h>

#include "protocol_slip.h"

/**
 * @brief ������ݣ�ת�������ֽ�
 * @param src ԭʼ����ָ��
 * @param src_len ԭʼ���ݳ���
 * @param dst ������������ָ�루��ҪԤ�ȷ����㹻�ռ䣩
 * @return ���������ݳ���
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
 * @brief ����������������ͷ��ת�����ݣ�
 * @param src ԭʼ����ָ��
 * @param src_len ԭʼ���ݳ���
 * @param dst ������������ָ�루��ҪԤ�ȷ����㹻�ռ䣩
 * @return �������ܳ���
 */
U32 pack_data(const U8 *src, U32 src_len, U8 *dst) {
    // ���Э��ͷ
    dst[0] = 0xC0;
    U32 total_len = 1;
    
    // ת������
    U32 escaped_len = escape_data(src, src_len, dst + 1);
    total_len += escaped_len;
    
    return total_len;
}


/**
 * @brief �������ݣ���ԭת���ֽ�
 * @param src ���յ�������ָ�루����Э��ͷ��
 * @param src_len ���յ������ݳ���
 * @param dst �������������ָ�루��ҪԤ�ȷ����㹻�ռ䣩
 * @return ����������ݳ��ȣ�-1��ʾ��������
 */
U32 unescape_data(const U8 *src, U32 src_len, U8 *dst) {
    U32 dst_len = 0;
    
    for (U32 i = 0; i < src_len; ) {
        if (src[i] == 0xDB) {
            // ����Ƿ����㹻���ֽڿ��Զ�ȡ
            if (i + 1 >= src_len) {
                return 0; // ���ݲ�����
            }
            
            switch (src[i+1]) {
                case 0xDC:
                    dst[dst_len++] = 0xC0;
                    break;
                case 0xDD:
                    dst[dst_len++] = 0xDB;
                    break;
                default:
                    return 0; // ��Ч��ת������
            }
            i += 2;
        } else {
            // ��ͨ�ֽ�ֱ�Ӹ���
            dst[dst_len++] = src[i++];
        }
    }
    
    return dst_len;
}

/**
 * @brief �����������������ͷ���������ݣ�
 * @param src ���յ�����������ָ��
 * @param src_len ���յ������ݳ���
 * @param dst �������������ָ�루��ҪԤ�ȷ����㹻�ռ䣩
 * @return ����������ݳ��ȣ�-1��ʾ��������
 */
U32 parse_data(const U8 *src, U32 src_len, U8 *dst) {
    // �����С���Ⱥ�Э��ͷ
    if (src_len < 1 || src[0] != 0xC0 || src[src_len-1] != 0xC0) {
        return 0;
    }
    
    // ����ת�����ݣ�����ͷ����β����
    return unescape_data(src + 1, src_len - 2, dst);
}