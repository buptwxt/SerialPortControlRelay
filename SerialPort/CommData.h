#ifndef COMM_DATA_H
#define COMM_DATA_H

#include <stdlib.h>
#include <string>
#include <queue>
#include <vector>
#include <stdio.h>
#include <Winsock2.h>   // Ws2_32.lib
#include <string>
#include <iostream>
#include <atltime.h>
#include <Windows.h>
#include <sstream>
#include <sys/types.h>  
#include <sys/timeb.h>
#include <sys/types.h> //����socket��ͷ�ļ�
//#include "minidump.h"
#include "CriticalSection.h"//�ٽ���
#include "Event.h"
//#include "CommData.h"
//#include "DebugLog.h"
#include <queue>
#include "DebugLog.h"



#pragma pack(1)//һ�ֽڶ���

//������궨�� 
#define SX_HEAD_FUNID    0x33aa     //��ͷ��ʼ��
#define SX_RELAY_CMD     0x01       //�򿪻��߹رռ̵�������
#define SX_RELAY_JSON    0x02       //����json��ʽ�ַ�����json����keyΪport chanel chanelStatus
#define SX_RELAY_SET_XML 0x03       //set XML����
#define SX_RELAY_GET_XML 0x04       //get XML����
#define SX_CRC16_INIT    0x00000000 //crc16��ʼ��

#define SX_EXIST_XML     1          //����xml��Ϣ  set�ɹ� ����get�ɹ��ı�ʶ
#define SX_NOT_EXIST_XML 0          //������xml��Ϣ setʧ�� ����setʧ�ܵı�ʶ


//�ṹ�嶨��

//��ͷ
typedef struct StruHeadPack
{ 
	WORD  wdPackHead;     //��ʼ��
	BYTE  byHeadDataType; //��ͷ��������   
	DWORD dwdCheckCrc16;  //CRC16  У���
	DWORD wdTotalLen;     //������:��ͷ+����ĳ���
	BYTE  byBobyDataType; //������������
}STRU_HEAD_PACK_S;

//ȥ���ַ�����ͷ�ո񣬺ϲ��м�Ķ���ո�
void EraseMultiSpace(std::string &str);
//��ȡ�ַ����а���������
int getDigit(std::string &dataStr1);
//������port chanel chanelStatus
bool AnalyseCmdBuf(std::string recvStr, int & port, int & chanel, int & chanelStatus);
int StringToBytes(const char* pSrc, unsigned char* pDst, int nSrcLength);




typedef struct struct_Relay_Cmd
{
	int port;
	int chanel;
	int chanelStatus;
}Relay_Cmd_S;

#pragma pack()

#endif