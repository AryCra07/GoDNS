/**
 * @file     analysis.h
 * @brief    DNS���Ľ����빹��Ľӿڶ����ļ��������ֽ���ת�����ṹ�塢�ṹ��ת�����ֽ������ͷŽṹ��ռ䡢���ƽṹ��ȹ���
 */
#ifndef analysis_h
#define analysis_h
#include "dns_structure.h"

/**
 * @brief DNS�����ֽ���ת�����ṹ��
 *
 * @param pmsg DNS���Ľṹ��
 * @param pstring DNS�����ֽ���
 * @note ΪHeader Section��Question Section��Resource Record�����˿ռ�
 */
extern void BufferToPacket(DNSMessage * dnsMessage,char* buffer);

/**
 * @brief DNS���Ľṹ��ת�����ֽ���
 *
 * @param pmsg DNS���Ľṹ��
 * @param pstring DNS�����ֽ���
 * @return �����ֽ����ĳ���
 */
extern unsigned PacketToBuffer(DNSMessage * dnsMessage, char* buffer);

/**
 * @brief �ͷ�DNS����RR�ṹ��Ŀռ�
 *
 * @param prr DNS����RR�ṹ��
 */
extern void DestroyRR(DNSResourceRecord * dnsRR);
extern void DestroyQD(DNSQuestion * dnsQD);
/**
 * @brief �ͷ�DNS���Ľṹ��Ŀռ�
 *
 * @param pmsg DNS���Ľṹ��
 */
void DestroyPacket(DNSMessage * dnsP);

/**
 * @brief ����DNS����RR�ṹ��
 *
 * @param src ԴRR�ṹ��
 * @return ���ƺ��RR�ṹ��
 * @note Ϊ�µ�RR�ṹ������˿ռ�
 */
extern DNSResourceRecord * CopyRR(DNSResourceRecord * dnsRR);
extern DNSQuestion * CopyQD(DNSQuestion * dnsQD);
/**
 * @brief ����DNS���Ľṹ��
 *
 * @param src Դ�ṹ��
 * @return ���ƺ�Ľṹ��
 * @note Ϊ�µĽṹ������˿ռ�
 */
extern DNSMessage * CopyPacket(DNSMessage * src);

#endif // analysis_h
