#include "../include/analysis.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winsock2.h>

#pragma comment(lib, "wsock32.lib")

/**
 * @brief ���ֽ����ж���һ����˷���ʾ��16λ����
 *
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @return ��(pstring + *offset)��ʼ��16λ����
 * @note �����ƫ��������2
 */
static uint16_t Read2B(char *ptr, unsigned *offset) {
    uint16_t ret = ntohs(*(uint16_t *) (ptr + *offset));
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
static uint32_t Read4B(char *ptr, unsigned *offset) {
    uint32_t ret = ntohl(*(uint32_t *) (ptr + *offset));
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
static unsigned BufferToName(uint8_t *namePtr, char *ptr, unsigned *offset) {
    unsigned start_offset = *offset;
    while (true) {
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
        int cur_length = (int) *(ptr + *offset);
        ++*offset;
        memcpy(namePtr, ptr + *offset, cur_length);
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
static void BufferToHead(DNSHeader *head, char *ptr, unsigned *offset) {
    head->id = Read2B(ptr, offset);
    uint16_t flag = Read2B(ptr, offset);//flag�ֶ���2B
    head->qdcount = (flag >> 15) & 0x1;//�� bit ��ȡ�ֶ�
    head->opcode = (flag >> 11) & 0xF;
    head->aa = (flag >> 10) & 0x1;
    head->tc = (flag >> 9) & 0x1;
    head->rd = (flag >> 8) & 0x1;
    head->ra = (flag >> 7) & 0x1;
    head->z = (flag >> 4) & 0x7;
    head->rcode = flag & 0xF;
    head->qdcount = Read2B(ptr, offset);//ÿ�ε���һ��Read2B��offset����+2
    head->ancount = Read2B(ptr, offset);
    head->nscount = Read2B(ptr, offset);
    head->arcount = Read2B(ptr, offset);
}

/**
 * @brief ���ֽ����ж���һ��Question Section
 *
 * @param pque Question Section
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note �����ƫ�������ӵ�Question Section��һ��λ�ã�ΪQNAME�ֶη����˿ռ�
 */
static void BufferToQD(DNSQuestion *qd, char *ptr, unsigned *offset) {
    qd->qname = (uint8_t *) calloc(DNSR_MAX_RR_SIZE, sizeof(uint8_t));//����ռ�
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
static void BufferToRR(DNSResourceRecord *rr, char *ptr, unsigned *offset) {
    rr->name = (uint8_t *) calloc(DNSR_MAX_RR_SIZE, sizeof(uint8_t));
    BufferToName(rr->name, ptr, offset);
    rr->type = Read2B(ptr, offset);
    rr->class = Read2B(ptr, offset);
    rr->ttl = Read4B(ptr, offset);
    rr->rdlength = Read2B(ptr, offset);
    if (rr->type == DNSR_TYPE_CNAME || rr->type == DNSR_TYPE_NS) // CNAME��NS��RDATA��һ������
    {
        uint8_t *temp = (uint8_t *) calloc(DNSR_MAX_RR_SIZE, sizeof(uint8_t));

        rr->rdlength = BufferToName(temp, ptr, offset);
        rr->rdata = (uint8_t *) calloc(rr->rdlength, sizeof(uint8_t));
        memcpy(rr->rdata, temp, rr->rdlength);
        free(temp);
    } else if (rr->type == DNSR_TYPE_MX) // RFC1035 3.3.9. MX RDATA format  ����Ҿ���Ҳ�����ɾ,Ȼ��Ѷ�Ӧ��QTYPEɾ���ͺ�
    {
        int i = 0;
        //ɾ�˵����ɣ�����������˵
        /*
        uint8_t* temp = (uint8_t*)calloc(DNSR_MAX_RR_NUM, sizeof(uint8_t));
    
        unsigned temp_offset = *offset + 2;
        rr->rdlength = string_to_rrname(temp, pstring, &temp_offset);
        rr->rdata = (uint8_t*)calloc(prr->rdlength + 2, sizeof(uint8_t));
        if (!prr->rdata)
            log_fatal("�ڴ�������")
            memcpy(prr->rdata, pstring + *offset, 2);
        memcpy(prr->rdata + 2, temp, prr->rdlength);
        prr->rdlength += 2;
        *offset = temp_offset;
        free(temp);
        */
    } else if (rr->type == DNSR_TYPE_SOA) // RFC1035 3.3.13. SOA RDATA format  �����Ҳ���ÿ���ɾ,Ȼ��Ѷ�Ӧ��QTYPEɾ���ͺ�
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
    } else {
        rr->rdata = (uint8_t *) calloc(rr->rdlength, sizeof(uint8_t));
        memcpy(rr->rdata, ptr + *offset, rr->rdlength);
        *offset += rr->rdlength;
    }
}

void BufferToPacket(DNSMessage *packet, char *ptr) {
    unsigned offset = 0;
    packet->header = (DNSHeader *) calloc(1, sizeof(DNSHeader));
    BufferToHead(packet->header, ptr, &offset);
    DNSQuestion *qtail = NULL; // �����βָ��
    for (int i = 0; i < packet->header->qdcount; i++) {
        DNSQuestion *temp = (DNSQuestion *) calloc(1, sizeof(DNSQuestion));
        if (qtail == NULL) { // ����ĵ�һ���ڵ�
            packet->que = temp;
            qtail = temp;
        } else {
            qtail->next = temp;
            qtail = temp;
        }
        BufferToQD(qtail, ptr, &offset);
    }
    DNSResourceRecord *rtail = NULL; // Resource Record�����βָ��
    for (int i = 0; i < packet->header->ancount + packet->header->nscount + packet->header->arcount; i++) {
        DNSResourceRecord *temp = (DNSResourceRecord *) calloc(1, sizeof(DNSResourceRecord));
        if (!rtail) { // ����ĵ�һ���ڵ�
            packet->rr = temp;
            rtail = temp;
        } else {
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
static void Write2B(char *ptr, unsigned *offset, uint16_t num) {
    *(uint16_t *) (ptr + *offset) = htons(num);
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
static void Write4B(char *ptr, unsigned *offset, uint32_t num) {
    *(uint32_t *) (ptr + *offset) = htonl(num);
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
static void NameToBuffer(uint8_t *name, char *ptr, unsigned *offset) {
    while (true) {
        uint8_t *loc = strchr(name, '.');//������
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
static void HeadToBuffer(DNSHeader *head, char *ptr, unsigned *offset) {
    Write2B(ptr, offset, head->id);
    uint16_t flag = 0;
    flag |= (head->qr << 15);
    flag |= (head->opcode << 11);
    flag |= (head->aa << 10);
    flag |= (head->tc << 9);
    flag |= (head->rd << 8);
    flag |= (head->ra << 7);
    flag |= (head->z << 4);
    flag |= (head->rcode);
    Write2B(ptr, offset, flag);
    Write2B(ptr, offset, head->qdcount);
    Write2B(ptr, offset, head->ancount);
    Write2B(ptr, offset, head->nscount);
    Write2B(ptr, offset, head->arcount);
}

/**
 * @brief ���ֽ�����д��һ��Question Section
 *
 * @param pque Question Section
 * @param pstring �ֽ������
 * @param offset �ֽ���ƫ����
 * @note д���ƫ�������ӵ�Question Section��һ��λ��
 */
static void QDToBuffer(DNSQuestion *qd, char *ptr, unsigned *offset) {
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
static void RRToBuffer(DNSResourceRecord *rr, char *ptr, unsigned *offset) {
    NameToBuffer(rr->name, ptr, offset);
    Write2B(ptr, offset, rr->type);
    Write2B(ptr, offset, rr->class);
    Write2B(ptr, offset, rr->ttl);
    Write2B(ptr, offset, rr->rdlength);
    if (rr->type == DNSR_TYPE_CNAME || rr->type == DNSR_TYPE_NS)
        NameToBuffer(rr->rdata, ptr, offset);
    else if (rr->type == DNSR_TYPE_MX)//ͬ���ɾ
    {
        int i = 0;
        /*
        unsigned temp_offset = *offset + 2;
        NameToBuffer(rr->rdata + 2, ptr, &temp_offset);
        memcpy(ptr + *offset, rr->rdata, 2);
        *offset = temp_offset;
        */
    } else if (rr->type == DNSR_TYPE_SOA)//ͬ���ɾ
    {
        int i = 0;/*
        NameToBuffer(rr->rdata, ptr, offset);
        NameToBuffer(rr->rdata + strlen(rr->rdata) + 1, ptr, offset);
        memcpy(pstring + *offset, prr->rdata + prr->rdlength - 20, 20);
        *offset += 20;
        */
    } else {
        memcpy(ptr + *offset, rr->rdata, rr->rdlength);
        *offset += rr->rdlength;
    }
}

unsigned PacketToBuffer(DNSMessage *packet, char *ptr) {
    unsigned offset = 0;
    HeadToBuffer(packet->header, ptr, &offset);
    DNSQuestion *qtail = packet->que;
    for (int i = 0; i < packet->header->qdcount; i++) {
        QDToBuffer(qtail, ptr, &offset);
        qtail = qtail->next;
    }
    DNSResourceRecord *rtail = packet->rr;
    for (int i = 0; i < packet->header->ancount + packet->header->nscount + packet->header->arcount; i++) {
        RRToBuffer(rtail, ptr, &offset);
        rtail = rtail->next;
    }
    return offset;
}

void DestroyRR(DNSResourceRecord *rr) {
    while (rr != NULL) {
        DNSResourceRecord *temp = rr->next;
        free(rr->name);
        free(rr->rdata);
        free(rr);
        rr = temp;
    }
}

void DestroyQD(DNSQuestion *qd) {
    while (qd != NULL) {
        DNSQuestion *temp = qd->next;
        free(qd->qname);
        free(qd);
        qd = temp;
    }
}

void DestroyPacket(DNSMessage *packet) {
    //  log_debug("�ͷ�DNS���Ŀռ� ID: 0x%04x", pmsg->header->id)
    free(packet->header);
    DestroyQD(packet->que);
    DestroyRR(packet->rr);
    free(packet);
}

DNSResourceRecord *CopyRR(DNSResourceRecord *dnsRR) {
    if (dnsRR == NULL) return NULL;
    // ���������ͷ�ڵ�
    DNSResourceRecord *oldDNSRR = dnsRR;
    DNSResourceRecord *newDNSRR = (DNSResourceRecord *) calloc(1, sizeof(DNSResourceRecord));
    //  DNSResourceRecord* tempRR = newDNSRR;
    memcpy(newDNSRR, oldDNSRR, sizeof(DNSResourceRecord));
    newDNSRR->name = (uint8_t *) calloc(DNSR_MAX_RR_NUM, sizeof(uint8_t));
    memcpy(newDNSRR->name, oldDNSRR->name, DNSR_MAX_RR_NUM);
    newDNSRR->rdata = (uint8_t *) calloc(DNSR_MAX_RR_NUM, sizeof(uint8_t));
    memcpy(newDNSRR->rdata, oldDNSRR->rdata, DNSR_MAX_RR_NUM);
    // ���������ʣ��ڵ�
    while (oldDNSRR->next != NULL) {
        newDNSRR->next = (DNSResourceRecord *) calloc(1, sizeof(DNSResourceRecord));
        oldDNSRR = oldDNSRR->next;
        newDNSRR = newDNSRR->next;
        memcpy(newDNSRR, oldDNSRR, sizeof(DNSResourceRecord));
        newDNSRR->name = (uint8_t *) calloc(DNSR_MAX_RR_NUM, sizeof(uint8_t));
        memcpy(newDNSRR->name, oldDNSRR->name, DNSR_MAX_RR_NUM);
        newDNSRR->rdata = (uint8_t *) calloc(DNSR_MAX_RR_NUM, sizeof(uint8_t));
        memcpy(newDNSRR->rdata, oldDNSRR->rdata, DNSR_MAX_RR_NUM);
    }
    return newDNSRR;
}

DNSQuestion *CopyQD(DNSQuestion *dnsQD) {
    if (dnsQD == NULL) return NULL;
    // ���������ͷ�ڵ�
    DNSQuestion *oldDNSQD = dnsQD;
    DNSQuestion *newDNSQD = (DNSQuestion *) calloc(1, sizeof(DNSQuestion));
    //  DNSQuestion* tempQD = newDNSQD;
    memcpy(newDNSQD, oldDNSQD, sizeof(DNSQuestion));
    newDNSQD->qname = (uint8_t *) calloc(DNSR_MAX_RR_NUM, sizeof(uint8_t));
    memcpy(newDNSQD->qname, oldDNSQD->qname, DNSR_MAX_RR_NUM);
    // ���������ʣ��ڵ�
    while (oldDNSQD->next != NULL) {
        newDNSQD->next = (DNSQuestion *) calloc(1, sizeof(DNSQuestion));
        oldDNSQD = oldDNSQD->next;
        newDNSQD = newDNSQD->next;
        memcpy(newDNSQD, oldDNSQD, sizeof(DNSQuestion));
        newDNSQD->qname = (uint8_t *) calloc(DNSR_MAX_RR_NUM, sizeof(uint8_t));
        memcpy(newDNSQD->qname, oldDNSQD->qname, DNSR_MAX_RR_NUM);
    }
    return newDNSQD;
}

DNSMessage *CopyPacket(DNSMessage *packet) {
    if (packet == NULL) return NULL;
    DNSMessage *oldPacket = packet;
    DNSMessage *newPacket = (DNSMessage *) calloc(1, sizeof(DNSMessage));
    newPacket->header = (DNSHeader *) calloc(1, sizeof(DNSHeader));
    memcpy(newPacket->header, oldPacket->header, sizeof(DNSHeader));
    // ���������ͷ�ڵ�
    newPacket->que = CopyQD(oldPacket->que);
    newPacket->rr = CopyRR(oldPacket->rr);
    return newPacket;
}