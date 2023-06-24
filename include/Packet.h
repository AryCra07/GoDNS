#ifndef Packet_h
#define Packet_h


#define DEFAULT_TTL 120
#define GET_TTL 1
#define DECREASE_TTL 2

#define STRING_MAX_SIZE 8192
#define RR_NAME_MAX_SIZE 512

#define QR_QUERY 0
#define QR_ANSWER 1

#define OPCODE_QUERY 0
#define OPCODE_IQUERY 1
#define OPCODE_STATUS 2

#define QTYPE_A 1
#define QTYPE_NS 2
#define QTYPE_CNAME 5
#define QTYPE_SOA 6
#define QTYPE_PTR 12
#define QTYPE_HINFO 13
#define QTYPE_MINFO 15
#define QTYPE_MX 15
#define QTYPE_TXT 16
#define QTYPE_AAAA 28

#define QCLASS_IN 1

#define RCODE_OK 0
#define RCODE_NXDOMAIN 3

#include<stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>
/*
 *                                 1  1  1  1  1  1
 *   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                      ID                       |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |QR|   Opcode  |AA|TC|RD|RA| Z|AD|CD|   RCODE   |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                    QDCOUNT                    |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                    ANCOUNT                    |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                    NSCOUNT                    |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                    ARCOUNT                    |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
typedef struct DNSPacketHeader { //ע���ڴ����
    uint16_t ID ;      //���ı�ʶ��ID
    uint8_t RD : 1;       //=1DNS�ݹ��ѯ,=0������ѯ
    uint8_t TC : 1;       //����512B��=1�ض�
    uint8_t AA : 1;       //=1Ȩ����������=0��Ȩ��������
    uint8_t QR : 1;       //=0��ѯ����=1��Ӧ
    uint8_t OPCODE : 4;   //=0��׼��ѯ��=1����ѯ��=2������״̬
    uint8_t RCODE : 4;    //=0�޲��=1��ʽ����=2���ӷ�����ʧ��
                          //=3���ִ���=4��ѯ���Ͳ�֧�֣�=5�ܾ���Ӧ
    uint8_t Z : 3;        //��=0������
    uint8_t RA : 1;       //=1֧�ֵݹ��ѯ
    uint16_t QDCOUNT ; //��ѯ�������
    uint16_t ANCOUNT ; //�ش����
    uint16_t NSCOUNT ; //Ȩ�����Ʒ������
    uint16_t ARCOUNT ; //���������Ȩ����������ӦIP��ַ����Ŀ
 }DNSPacketHeader;
/*
ID: 16 bits
RD: 1 bit
TC: 1 bit
AA: 1 bit
QR: 1 bit
OPCODE: 4 bits
RCODE: 4 bits
Z: 3 bits
RA: 1 bit
QDCOUNT: 16 bits
ANCOUNT: 16 bits
NSCOUNT: 16 bits
ARCOUNT: 16 bits
*/

typedef struct DNSPacketQD{
    uint16_t qtype;//DNS�������Դ����    2B
    uint16_t qclass;//��������ѯ��Ϊ1    2B
    uint8_t* qname;
    DNSPacketQD* next;//����ʵ��
}DNSPacketQD;

typedef struct DNSPacketRR{//DNS��Դ��¼
    uint8_t* rname;       //DNS���������
    uint16_t rtype;        //��Դ��¼������
    uint16_t rclass;     //��ַ���ͣ���������һ�£�
    uint32_t ttl;         //��Чʱ�䣬������
    uint16_t rdataLength; //rdata�ĳ���
    uint8_t* rdata;          //��Դ���ݣ�������answers��authorities����additional����Ϣ��
    DNSPacketRR* next;//��3
}DNSPacketRR;

typedef struct DNSPacket {
    DNSPacketHeader* header;
    DNSPacketQD* question;
    DNSPacketRR* resourceRecord;
}DNSPacket;
/*
extern void DecodeHeader(DNSPacketHeader* header);//��������ֽ�תС�˱����ֽ�
extern void EncodeHeader(DNSPacketHeader* header);//С�˱����ֽ�ת��������ֽ�
extern void ResolveQname(unsigned char*buffer, DNSPacketQD* DNSpacketQD);
extern unsigned int form_standard_response(unsigned char* buffer, char* domain_name, unsigned int ip, unsigned int* question_size);
extern void ResolveResourse(unsigned char* buffer, DNSPacketRR* DNSpacketRR)
*/
#endif // !Packet_h
