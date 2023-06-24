#include "Analysis.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
//#include <WinSock2.h>
#include <winsock2.h>
#pragma comment(lib,"wsock32.lib")
/**
 * @brief ���ֽ����ж���һ����˷���ʾ��16λ����
 *
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @return ��(pstring + *offset)��ʼ��16λ����
 * @note �����ƫ��������2
 */
static uint16_t Read2B(char* ptr, unsigned* offset)
{
    uint16_t ret = ntohs(*(uint16_t*)(ptr + *offset));
    *offset += 2;
    return ret;
}

/**
 * @brief ���ֽ����ж���һ����˷���ʾ��32λ����
 *
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @return ��(pstring + *offset)��ʼ��32λ����
 * @note �����ƫ��������4
 */
static uint32_t Read4B(char* ptr, unsigned* offset)
{
    uint32_t ret = ntohl(*(uint32_t*)(ptr + *offset));
    *offset += 4;
    return ret;
}

/**
 * @brief ���ֽ����ж���һ��NAME�ֶ�
 *
 * @param pname NAME�ֶ�
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @return NAME�ֶ��ܳ���
 * @note �����ƫ�������ӵ�NAME�ֶκ�һ��λ��
 */
static unsigned BufferToName(uint8_t* namePtr, char* ptr, unsigned* offset)
{
    unsigned start_offset = *offset;
    while (true)
    {
        if ((*(ptr + *offset) >> 6) & 0x3) // ѹ������
        {
            unsigned new_offset = Read2B(ptr, offset) & 0x3fff;//8bit��ʶ��-->16bitָ��
            return *offset - start_offset - 2 + BufferToName(namePtr, ptr, &new_offset);//�ָ����ݹ�ָ���һ����ʶ��
        }
        if (!*(ptr + *offset)) // ����0����ʾNAME�ֶεĽ���
        {
            ++*offset;//offset��ƫ��ֵ++
            *namePtr = 0;
            return *offset - start_offset;
        }
        int cur_length = (int)*(ptr + *offset);
        ++* offset;
        memcpy(namePtr, ptr  + *offset, cur_length);
        namePtr += cur_length;
        *offset += cur_length;
        *namePtr++ = '.';
    }
}

/**
 * @brief ���ֽ����ж���һ��Header Section
 *
 * @param phead Header Section
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note �����ƫ�������ӵ�Header Section��һ��λ��
 */
static void BufferToHead(DNSPacketHeader* head, char* ptr, unsigned* offset)
{
    head->ID = Read2B(ptr, offset);
    uint16_t flag = Read2B(ptr, offset);//flag�ֶ���2B
    head->QDCOUNT = (flag >> 15) & 0x1;//�� bit ��ȡ�ֶ�
    head->OPCODE = (flag >> 11) & 0xF;
    head->AA = (flag >> 10) & 0x1;
    head->TC = (flag >> 9) & 0x1;
    head->RD = (flag >> 8) & 0x1;
    head->RA = (flag >> 7) & 0x1;
    head->Z = (flag >> 4) & 0x7;
    head->RCODE = flag & 0xF;
    head->QDCOUNT = Read2B(ptr, offset);//ÿ�ε���һ��Read2B��offset����+2
    head->ANCOUNT = Read2B(ptr, offset);
    head->NSCOUNT = Read2B(ptr, offset);
    head->ARCOUNT = Read2B(ptr, offset);
}

/**
 * @brief ���ֽ����ж���һ��Question Section
 *
 * @param pque Question Section
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note �����ƫ�������ӵ�Question Section��һ��λ�ã�ΪQNAME�ֶη����˿ռ�
 */
static void BufferToQD(DNSPacketQD* qd, char* ptr, unsigned* offset)
{
   qd->qname = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));//����ռ�
    BufferToName(qd->qname, ptr, offset);
    qd->qtype = Read2B(ptr, offset);
    qd->qclass = Read2B(ptr, offset);
}

/**
 * @brief ���ֽ����ж���һ��Resource Record
 *
 * @param prr Resource Record
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note �����ƫ�������ӵ�Resource Record��һ��λ�ã�ΪNAME�ֶκ�RDATA�ֶη����˿ռ�
 */
static void BufferToRR(DNSPacketRR* rr, char* ptr, unsigned* offset)
{
    rr->rname = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
    BufferToName(rr->rname, ptr, offset);
    rr->rtype = Read2B(ptr, offset);
    rr->rclass = Read2B(ptr, offset);
    rr->ttl = Read4B(ptr, offset);
    rr->rdataLength = Read2B(ptr, offset);
    if (rr->rtype == QTYPE_CNAME || rr->rtype == QTYPE_NS) // CNAME��NS��RDATA��һ������
    {
        uint8_t* temp = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
     
        rr->rdataLength = BufferToName(temp, ptr, offset);
        rr->rdata = (uint8_t*)calloc(rr->rdataLength, sizeof(uint8_t));
        memcpy(rr->rdata, temp, rr->rdataLength);
        free(temp);
    }
    else if (rr->rtype == QTYPE_MX) // RFC1035 3.3.9. MX RDATA format  ����Ҿ���Ҳ�����ɾ,Ȼ��Ѷ�Ӧ��QTYPEɾ���ͺ�
    {
        int i = 0;
        //ɾ�˵����ɣ�����������˵
        /*
        uint8_t* temp = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
    
        unsigned temp_offset = *offset + 2;
        rr->rdataLength = string_to_rrname(temp, pstring, &temp_offset);
        rr->rdata = (uint8_t*)calloc(prr->rdlength + 2, sizeof(uint8_t));
        if (!prr->rdata)
            log_fatal("�ڴ�������")
            memcpy(prr->rdata, pstring + *offset, 2);
        memcpy(prr->rdata + 2, temp, prr->rdlength);
        prr->rdlength += 2;
        *offset = temp_offset;
        free(temp);
        */
    }
    else if (rr->rtype == QTYPE_SOA) // RFC1035 3.3.13. SOA RDATA format  �����Ҳ���ÿ���ɾ,Ȼ��Ѷ�Ӧ��QTYPEɾ���ͺ�
    {
        int i = 0;
        //ɾ�˵����ɣ�����������˵
        /*
        uint8_t* temp = (uint8_t*)calloc(DNS_RR_NAME_MAX_SIZE, sizeof(uint8_t));
        if (!temp)
            log_fatal("�ڴ�������")
            prr->rdlength = string_to_rrname(temp, pstring, offset);
        prr->rdlength += string_to_rrname(temp + prr->rdlength, pstring, offset);
        prr->rdata = (uint8_t*)calloc(prr->rdlength + 20, sizeof(uint8_t));
        if (!prr->rdata)
            log_fatal("�ڴ�������")
            memcpy(prr->rdata, temp, prr->rdlength);
        memcpy(prr->rdata + prr->rdlength, pstring + *offset, 20);
        *offset += 20;
        prr->rdlength += 20;
        free(temp);
        */
    }
    else
    {
        rr->rdata = (uint8_t*)calloc(rr->rdataLength, sizeof(uint8_t));
        memcpy(rr->rdata, ptr + *offset, rr->rdataLength);
        *offset += rr->rdataLength;
    }
}

void BufferToPacket(DNSPacket* packet, char* ptr)
{
    unsigned offset = 0;
    packet->header = (DNSPacketHeader*)calloc(1, sizeof(DNSPacketHeader));
    BufferToHead(packet->header, ptr, &offset);
    DNSPacketQD* qtail = NULL; // �����βָ��
    for (int i = 0; i < packet->header->QDCOUNT; i++)
    {
        DNSPacketQD* temp = (DNSPacketQD*)calloc(1, sizeof(DNSPacketQD));
        if (qtail == NULL) { // ����ĵ�һ���ڵ�
            packet->question = temp;
            qtail = temp;
        }
        else
        {
                qtail->next = temp;
                qtail = temp;
        }
        BufferToQD(qtail, ptr, &offset);
    }
    DNSPacketRR* rtail = NULL; // Resource Record�����βָ��
    for (int i = 0; i < packet->header->ANCOUNT + packet->header->NSCOUNT + packet->header->ARCOUNT; i++)
    {
        DNSPacketRR* temp = (DNSPacketRR*)calloc(1, sizeof(DNSPacketRR));
        if (!rtail) { // ����ĵ�һ���ڵ�
            packet->resourceRecord = temp;
            rtail = temp;
        }
        else
        {
                rtail->next = temp;
                rtail = temp;
        }
        BufferToRR(rtail, ptr, &offset);
    }
}

/**
 * @brief ���ֽ�����д��һ��С�˷���ʾ��16λ����
 *
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @param num ��д�������
 * @note д���ƫ��������2
 */
static void Write2B(char* ptr, unsigned* offset, uint16_t num)
{
    *(uint16_t*)(ptr + *offset) = htons(num);
    *offset += 2;
}

/**
 * @brief ���ֽ�����д��һ��С�˷���ʾ��32λ����
 *
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @param num ��д�������
 * @note д���ƫ��������4
 */
static void Write4B(char* ptr, unsigned* offset, uint32_t num)
{
    *(uint32_t*)(ptr + *offset) = htonl(num);
    *offset += 4;
}

/**
 * @brief ���ֽ�����д��һ��NAME�ֶ�
 *
 * @param pname NAME�ֶ�
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note д���ƫ�������ӵ�NAME�ֶκ�һ��λ��
 */
static void NameToBuffer(uint8_t* name, char* ptr, unsigned* offset)
{
    while (true)
    {
        uint8_t* loc = strchr(name, '.');//������
        if (loc == NULL)break;
        long cur_length = loc - name;
        ptr[(*offset)++] = cur_length;
        memcpy(ptr + *offset, name, cur_length);
        name += cur_length + 1;
        *offset += cur_length;
    }
    ptr[(*offset)++] = 0;
}

/**
 * @brief ���ֽ�����д��һ��Header Section
 *
 * @param phead Header Section
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note д���ƫ�������ӵ�Header Section��һ��λ��
 */
static void HeadToBuffer(DNSPacketHeader* head, char* ptr, unsigned* offset)
{
    Write2B(ptr, offset, head->ID);
    uint16_t flag = 0;
    flag |= (head->QR << 15);
    flag |= (head->OPCODE << 11);
    flag |= (head->AA << 10);
    flag |= (head->TC << 9);
    flag |= (head->RD << 8);
    flag |= (head->RA << 7);
    flag |= (head->Z << 4);
    flag |= (head->RCODE);
    Write2B(ptr, offset, flag);
    Write2B(ptr, offset, head->QDCOUNT);
    Write2B(ptr, offset, head->ANCOUNT);
    Write2B(ptr, offset, head->NSCOUNT);
    Write2B(ptr, offset, head->ARCOUNT);
}

/**
 * @brief ���ֽ�����д��һ��Question Section
 *
 * @param pque Question Section
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note д���ƫ�������ӵ�Question Section��һ��λ��
 */
static void QDToBuffer(DNSPacketQD* qd, char* ptr, unsigned* offset)
{
    NameToBuffer(qd->qname, ptr, offset);
    Write2B(ptr, offset, qd->qtype);
    Write2B(ptr, offset, qd->qclass);
}

/**
 * @brief ���ֽ�����д��һ��Resource Record
 *
 * @param prr Resource Record
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note д���ƫ�������ӵ�Resource Record��һ��λ��
 */
static void RRToBuffer(DNSPacketRR* rr, char* ptr, unsigned* offset)
{
    NameToBuffer(rr->rname, ptr, offset);
    Write2B(ptr, offset, rr->rtype);
    Write2B(ptr, offset, rr->rclass);
    Write2B(ptr, offset, rr->ttl);
    Write2B(ptr, offset, rr->rdataLength);
    if (rr->rtype == QTYPE_CNAME || rr->rtype == QTYPE_NS)
        NameToBuffer(rr->rdata, ptr, offset);
    else if (rr->rtype == QTYPE_MX)//ͬ���ɾ
    {
        int i = 0;
        /*
        unsigned temp_offset = *offset + 2;
        NameToBuffer(rr->rdata + 2, ptr, &temp_offset);
        memcpy(ptr + *offset, rr->rdata, 2);
        *offset = temp_offset;
        */
    }
    else if (rr->rtype == QTYPE_SOA)//ͬ���ɾ
    {
        int i = 0;/*
        NameToBuffer(rr->rdata, ptr, offset);
        NameToBuffer(rr->rdata + strlen(rr->rdata) + 1, ptr, offset);
        memcpy(pstring + *offset, prr->rdata + prr->rdlength - 20, 20);
        *offset += 20;
        */
    }
    else
    {
        memcpy(ptr + *offset, rr->rdata, rr->rdataLength);
        *offset += rr->rdataLength;
    }
}

unsigned PacketToBuffer(DNSPacket* packet, char* ptr)
{
    unsigned offset = 0;
    HeadToBuffer(packet->header, ptr, &offset);
    DNSPacketQD* qtail = packet->question;
    for (int i = 0; i < packet->header->QDCOUNT; i++)
    {
        QDToBuffer(qtail, ptr, &offset);
        qtail = qtail->next;
    }
    DNSPacketRR* rtail = packet->resourceRecord;
    for (int i = 0; i < packet->header->ANCOUNT + packet->header->NSCOUNT + packet->header->ARCOUNT; i++)
    {
        RRToBuffer(rtail, ptr, &offset);
        rtail = rtail->next;
    }
    return offset;
}

void DestroyRR(DNSPacketRR* rr)
{
    while (rr != NULL)
    {
        DNSPacketRR * temp = rr->next;
        free(rr->rname);
        free(rr->rdata);
        free(rr);
        rr = temp;
    }
}
void DestroyQD(DNSPacketQD* qd)
{
    while (qd != NULL)
    {
        DNSPacketQD* temp = qd->next;
        free(qd->qname);
        free(qd);
        qd = temp;
    }
}

void DestroyPacket(DNSPacket* packet)
{
  //  log_debug("�ͷ�DNS���Ŀռ� ID: 0x%04x", pmsg->header->id)
    free(packet->header);
    DestroyQD(packet->question);
    DestroyRR(packet->resourceRecord);
    free(packet);
}

DNSPacketRR* CopyRR(DNSPacketRR* dnsRR)
{
    if (dnsRR == NULL) return NULL;
    // ���������ͷ�ڵ�
    DNSPacketRR* oldDNSRR = dnsRR;
    DNSPacketRR* newDNSRR = (DNSPacketRR*)calloc(1, sizeof(DNSPacketRR));
  //  DNSPacketRR* tempRR = newDNSRR;
    memcpy(newDNSRR, oldDNSRR, sizeof(DNSPacketRR));
    newDNSRR->rname = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
    memcpy(newDNSRR->rname, oldDNSRR->rname, RR_NAME_MAX_SIZE);
    newDNSRR->rdata = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
    memcpy(newDNSRR->rdata, oldDNSRR->rdata, RR_NAME_MAX_SIZE);
    // ���������ʣ��ڵ�
    while (oldDNSRR->next!=NULL)
    {
        newDNSRR->next = (DNSPacketRR*)calloc(1, sizeof(DNSPacketRR));
        oldDNSRR = oldDNSRR->next;
        newDNSRR = newDNSRR->next;
        memcpy(newDNSRR, oldDNSRR, sizeof(DNSPacketRR));
        newDNSRR->rname = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
        memcpy(newDNSRR->rname, oldDNSRR->rname, RR_NAME_MAX_SIZE);
        newDNSRR->rdata = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
        memcpy(newDNSRR->rdata, oldDNSRR->rdata, RR_NAME_MAX_SIZE);
    }
    return newDNSRR;
}

DNSPacketQD* CopyQD(DNSPacketQD* dnsQD)
{
    if (dnsQD == NULL) return NULL;
    // ���������ͷ�ڵ�
    DNSPacketQD* oldDNSQD = dnsQD;
    DNSPacketQD* newDNSQD = (DNSPacketQD*)calloc(1, sizeof(DNSPacketQD));
  //  DNSPacketQD* tempQD = newDNSQD;
    memcpy(newDNSQD, oldDNSQD, sizeof(DNSPacketQD));
    newDNSQD->qname = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
    memcpy(newDNSQD->qname, oldDNSQD->qname, RR_NAME_MAX_SIZE);
    // ���������ʣ��ڵ�
    while (oldDNSQD->next != NULL)
    {
        newDNSQD->next = (DNSPacketQD*)calloc(1, sizeof(DNSPacketQD));
        oldDNSQD = oldDNSQD->next;
        newDNSQD = newDNSQD->next;
        memcpy(newDNSQD, oldDNSQD, sizeof(DNSPacketQD));
        newDNSQD->qname = (uint8_t*)calloc(RR_NAME_MAX_SIZE, sizeof(uint8_t));
        memcpy(newDNSQD->qname, oldDNSQD->qname, RR_NAME_MAX_SIZE);
    }
    return newDNSQD;
}

DNSPacket* CopyPacket(DNSPacket* packet)
{
    if (packet == NULL) return NULL;
    DNSPacket* oldPacket = packet;
    DNSPacket* newPacket = (DNSPacket*)calloc(1, sizeof(DNSPacket));
    newPacket->header = (DNSPacketHeader*)calloc(1, sizeof(DNSPacketHeader));
    memcpy(newPacket->header, oldPacket->header, sizeof(DNSPacketHeader));
    // ���������ͷ�ڵ�
    newPacket->question = CopyQD(oldPacket->question);
    newPacket->resourceRecord = CopyRR(oldPacket->resourceRecord);
    return newPacket;
}