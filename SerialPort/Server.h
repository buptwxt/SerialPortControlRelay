#ifndef SERVER_H
#define SERVER_H

#include "stdafx.h"
#include "SerialPort.h"
#include "Relay.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "Common.h"
#include "CommData.h"
#include ".\tiny\tinyxml.h"
#include ".\tiny\tinystr.h"

#include "json/json-forwards.h"
#include "json/json.h"

class TCPServer
{
private:  
	TCPServer();  
	TCPServer(const TCPServer &);  
	TCPServer& operator = (const TCPServer &);  

public:  
	static TCPServer *GetTCPServerInstance();  
	static TCPServer *pInstance;  

	//全局队列锁
	CCriticalSection m_ReceivedCmdCri;//发送数据锁
	CCriticalSection m_SetXMLCri;//set xml
	CCriticalSection m_GetXMLCri;//get xml

public:
	void run();
	~TCPServer();
	bool ReadXMLInfor();
	bool WriteXMLInfor(const Serial2Device & serial2device);
	std::vector<Serial2Device> getVecSerialnum2DeviceType();
	
	//接收数据
	//int RecvData(char *pszRecvData, int aiBuffLen);
	int RecvData(const SOCKET & acceptSocket, char *pszRecvData, int aiBuffLen);
	//处理接收数据
	bool DealData(char *pszRecvData,int iLen, int & iOutLen, const SOCKET & acceptSocket);
	//求校验和
	unsigned short Crc16(const char *pBuf, unsigned short nLen);
	//解析出port chanel chanelStatus
	void AnalyseCmdPortChanelStatus(char *pszRecvData, const int & iLen, int & port, int & chanel, int & chanelStatus);
	//初始化TCPServer
	void InitServer();
	//创建accept socket线程
	bool CreateAcceptThread();	
	//创建接收cmd命令串线程
	bool CreateCmdThread();
	//创建执行命令线程
	bool CreateExecuteReceivedRelayCmdhread();
	//启动服务端
	bool Start();
	//set xml
	bool CreateSetXMLthread();
    //get xml
	bool CreateGetXMLthread();
	
	HANDLE m_AcceptThread;               //accept 线程句柄
	DWORD  m_AcceptThreadID;             //accept 线程ID
	HANDLE m_CmdThread;                  //Cmd 线程句柄
	DWORD  m_CmdThreadID;                //Cmd 线程ID
	HANDLE m_WorkThread;                 //accept 线程句柄
	DWORD  m_WorkThreadID;               //accept 线程ID
	HANDLE m_XMLCfgThread;               //set xml
	DWORD  m_XMLCfgThreadID;             //set xml

	HANDLE m_GetXMLCfgThread;            //get xml
	DWORD m_GetXMLCfgThreadID;           //get xml

	SOCKET GetServerSocket()
	{
		return mServerSocket;
	}

private:
	SOCKET		  mServerSocket;	///< 服务器套接字句柄
	sockaddr_in	  mServerAddr;	///< 服务器地址

	//SOCKET		mAcceptSocket;	///< 接受的客户端套接字句柄
	//sockaddr_in	mAcceptAddr;	///< 接收的客户端地址
	std::vector<Serial2Device> vecSerialnum2DeviceType;	
};

//执行全局cmd队列中的命令
DWORD WINAPI ExecuteReceivedRelayCmd(LPVOID lpParameter);
//accept线程
DWORD WINAPI AcceptThread(LPVOID lpParameter);
//cmd线程
DWORD WINAPI CmdThread(LPVOID lpParameter);
//set xml
DWORD WINAPI ExecuteSetXMLCfg(LPVOID lpParameter);
//get xml
DWORD WINAPI ExecuteGetXMLCfg(LPVOID lpParameter);
//封装发送包结构
int BuildPack(BYTE headDataType, char **pSendBuf, const char * pPackData, int iPackDataLen);
//整数转字符串
char * my_itoa(int value, char *string, int radix);
//产生json字符串，用于传输xml配置信息
char * GenerateSend2ClientJsonStr(int flagValue, std::string idStr, int serialNumValue, int chanelValue, int deviceTypeValue, std::string nameStr);


#endif