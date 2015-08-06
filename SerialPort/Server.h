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

	//ȫ�ֶ�����
	CCriticalSection m_ReceivedCmdCri;//����������
	CCriticalSection m_SetXMLCri;//set xml
	CCriticalSection m_GetXMLCri;//get xml

public:
	void run();
	~TCPServer();
	bool ReadXMLInfor();
	bool WriteXMLInfor(const Serial2Device & serial2device);
	std::vector<Serial2Device> getVecSerialnum2DeviceType();
	
	//��������
	//int RecvData(char *pszRecvData, int aiBuffLen);
	int RecvData(const SOCKET & acceptSocket, char *pszRecvData, int aiBuffLen);
	//�����������
	bool DealData(char *pszRecvData,int iLen, int & iOutLen, const SOCKET & acceptSocket);
	//��У���
	unsigned short Crc16(const char *pBuf, unsigned short nLen);
	//������port chanel chanelStatus
	void AnalyseCmdPortChanelStatus(char *pszRecvData, const int & iLen, int & port, int & chanel, int & chanelStatus);
	//��ʼ��TCPServer
	void InitServer();
	//����accept socket�߳�
	bool CreateAcceptThread();	
	//��������cmd����߳�
	bool CreateCmdThread();
	//����ִ�������߳�
	bool CreateExecuteReceivedRelayCmdhread();
	//���������
	bool Start();
	//set xml
	bool CreateSetXMLthread();
    //get xml
	bool CreateGetXMLthread();
	
	HANDLE m_AcceptThread;               //accept �߳̾��
	DWORD  m_AcceptThreadID;             //accept �߳�ID
	HANDLE m_CmdThread;                  //Cmd �߳̾��
	DWORD  m_CmdThreadID;                //Cmd �߳�ID
	HANDLE m_WorkThread;                 //accept �߳̾��
	DWORD  m_WorkThreadID;               //accept �߳�ID
	HANDLE m_XMLCfgThread;               //set xml
	DWORD  m_XMLCfgThreadID;             //set xml

	HANDLE m_GetXMLCfgThread;            //get xml
	DWORD m_GetXMLCfgThreadID;           //get xml

	SOCKET GetServerSocket()
	{
		return mServerSocket;
	}

private:
	SOCKET		  mServerSocket;	///< �������׽��־��
	sockaddr_in	  mServerAddr;	///< ��������ַ

	//SOCKET		mAcceptSocket;	///< ���ܵĿͻ����׽��־��
	//sockaddr_in	mAcceptAddr;	///< ���յĿͻ��˵�ַ
	std::vector<Serial2Device> vecSerialnum2DeviceType;	
};

//ִ��ȫ��cmd�����е�����
DWORD WINAPI ExecuteReceivedRelayCmd(LPVOID lpParameter);
//accept�߳�
DWORD WINAPI AcceptThread(LPVOID lpParameter);
//cmd�߳�
DWORD WINAPI CmdThread(LPVOID lpParameter);
//set xml
DWORD WINAPI ExecuteSetXMLCfg(LPVOID lpParameter);
//get xml
DWORD WINAPI ExecuteGetXMLCfg(LPVOID lpParameter);
//��װ���Ͱ��ṹ
int BuildPack(BYTE headDataType, char **pSendBuf, const char * pPackData, int iPackDataLen);
//����ת�ַ���
char * my_itoa(int value, char *string, int radix);
//����json�ַ��������ڴ���xml������Ϣ
char * GenerateSend2ClientJsonStr(int flagValue, std::string idStr, int serialNumValue, int chanelValue, int deviceTypeValue, std::string nameStr);


#endif