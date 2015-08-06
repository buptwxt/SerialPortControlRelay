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
#include <sys/types.h> //设置socket，头文件
//#include "minidump.h"
#include "CriticalSection.h"//临界区
#include "Event.h"
//#include "CommData.h"
//#include "DebugLog.h"
#include <queue>
#include "DebugLog.h"



#pragma pack(1)//一字节对齐

//功能码宏定义 
#define SX_HEAD_FUNID    0x33aa     //包头起始码
#define SX_RELAY_CMD     0x01       //打开或者关闭继电器数据
#define SX_RELAY_JSON    0x02       //传输json格式字符串，json串中key为port chanel chanelStatus
#define SX_RELAY_SET_XML 0x03       //set XML配置
#define SX_RELAY_GET_XML 0x04       //get XML配置
#define SX_CRC16_INIT    0x00000000 //crc16初始化

#define SX_EXIST_XML     1          //存在xml信息  set成功 或者get成功的标识
#define SX_NOT_EXIST_XML 0          //不存在xml信息 set失败 或者set失败的标识


//结构体定义

//包头
typedef struct StruHeadPack
{ 
	WORD  wdPackHead;     //起始码
	BYTE  byHeadDataType; //报头数据类型   
	DWORD dwdCheckCrc16;  //CRC16  校验和
	DWORD wdTotalLen;     //包长度:包头+包体的长度
	BYTE  byBobyDataType; //包体数据类型
}STRU_HEAD_PACK_S;

//去除字符串两头空格，合并中间的多余空格
void EraseMultiSpace(std::string &str);
//获取字符串中包含的数字
int getDigit(std::string &dataStr1);
//解析出port chanel chanelStatus
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