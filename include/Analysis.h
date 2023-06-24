#ifndef Analysis_h
#define Analysis_h
#include "Packet.h"
//���Ľ���ͷ�ļ�

/**
 * @brief DNS�����ֽ���ת�����ṹ��
 *
 * @param pmsg DNS���Ľṹ��
 * @param pstring DNS�����ֽ���
 * @note ΪHeader Section��Question Section��Resource Record�����˿ռ�
 */
extern void BufferToPacket(DNSPacket* dnsPacket,char* buffer);

/**
 * @brief DNS���Ľṹ��ת�����ֽ���
 *
 * @param pmsg DNS���Ľṹ��
 * @param pstring DNS�����ֽ���
 * @return �����ֽ����ĳ���
 */
extern unsigned PacketToBuffer(DNSPacket* dnsPacket, char* buffer);

/**
 * @brief �ͷ�DNS����RR�ṹ��Ŀռ�
 *
 * @param prr DNS����RR�ṹ��
 */
extern void DestroyRR(DNSPacketRR* dnsRR);
extern void DestroyQD(DNSPacketQD* dnsQD);
/**
 * @brief �ͷ�DNS���Ľṹ��Ŀռ�
 *
 * @param pmsg DNS���Ľṹ��
 */
void DestroyPacket(DNSPacket* dnsP);

/**
 * @brief ����DNS����RR�ṹ��
 *
 * @param src ԴRR�ṹ��
 * @return ���ƺ��RR�ṹ��
 * @note Ϊ�µ�RR�ṹ������˿ռ�
 */
extern DNSPacketRR * CopyRR(DNSPacketRR* dnsRR);
extern DNSPacketQD * CopyQD(DNSPacketQD* dnsQD);
/**
 * @brief ����DNS���Ľṹ��
 *
 * @param src Դ�ṹ��
 * @return ���ƺ�Ľṹ��
 * @note Ϊ�µĽṹ������˿ռ�
 */
extern DNSPacket* CopyPacket(DNSPacket* src);

#endif // !Analysis_h
