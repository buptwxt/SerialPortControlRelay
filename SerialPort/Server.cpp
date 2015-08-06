#include "Server.h"

#pragma comment(lib,"tiny/tinyxml.lib")

std::vector<Serial2Device> gvecSerialNum2DeviceType;
TCPServer* TCPServer::pInstance = 0;  
//ȫ�ֶ��У���Ŵ���ҳ�˽��պʹ�
std::queue<Relay_Cmd_S> gqueue_ReceiveRelayCmd;
//set xml����
std::queue<std::pair <Serial2Device, SOCKET>> gqueue_SetXMLCfg;
//get xml����
std::queue<std::pair <Serial2Device, SOCKET>> gqueue_GetXMLCfg;

//�����ֵ
#define ABS(cond) (cond>0 ? cond: -cond)

//����ת�ַ���
char * my_itoa(int value, char *string, int radix)  
{  
	assert(string!=NULL);  

	char tmp[32]={'\0'};  
	int tmpval=ABS(value);  

	int i,j;  
	for(i=0; i<32; i++)  
	{                  
		tmp[i] = (tmpval%radix)+'0';  
		tmpval = tmpval/radix;

		if(tmpval==0)  
		{
			break;
		}
	}  

	if(value<0)tmp[++i]='-'; 

	for(j=0;i>=0;i--)  
		string[j++]=tmp[i];  

	string[j]='\0';  
	return string;
} 


//����json�ַ��������ڴ���xml������Ϣ
char * GenerateSend2ClientJsonStr(int flagValue, std::string idStr, int serialNumValue, int chanelValue, int deviceTypeValue, std::string nameStr)
{
	//flagValue: 1 
	//"{\"flag\": flagValue,\"id\": idValue,\"serialNum\": serialNumValue,\"chanel\": chanelValue,\"deviceType\": deviceTypeValue,\"name\": nameStr}"	
	std::string xmlStr;
	char temp[10] = {0};
	xmlStr += "{\"flag\": ";
	xmlStr += my_itoa(flagValue, temp, 10);
	xmlStr += ",\"id\": \"";
	xmlStr += idStr;
	xmlStr += "\",\"serialNum\":";
	xmlStr += my_itoa(serialNumValue, temp, 10);
	xmlStr += ",\"chanel\":";
	xmlStr += my_itoa(chanelValue, temp, 10);
	xmlStr += ",\"deviceType\":";
	xmlStr += my_itoa(deviceTypeValue, temp, 10);
	xmlStr += ",\"name\":\"";
	xmlStr += nameStr;
	xmlStr += "\"}";

	char *pXmlBuf = new char[xmlStr.size()+1];
	memset(pXmlBuf, '\0', xmlStr.size()+1);
	memcpy(pXmlBuf, xmlStr.c_str(), xmlStr.size());

	//std::cout << "xmlStr = " << xmlStr << std::endl;

	return pXmlBuf;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: Crc16
DESCRIPTION: У����
AUTHOR�������� 
PARAMETERS: pBuf��У������,nLen��У�����ݵĳ���,��������У����
RETURN: 
*-------------------------------------------------------------------------*/
unsigned short Crc16(const char *pBuf, unsigned short nLen)
{
	//stack wangxintang
	//return 0;

	BYTE i;
	unsigned short crc = 0;

	while (nLen--)
	{
		for (i = 0x80; i != 0; i >>= 1)
		{
			if ((crc&0x8000) != 0)
			{
				crc <<= 1;
				crc ^= 0x1021;
			}
			else
			{
				crc <<= 1;
			}
			if ((*pBuf&i) != 0)
			{
				crc ^= 0x1021;
			}
		}
		pBuf++;
	}

	return crc; 
}


/*--------------------------------------------------------------------------
FUNCTION NAME: BytesToString
DESCRIPTION: �ϲ����ڵĵ��ֽڵ�����ת��Ϊ��Ӧ��˫�ֽڵ�����
PARAMETERS:  
RETURN: ����ת����ĳ���
*-------------------------------------------------------------------------*/
int BytesToString(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	if(pSrc == NULL)
		return -1;
	int i; 
	const char tab[]="0123456789ABCDEF";	// 0x0-0xf���ַ����ұ�
	for (i = 0; i < nSrcLength; i++)
	{
		*pDst++ = tab[*pSrc >> 4];		// �����4λ
		*pDst++ = tab[*pSrc & 0x0f];	// �����4λ
		pSrc++;
	}
	// ����Ŀ���ַ�������
	return (nSrcLength * 2);
}

/*--------------------------------------------------------------------------
FUNCTION NAME: StringToBytes
DESCRIPTION: ˫�ֽڵ�����ת��Ϊ��Ӧ�ĵ��ֽڵ�����
PARAMETERS:  
RETURN: ����ת����ĳ���
*-------------------------------------------------------------------------*/
int StringToBytes(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	if(pSrc == NULL)
		return -1;
	if(std::string(pSrc) == "")
		return -1;

	int i;
	for (i = 0; i < nSrcLength; i += 2)
	{
		// �����4λ
		if ((*pSrc >= '0') && (*pSrc <= '9'))
		{
			*pDst = (*pSrc - '0') << 4;
		}
		else
		{
			*pDst = (*pSrc - 'A' + 10) << 4;
		}
		pSrc++;

		// �����4λ
		if ((*pSrc>='0') && (*pSrc<='9'))
		{
			*pDst |= *pSrc - '0';
		}
		else
		{
			*pDst |= *pSrc - 'A' + 10;
		}
		pSrc++;
		pDst++;
	}
	// ����Ŀ�����ݳ���
	return (nSrcLength / 2);
}



/*--------------------------------------------------------------------------
FUNCTION NAME: CreateGetXMLhread
DESCRIPTION: 
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::CreateGetXMLthread()
{
	m_GetXMLCfgThread = CreateThread(NULL,0,ExecuteGetXMLCfg,NULL,0,&m_GetXMLCfgThreadID);

	if(m_GetXMLCfgThread == NULL)
	{
		return false;
	}
	//CloseHandle(m_WorkThread);
	return true;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: BuildPack
DESCRIPTION: ��������ͷ����
PARAMETERS: headDataType ���json�� get/set XML
RETURN:  ���ذ�ͷת����ĳ���  "-1"ʧ��
*-------------------------------------------------------------------------*/
int BuildPack(BYTE headDataType, char **pSendBuf, const char * pPackData, int iPackDataLen)
{
	if (NULL == pSendBuf || NULL == pPackData)
	{
		return -1;
	}

	STRU_HEAD_PACK_S headPack;
	int iHeadLen = sizeof(STRU_HEAD_PACK_S);
	memset(&headPack, 0, iHeadLen);
	headPack.wdPackHead = SX_HEAD_FUNID;
	//У���������Ϊ��ʼֵ
	headPack.dwdCheckCrc16 = SX_CRC16_INIT;
	headPack.byHeadDataType = headDataType;
	headPack.wdTotalLen = iHeadLen + iPackDataLen;

	//���뻺���������������
	char* pHeadData=new char[iHeadLen];
	if (pHeadData == NULL)
	{
		return -1;
	}
	memset(pHeadData,0,iHeadLen);

	//��������������������У����������
	memcpy(pHeadData, &headPack, iHeadLen);
	//�ϲ�ͷ������ pPackData + pHeadData => strTemp, ���ڼ���crcֵ

	//����һ������ڴ�ռ䣬���ڴ��ͷ��������Ϣ
	char *pPack = new char[iHeadLen + iPackDataLen];
	memcpy(pPack, pHeadData, iHeadLen);
	memcpy(pPack+iHeadLen, pPackData, iPackDataLen);

	//У��ͣ�ͷ�������ݵĳ�����У��

	unsigned short crcValue = Crc16(pPack, headPack.wdTotalLen);
	BytesToString((const unsigned char *)(&crcValue), (char *)(&(headPack.dwdCheckCrc16)), sizeof(crcValue));

	//�ٽ���������д�������� str�ϲ���ͷ��������Ϣ����Ϣ��str���ڷ��ͣ�
	memcpy(pHeadData, &headPack, iHeadLen);
	memcpy(pPack, pHeadData, iHeadLen);
	memcpy(pPack+iHeadLen, pPackData, iPackDataLen);
	

	delete [] pHeadData;
	*pSendBuf = pPack;
	return headPack.wdTotalLen;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: ExecuteGetXMLCfg
DESCRIPTION: 
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
DWORD WINAPI ExecuteGetXMLCfg(LPVOID lpParameter)
{
	Serial2Device_S strSerial2Device;
	TCPServer *pTCPServer = (TCPServer *)lpParameter;

	while(true)
	{
		//get xml
		while (!gqueue_GetXMLCfg.empty())
		{
			std::pair<Serial2Device_S, SOCKET> pairValue;
			pairValue = gqueue_GetXMLCfg.front();
			strSerial2Device = pairValue.first;

			//��ȡ���ں����豸���͵������ļ�
			std::vector<Serial2Device>::iterator iter;
			Serial2Device_S getSerial2Device;
			memset((char *)(&getSerial2Device), '\0', sizeof(Serial2Device_S));
			bool foundFlag = false;
			char *pSendBuf;
			char *pBufValue;
			int totalLen;

			for (iter=gvecSerialNum2DeviceType.begin(); iter<gvecSerialNum2DeviceType.end(); iter++)
			{   //id��һ�������ߴ��ںź�chanelͬʱƥ��
				if (!strcmp(iter->id, strSerial2Device.id) || ((iter->serialNum == strSerial2Device.serialNum) && (iter->chanel == strSerial2Device.chanel)))
				{
					memcpy((char *)(&getSerial2Device), static_cast<void *>((&(*iter))), sizeof(Serial2Device_S));
					foundFlag = true;
					break;
				}
			}

			if (foundFlag)
			{
				//����Ϣ���ݸ��ͻ��� 
				//wangxintang
				//std::string idStr, int serialNumValue, int chanelValue, int deviceTypeValue, std::string nameStr
				//SX_EXIST_XML ��ʾ����xml��Ϣ
				pBufValue = GenerateSend2ClientJsonStr(SX_EXIST_XML, getSerial2Device.id, getSerial2Device.serialNum, getSerial2Device.chanel, getSerial2Device.deviceName, getSerial2Device.name);
			}
			else
			{
				//����û�л�ȡ��
				//SX_NOT_EXIST_XML ��ʾ����xml��Ϣ
				pBufValue = GenerateSend2ClientJsonStr(SX_NOT_EXIST_XML, getSerial2Device.id, getSerial2Device.serialNum, getSerial2Device.chanel, getSerial2Device.deviceName, getSerial2Device.name);		
			}

			totalLen = BuildPack(SX_RELAY_GET_XML, &pSendBuf, pBufValue, strlen(pBufValue));
			::send(pairValue.second, pSendBuf, totalLen, 0);

			gqueue_GetXMLCfg.pop();
		}
		Sleep(5);
	}
}


/*--------------------------------------------------------------------------
FUNCTION NAME: CreateSetXMLthread
DESCRIPTION: 
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::CreateSetXMLthread()
{
	m_XMLCfgThread = CreateThread(NULL,0,ExecuteSetXMLCfg,NULL,0,&m_XMLCfgThreadID);

	if(m_XMLCfgThread == NULL)
	{
		return false;
	}
	//CloseHandle(m_WorkThread);
	return true;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: ExecuteSetXMLCfg
DESCRIPTION: 
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
DWORD WINAPI ExecuteSetXMLCfg(LPVOID lpParameter)
{
	Serial2Device_S strSerial2Device;
	TCPServer *pTCPServer = (TCPServer *)lpParameter;

	while(true)
	{
		while(!gqueue_SetXMLCfg.empty())
		{
			std::pair<Serial2Device_S, SOCKET> pairValue;
			pairValue = gqueue_SetXMLCfg.front();
			strSerial2Device = pairValue.first;
			//дxml�����ļ����߶�xml�����ļ�

            //���strSerial2Device�к��г�id�������Ϣ���Ǿ�set��Ϣ

			bool writeFlag = pTCPServer->WriteXMLInfor(strSerial2Device);
			char *pBufValue;
			char *pSendBuf;
			int totalLen;
			Serial2Device_S send2Client;
			memset((char *)(&send2Client), '\0', sizeof(Serial2Device_S));

			if (writeFlag)
			{
				//�����޸ĳɹ����ͻ���
				pBufValue = GenerateSend2ClientJsonStr(SX_EXIST_XML, send2Client.id, send2Client.serialNum, send2Client.chanel, send2Client.deviceName, send2Client.name);
				totalLen = BuildPack(SX_RELAY_SET_XML, &pSendBuf, pBufValue, strlen(pBufValue));
				::send(pairValue.second, pSendBuf, totalLen, 0);
				//�޸��ڴ��еĴ����豸vector
				std::vector<Serial2Device>::iterator iterSerial2Device = gvecSerialNum2DeviceType.begin();
				for ( ; iterSerial2Device != gvecSerialNum2DeviceType.end(); iterSerial2Device++)
				{
					if (iterSerial2Device->serialNum == strSerial2Device.serialNum)
					{
						//�޸��豸���ͺͼ̵�������ڵ�����
						iterSerial2Device->deviceName = strSerial2Device.deviceName; 
						memset(iterSerial2Device->name, '\0', strlen(iterSerial2Device->name)+1);
						memcpy(iterSerial2Device->name, strSerial2Device.name, strlen(strSerial2Device.name)); 
					}
				}
			}
			else //write xmlʧ�ܣ�id���� ���� port��Chanel�����û�ж�
			{
				pBufValue = GenerateSend2ClientJsonStr(SX_NOT_EXIST_XML, send2Client.id, send2Client.serialNum, send2Client.chanel, send2Client.deviceName, send2Client.name);
				totalLen = BuildPack(SX_RELAY_SET_XML, &pSendBuf, pBufValue, strlen(pBufValue));
				::send(pairValue.second, pSendBuf, totalLen, 0);
			}

			gqueue_SetXMLCfg.pop();
		}
		Sleep(5);
	}
}



/*--------------------------------------------------------------------------
FUNCTION NAME: CreateExecuteReceivedRelayCmdhread
DESCRIPTION: 
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::CreateExecuteReceivedRelayCmdhread()
{
	m_WorkThread = CreateThread(NULL,0,ExecuteReceivedRelayCmd,NULL,0,&m_WorkThreadID);

	if(m_WorkThread == NULL)
	{
		return false;
	}
	//CloseHandle(m_WorkThread);
	return true;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: ExecuteReceivedRelayCmd
DESCRIPTION: ִ������ȫ�ֶ��������ݣ������̵�������
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
DWORD WINAPI ExecuteReceivedRelayCmd(LPVOID lpParameter)
{
	Relay_Cmd_S relayCmd;

	while(true)
	{
		while(!gqueue_ReceiveRelayCmd.empty())
		{
			relayCmd = gqueue_ReceiveRelayCmd.front();
			//�������ڼ̵�����Ӧ��
			ControlDeviceChanel(relayCmd.port, relayCmd.chanel, relayCmd.chanelStatus);
			gqueue_ReceiveRelayCmd.pop();
		}
		Sleep(5);
	}

	return 0;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: Start
DESCRIPTION: ������������˺�����˷��͵������ַ����߳�
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::Start()
{
	bool bRet = true;
	//���������߳�
	bRet = CreateAcceptThread();
	if (!bRet)
	{
		return bRet;
	}

	//����set xml������Ϣ�߳�
	bRet = CreateSetXMLthread();
	if (!bRet)
	{
		return bRet;
	}

	//get xml
	bRet = CreateGetXMLthread();
	if (!bRet)
	{
		return bRet;
	}

	//����ִ�����������߳�
	bRet = CreateExecuteReceivedRelayCmdhread();
	if (!bRet)
	{
		return bRet;
	}
	return bRet;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: CmdThread
DESCRIPTION: cmd socket�߳�
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
DWORD WINAPI CmdThread(LPVOID lpParameter)
{
	char szTemp[255] = {0};
	TCPServer *pTcpServer = (TCPServer*)lpParameter;
	if (NULL == pTcpServer)
	{
		return 0;
	}

	//��ȡ�ն���������������ŵ����ж�����
	while (true)
	{
		int port;
		int chanel; 
		int chanelStatus;
		//const int BUF_LEN = 100; 
		//char buf[BUF_LEN] = "c1 h2 on";
		//char buf[BUF_LEN] = {'\0'};
		//char *buf = new char[BUF_LEN];
		//memset(buf, '\0', BUF_LEN);
		std::string *buf = new std::string;
		getline(std::cin, *buf);
		//std::cout << "cmd input" << *buf << std::endl;
		//DebugLog("cmd input: %s\n", buf->c_str());
		//std::cin.getline(buf, BUF_LEN);
		while(true)
		{
			bool bRet = AnalyseCmdBuf((*buf).c_str(), port, chanel, chanelStatus);

			if (bRet)
			{
				//��������port chanel chanelStatus��һ���ṹ��洢������
				//ControlDeviceChanel(port, chanel, chanelStatus);
				Relay_Cmd_S relayCmd;
				relayCmd.port = port;
				relayCmd.chanel = chanel;
				relayCmd.chanelStatus = chanelStatus;
				//�����������ȫ�ֶ���
				pTcpServer->m_ReceivedCmdCri.Lock();
				gqueue_ReceiveRelayCmd.push(relayCmd);
				pTcpServer->m_ReceivedCmdCri.UnLock();
			}

			delete buf;
			buf = NULL;
			if (NULL == buf)
			{
				break;
			}
		}
		Sleep(5);
	}
	
	return 0;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: CreateCmdThread
DESCRIPTION: ����cmd input�߳�
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::CreateCmdThread()
{
	m_CmdThread = CreateThread(NULL,0,CmdThread,this,0,&m_CmdThreadID);

	if(m_CmdThread == NULL)
	{
		return false;
	}
	return true;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: AcceptThread
DESCRIPTION: accept socket�߳�
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
DWORD WINAPI AcceptThread(LPVOID lpParameter)
{
	SOCKET		mAcceptSocket;	///< ���ܵĿͻ����׽��־��
	sockaddr_in	mAcceptAddr;	///< ���յĿͻ��˵�ַ

	int nAcceptAddrLen = sizeof(mAcceptAddr);
	TCPServer* pTcpServer = (TCPServer*)lpParameter;

	while(true)
	{
		// ��������ʽ,�ȴ����տͻ�������
		const int liBuffLen=71680;//50kb����
		char szBuf[liBuffLen] = {0};
		int liOutLen=0;

		mAcceptSocket = ::accept(pTcpServer->GetServerSocket(), (struct sockaddr*)&mAcceptAddr, &nAcceptAddrLen);
		std::cout << "���ܿͻ���IP:" << inet_ntoa(mAcceptAddr.sin_addr) << std::endl;

		while (true)
		{
			int iRecvLen = pTcpServer->RecvData(mAcceptSocket, szBuf+liOutLen, liBuffLen-liOutLen);

			//���ճɹ�����������
			if (iRecvLen > 0)
			{
				iRecvLen += liOutLen;
				liOutLen=0;
				pTcpServer->DealData(szBuf, iRecvLen, liOutLen, mAcceptSocket);

				if(liOutLen > 0)
				{
					memcpy(szBuf, szBuf+iRecvLen-liOutLen, liOutLen);
				}
			}
		}
		Sleep(5);
	}
}


/*--------------------------------------------------------------------------
FUNCTION NAME: CreateAcceptThread
DESCRIPTION: ����accept socket�߳�
AUTHOR�������� 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::CreateAcceptThread()
{
	m_AcceptThread = CreateThread(NULL,0,AcceptThread,this,0,&m_AcceptThreadID);

	if(m_AcceptThread == NULL)
	{
		return false;
	}
	//CloseHandle(m_WorkThread);
	return true;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: GetTCPServerInstance
DESCRIPTION: ��ȡserverʵ��
AUTHOR�������� 
PARAMETERS: TCPServer����
RETURN: 
*-------------------------------------------------------------------------*/
TCPServer* TCPServer::GetTCPServerInstance()  
{  
	if(pInstance == NULL)  
	{   
		//ʹ����Դ�����࣬���׳��쳣��ʱ����Դ���������ᱻ�������������Ƿ�������������Ϊ�쳣�׳��������������  
		if(pInstance == NULL)  
		{  
			pInstance = new TCPServer();  
			//��ʼ��
			pInstance->InitServer();
		}  
	}  

	return pInstance;  
} 


/*--------------------------------------------------------------------------
FUNCTION NAME: TCPServer
DESCRIPTION: ˽�й��캯������ʼ��mServerSocket
AUTHOR�������� 
PARAMETERS: NULL
RETURN: 
*-------------------------------------------------------------------------*/
TCPServer::TCPServer(): mServerSocket(INVALID_SOCKET)
{
	 m_ReceivedCmdCri = CCriticalSection();
	 m_SetXMLCri      = CCriticalSection();
	 m_GetXMLCri      = CCriticalSection();
}


void TCPServer::InitServer()
{
	// �����׽���
	mServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (mServerSocket == INVALID_SOCKET)
	{
		std::cout << "�����׽���ʧ��!" << std::endl;
		return;
	}

	//��ȡ�����ļ����ip��port
	//1����ȡ�����ļ���·��������
	char szPath[256]={0};
	GetModuleFileName(NULL, szPath, 256);
	std::string strPath = szPath;
	int nPoz = strPath.find_last_of('\\');
	strPath = strPath.substr(0,nPoz+1);
	strPath += "IpAndPort.ini";

	//2���������ļ��л�ȡip��port
	char cIpArray[20];
	int iIpLen = GetPrivateProfileString("SECTION 1", "ip", "127.0.0.1", cIpArray, 20, strPath.c_str());
	//2.1 ��ȡ��ipStr
	//std::cout << "ip: " << cIpArray << std::endl;
	//2.2 ��ȡ�Ķ˿ں�
	int port = GetPrivateProfileInt("SECTION 2", "port", 20000, strPath.c_str());
	//std::cout << "port: " << port << std::endl;

	// ����������IP�Ͷ˿ں�
	/*
	mServerAddr.sin_family		= AF_INET;
	mServerAddr.sin_addr.s_addr	= INADDR_ANY;
	mServerAddr.sin_port		= htons((u_short)SERVER_PORT);
	*/

	mServerAddr.sin_family		= AF_INET;
	mServerAddr.sin_addr.s_addr	= INADDR_ANY;
	mServerAddr.sin_port		= htons((u_short)port);


	// ��IP�Ͷ˿�
	if ( ::bind(mServerSocket, (sockaddr*)&mServerAddr, sizeof(mServerAddr)) == SOCKET_ERROR)
	{
		std::cout << "��IP�Ͷ˿�ʧ��!" << std::endl;
		return;
	}

	// �����ͻ�������,���ͬʱ����������Ϊ10.
	if ( ::listen(mServerSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "�����˿�ʧ��!" << std::endl;
		return;
	}

	std::cout << "����TCP�������ɹ�!" << std::endl;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: ~TCPServer
DESCRIPTION: ����server
AUTHOR�������� 
PARAMETERS: NULL
RETURN: 
*-------------------------------------------------------------------------*/

TCPServer::~TCPServer()
{
	::closesocket(mServerSocket);
	std::cout << "�ر�TCP�������ɹ�!" << std::endl;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: RecvData
DESCRIPTION: ��������
AUTHOR�������� 
PARAMETERS: pszRecvData 
RETURN: 
*-------------------------------------------------------------------------*/
int TCPServer::RecvData(const SOCKET & acceptSocket, char *pszRecvData, int aiBuffLen)
{
	
	int iLen = recv(acceptSocket, pszRecvData, aiBuffLen,0); 

	if(iLen == 0)
	{
		return iLen;
	}
	else if(iLen == SOCKET_ERROR)
	{
		closesocket(acceptSocket);
		return iLen;
	}

	return iLen;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: AnalyseCmdPortChanelStatus
DESCRIPTION: ���������ݶ��а�����port chanel chanelStatus
AUTHOR�������� 
PARAMETERS: pszRecvDataȥ������ͷ������ݶΣ���ʽ��com1 chanel1 on , com1 chanel2 off ...
            iLen pszRecvData��ָ���ݶγ���
			port ���ں� 1-16
			chanel ���� 1-4 & 1-16
			chanelStatus ״̬ 1������ 2���� 
RETURN: 
*-------------------------------------------------------------------------*/
void TCPServer::AnalyseCmdPortChanelStatus(char *pszRecvData, 
										   const int & iLen, 
										   int & port, 
										   int & chanel, 
										   int & chanelStatus)
{
	//���Ȳ�Ϊ��ֵ
	if(NULL == pszRecvData || iLen <= 0)
		return;
	//pszRecvData ��ʽΪcom1 chanel1 on, com1 chanel1 off
	std::string recvStr(pszRecvData, pszRecvData+iLen);
	std::string::size_type index = 0;
	std::string::size_type loc = 0;
	loc = recvStr.find(",", index);

	while(std::string::npos != loc)
	{
		//��ȡ�Ӵ�
		std::string strTemp(recvStr, index, loc-index);
		//������port chanel chanelStatus
		bool bRet = AnalyseCmdBuf(strTemp, port, chanel, chanelStatus);
		if(bRet)
		{
			index = loc;

			//�ÿո����","�������ҳ��ڶ���","��λ��
			recvStr.at(index) = ' ';
			loc = recvStr.find(",", index);
		}
		else
		{
			//����Ĵ���ʽ����
		}
		
	}

	//���һ�������ֺ�û��","
	if(std::string::npos == loc)
	{
		//���һ�����
		std::string strTemp(recvStr, index, recvStr.length()-index);
		bool bRet = AnalyseCmdBuf(strTemp, port, chanel, chanelStatus);
		if(bRet)
		{
			//�ַ�����ʽ��ȷ�������ɹ�
		}
		else
		{
			//����Ĵ���ʽ����
		}
	}

	return;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: DealData
DESCRIPTION: �����������
AUTHOR�������� 
PARAMETERS: pszRecvData�����ַ����� iLenΪpszRecvData���ݳ��ȣ� iOutLenδ�������ݳ���
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::DealData(char *pszRecvData,int iLen, int & iOutLen, const SOCKET & acceptSocket)
{
	if (NULL == pszRecvData || iLen <= 0)
	{
		return false;
	}

	STRU_HEAD_PACK_S strPackHead;
	const int iHeadSize=sizeof(STRU_HEAD_PACK_S);
	memset(&strPackHead, 0, iHeadSize);
	memcpy(&strPackHead, pszRecvData, iHeadSize);
	
	//�ж�ͷ�Ƿ�Ϊ0x33aa �����Ͼ�ֱ�Ӷ���
	if (strPackHead.wdPackHead != SX_HEAD_FUNID)
	{
		return false;
	}

	//������У��λ���ж��Ƿ�ϸ�, �û�õ�У��λ���Լ������õļ���λ�Ƚ�

	unsigned short usCrcValueGet = 0;
	StringToBytes((char *)(&(strPackHead.dwdCheckCrc16)), (unsigned char *)(&usCrcValueGet), sizeof(strPackHead.dwdCheckCrc16));
	strPackHead.dwdCheckCrc16 = SX_CRC16_INIT;
	char *pPack = new char[strPackHead.wdTotalLen];
	memcpy(pPack, (char *)(&strPackHead), iHeadSize);
	memcpy(pPack+iHeadSize, pszRecvData+iHeadSize, strPackHead.wdTotalLen-iHeadSize);

	unsigned short usCrcValueCalc = Crc16(pPack, strPackHead.wdTotalLen);
	if(usCrcValueGet != usCrcValueCalc)
	{
		return false;
	}
	

	ULONG ulTotalPackLenth = strPackHead.wdTotalLen;
	//�����̵����������
	if ((SX_RELAY_CMD == strPackHead.byHeadDataType) && (iLen >= ulTotalPackLenth))
	{
		if (iLen >= iHeadSize)
		{		
			Relay_Cmd_S relayCmd;
			memcpy((char *)(&relayCmd), pszRecvData+iHeadSize, strPackHead.wdTotalLen-iHeadSize);

			//std::cout << "had AnalyseCmdPortChanelStatus port chanel status:" << relayCmd.port << relayCmd.chanel << relayCmd.chanelStatus << std::endl;
			//������port chanel chanelStatus��Ȼ�󽫽����Ľ���ŵ�ȫ��cmd������

			m_ReceivedCmdCri.Lock();
			gqueue_ReceiveRelayCmd.push(relayCmd);
			m_ReceivedCmdCri.UnLock();
			//2������port chanel chanelStatus�����̵���
			//ControlDeviceChanel(port, chanel, chanelStatus);

			iLen -= ulTotalPackLenth;
		}
	}
	else if ((SX_RELAY_JSON == strPackHead.byHeadDataType) && (iLen >= ulTotalPackLenth))
	{
		//����json������Ϣ
		//�Ȼ�ý��յ����ַ���������
		if (iLen >= iHeadSize)
		{
			char *pJsonStr = new char[strPackHead.wdTotalLen-iHeadSize+1];
			memset(pJsonStr, '\0', strPackHead.wdTotalLen-iHeadSize+1);
			memcpy(pJsonStr, pszRecvData+iHeadSize, strPackHead.wdTotalLen-iHeadSize);

			//std::cout << "pJsonStr: " << pJsonStr << std::endl;

			Json::Reader reader;
			Json::Value root;
			Relay_Cmd_S relayCmd;
			// json��ʽ��"{\"port\": portValue,\"chanel\": chanelValue,\"chanelStatus\": chanelStatusValue}"
			if (reader.parse(pJsonStr, root))
			{
				relayCmd.port = root["port"].asInt();
				relayCmd.chanel = root["chanel"].asInt();
				relayCmd.chanelStatus = root["chanelStatus"].asInt();
			}

			m_ReceivedCmdCri.Lock();
			gqueue_ReceiveRelayCmd.push(relayCmd);
			m_ReceivedCmdCri.UnLock();
			//2������port chanel chanelStatus�����̵���
			//ControlDeviceChanel(port, chanel, chanelStatus);
			iLen -= ulTotalPackLenth;
		}
	}
	else if ((SX_RELAY_SET_XML == strPackHead.byHeadDataType) && (iLen >= ulTotalPackLenth))
	{
		if (iLen >= iHeadSize)
		{
			char *pXMLStr = new char[strPackHead.wdTotalLen-iHeadSize+1];
			memset(pXMLStr, '\0', strPackHead.wdTotalLen-iHeadSize+1);
			memcpy(pXMLStr, pszRecvData+iHeadSize, strPackHead.wdTotalLen-iHeadSize);

			/*
			char id[20];
			int serialNum;
			int chanel;
			int deviceName; 
			char name[20];
			*/
			//�����õ�������Ϣ
			Json::Reader reader;
			Json::Value root;
			Serial2Device strSerial2Device;
			memset((char *)(&strSerial2Device), '\0', sizeof(strSerial2Device));
			//std::cout << "pXMLStr: " << pXMLStr << std::endl;
			//"{\"id\": idValue,\"serialNum\": serialNumValue,\"chanel\": chanelValue,\"deviceType\": deviceTypeValue,\"name\": nameStr}"
			if (reader.parse(pXMLStr, root))
			{
				//strSerial2Device.id,root["id"].asString();
				memcpy(strSerial2Device.id, root["id"].asCString(), strlen(root["id"].asCString()));
				strSerial2Device.serialNum = root["serialNum"].asInt();
				strSerial2Device.chanel = root["chanel"].asInt();
				strSerial2Device.deviceName = root["deviceType"].asInt();
				memcpy(strSerial2Device.name, root["name"].asCString(), strlen(root["name"].asCString()));
			}

			m_SetXMLCri.Lock();
			gqueue_SetXMLCfg.push(std::make_pair(strSerial2Device, acceptSocket));
			m_SetXMLCri.UnLock();
		}
	}
	else if ((SX_RELAY_GET_XML == strPackHead.byHeadDataType) && (iLen >= ulTotalPackLenth))
	{
		if (iLen >= iHeadSize)
		{
			char *pXMLStr = new char[strPackHead.wdTotalLen-iHeadSize+1];
			memset(pXMLStr, '\0', strPackHead.wdTotalLen-iHeadSize+1);
			memcpy(pXMLStr, pszRecvData+iHeadSize, strPackHead.wdTotalLen-iHeadSize);

			/*
			char id[20];
			int serialNum;
			int chanel;
			int deviceName; 
			char name[20];
			*/
			//�����õ�������Ϣ
			Json::Reader reader;
			Json::Value root;
			Serial2Device strSerial2Device;
			memset((char *)(&strSerial2Device), '\0', sizeof(strSerial2Device));
			//std::cout << "pXMLStr: " << pXMLStr << std::endl;
			//"{\"id\": idValue,\"serialNum\": serialNumValue,\"chanel\": chanelValue,\"deviceType\": deviceTypeValue,\"name\": nameStr}"
			if (reader.parse(pXMLStr, root))
			{
				//strSerial2Device.id,root["id"].asString();
				memcpy(strSerial2Device.id, root["id"].asCString(), strlen(root["id"].asCString()));
				strSerial2Device.serialNum = root["serialNum"].asInt();
				strSerial2Device.chanel = root["chanel"].asInt();
				strSerial2Device.deviceName = root["deviceType"].asInt();
				memcpy(strSerial2Device.name, root["name"].asCString(), strlen(root["name"].asCString()));
			}

			gqueue_GetXMLCfg.push(std::make_pair(strSerial2Device, acceptSocket));
		}
	}
	else  //����ͷ��û�н�����ȫ
	{
		if(iLen < iHeadSize)
		{
			iOutLen=iLen;
		}
		return false;
	}

	//������߼������������
	//��ͷ�����ݶ�����
	//��ͷ���������ݲ�����
	//��ͷ������
	if (iLen > iHeadSize)
	{
		memset(&strPackHead, 0, iHeadSize);
		memcpy(&strPackHead, pszRecvData+ulTotalPackLenth, iHeadSize);
		ULONG ulTempTotalPackLenth = strPackHead.wdTotalLen;
		
		//����ʵ�ʳ���С��ͷ�еĳ��ȱ��λ
		if (iLen < ulTempTotalPackLenth) //��ͷ���������ݲ�����
		{
			iOutLen=iLen;
			return true;
		}

		return DealData(pszRecvData+ulTotalPackLenth, iLen, iOutLen, acceptSocket); //��ͷ�����ݶ�����
	}
	else if(iLen <= iHeadSize) //С�ڰ�ͷ
	{
		iOutLen = iLen;
		return true;
	}
}


/*--------------------------------------------------------------------------
FUNCTION NAME: Crc16
DESCRIPTION: ����crcУ����
AUTHOR�������� 
PARAMETERS: pBuf��У������,nLen��У�����ݵĳ���,��������У����
RETURN: 
*-------------------------------------------------------------------------*/
unsigned short TCPServer::Crc16(const char *pBuf, unsigned short nLen)
{
	BYTE i;
	unsigned short crc = 0;

	while (nLen--)
	{
		for (i = 0x80; i != 0; i >>= 1)
		{
			if ((crc&0x8000) != 0)
			{
				crc <<= 1;
				crc ^= 0x1021;
			}
			else
			{
				crc <<= 1;
			}
			if ((*pBuf&i) != 0)
			{
				crc ^= 0x1021;
			}
		}
		pBuf++;
	}

	return crc; 
}


/*--------------------------------------------------------------------------
FUNCTION NAME: run
DESCRIPTION: ����server
AUTHOR�������� 
PARAMETERS: NULL
RETURN: VOID
*-------------------------------------------------------------------------*/
//void TCPServer::run()
//{
//	int nAcceptAddrLen = sizeof(mAcceptAddr);
//	while(true)
//	{
//		// ��������ʽ,�ȴ����տͻ�������
//		const int liBuffLen=71680;//50kb����
//		char szBuf[liBuffLen] = {0};
//		int liOutLen=0;
//
//
//
//		mAcceptSocket = ::accept(mServerSocket, (struct sockaddr*)&mAcceptAddr, &nAcceptAddrLen);
//		std::cout << "���ܿͻ���IP:" << inet_ntoa(mAcceptAddr.sin_addr) << std::endl;
//
//		while (true)
//		{
//			//int iRecvLen = ::recv(mAcceptSocket, szBuf, liBuffLen, 0);
//			int iRecvLen = RecvData(szBuf+liOutLen, liBuffLen-liOutLen);
//
//			//���ճɹ�����������
//			if (iRecvLen > 0)
//			{
//				iRecvLen += liOutLen;
//				liOutLen=0;
//				DealData(szBuf, iRecvLen, liOutLen);
//
//				if(liOutLen > 0)
//				{
//					memcpy(szBuf, szBuf+iRecvLen-liOutLen, liOutLen);
//				}
//			}
//		}
//	}
//}


/*--------------------------------------------------------------------------
FUNCTION NAME: getVecSerialnum2DeviceType
DESCRIPTION: ��ȡ���ڵ��豸����vector
AUTHOR�������� 
PARAMETERS: NULL
RETURN: �������ڶ�Ӧ���豸���͵�vector
*-------------------------------------------------------------------------*/
std::vector<Serial2Device> TCPServer::getVecSerialnum2DeviceType()
{
	return this->vecSerialnum2DeviceType;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: WriteXMLInfor
DESCRIPTION: ���������豸�Ķ�Ӧ��Ϣд��xml�ļ������޸�ԭ�е�xml��Ϣ
AUTHOR�������� 
PARAMETERS: WriteXMLInfor
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::WriteXMLInfor(const Serial2Device & serial2device)
{

	char _szPath[256]={0};
	const char* pTmp = NULL;
	GetModuleFileName(NULL, _szPath, 256);
	std::string strPath = _szPath;
	int nPoz = strPath.find_last_of('\\');
	strPath = strPath.substr(0,nPoz+1);
	strPath += "serialToDevice.xml";

	/*TiXmlDocument *myDocument = new TiXmlDocument(strPath.c_str());
	if (!myDocument->LoadFile())
	{
		return false;
	}*/

	char szTemp[255] = {0};
	/*
	TiXmlDocument loXmlDoc(strPath.c_str());
	loXmlDoc.Parse(strPath.c_str());
	*/

	TiXmlDocument *loXmlDoc = new TiXmlDocument(strPath.c_str());
	if (!loXmlDoc->LoadFile())
	{
		return false;
	}

	TiXmlPrinter  lTiXmlPrinter;
	TiXmlElement *pRoot = loXmlDoc->RootElement();

	if (pRoot)
	{   
		//
		bool bFoundNodeFlag = false;
		TiXmlElement *lpSerialCountNode = pRoot->FirstChildElement("SerialCount");
		TiXmlElement *lpServerInforNode = lpSerialCountNode->FirstChildElement("CSGSDK.dll");
		while (lpServerInforNode)
		{
			TiXmlElement * lpItemlNode = lpServerInforNode->FirstChildElement("id");
			if (lpItemlNode == NULL)
			{
				lpServerInforNode = lpServerInforNode->NextSiblingElement();
				continue;
			}

			if (lpItemlNode != NULL)
			{
				const char *pStr = lpItemlNode->FirstChild()->Value();

				if(!strcmp(pStr, serial2device.id))
				{
					bFoundNodeFlag = true; 
					lpItemlNode = lpServerInforNode->FirstChildElement("serialNum");
					if (lpItemlNode != NULL)
					{
						TiXmlElement *temp = new TiXmlElement("serialNum");
						char buf[10] = {'\0'};				
						sprintf(buf, "%d", serial2device.serialNum);		
						TiXmlText *serialNum = new TiXmlText(buf);						
						temp->LinkEndChild(serialNum);
						lpServerInforNode->ReplaceChild(lpItemlNode, *temp);
									
						//lpItemlNode->ReplaceChild(lpItemlNode, );
						//lpItemlNode->SetValue(buf);
						//std::cout << "buf serialNum:    " << buf << std::endl;
					}

					lpItemlNode = lpServerInforNode->FirstChildElement("chanel");
					if (lpItemlNode != NULL)
					{				
						TiXmlElement *temp = new TiXmlElement("chanel");
						char buf[10] = {'\0'};
						sprintf(buf, "%d", serial2device.chanel);
						TiXmlText *chanel = new TiXmlText(buf);
						temp->LinkEndChild(chanel);
						lpServerInforNode->ReplaceChild(lpItemlNode, *temp);
						//lpItemlNode->SetValue(buf);
						//std::cout << "buf chanel:    " << buf << std::endl;
					}

					lpItemlNode = lpServerInforNode->FirstChildElement("deviceType");
					if (lpItemlNode != NULL)
					{
						char buf[10] = {'\0'};
						sprintf(buf, "%d", serial2device.deviceName);
						TiXmlText *deviceName = new TiXmlText(buf);
						TiXmlElement *temp = new TiXmlElement("deviceType");
						temp->LinkEndChild(deviceName);
						lpServerInforNode->ReplaceChild(lpItemlNode, *temp);

						//lpItemlNode->SetValue(buf);
						//std::cout << "buf deviceType:    " << buf << std::endl;
					}

					lpItemlNode = lpServerInforNode->FirstChildElement("name");
					if (lpItemlNode != NULL)
					{
						TiXmlText *name = new TiXmlText(serial2device.name);
						TiXmlElement *temp = new TiXmlElement("name");
						temp->LinkEndChild(name);
						lpServerInforNode->ReplaceChild(lpItemlNode, *temp);

						//lpItemlNode->SetValue(serial2device.name);
						//std::cout << "buf name:    " << serial2device.name << std::endl;
					}

					break;
				}	
			}

			lpServerInforNode = lpServerInforNode->NextSiblingElement();
		}
		//û���ҵ��ڵ�
		if (!bFoundNodeFlag)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	//(*loXmlDoc).Accept( &lTiXmlPrinter );
	//std::string lsConfigXmlInfo = lTiXmlPrinter.CStr(); //�޸��Ժ�׼�������豸��xml��Ϣ

	//std::cout << "lsConfigXmlInfo = " << lsConfigXmlInfo << std::endl;
	loXmlDoc->SaveFile();

	return true;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: ReadXMLInfor
DESCRIPTION: ��ȡ���ڵ��豸��������xml�ļ�����������浽vecSerialnum2DeviceType
AUTHOR�������� 
PARAMETERS: NULL
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::ReadXMLInfor()
{   
	char _szPath[256]={0};
	const char* pTmp = NULL;
	GetModuleFileName(NULL, _szPath, 256);
	std::string strPath = _szPath;
	int nPoz = strPath.find_last_of('\\');
	strPath = strPath.substr(0,nPoz+1);
	strPath += "serialToDevice.xml";
	//CLog::GetInstance()->AddLog(strPath.c_str());
	TiXmlDocument *myDocument = new TiXmlDocument(strPath.c_str());
	if (!myDocument->LoadFile())
	{
		//CLog::GetInstance()->AddLog("Guard_Process>::��ȡxml���ò���ʧ��!");
		return false;
	}

	//��ø�Ԫ��
	TiXmlElement *RootElement = myDocument->RootElement();
	TiXmlElement *lpServerCount = RootElement->FirstChildElement("SerialCount");
	TiXmlElement *lpServerInfor = lpServerCount->FirstChildElement("CSGSDK.dll");//����ServerCount��Ӧ���ֶ� 
	while (lpServerInfor!=NULL)
	{
		Serial2Device serial2Device = {0};
		TiXmlElement *lpElement = lpServerInfor->FirstChildElement("id");
		if (lpElement == NULL)
		{
			lpServerInfor = lpServerInfor->NextSiblingElement();
			continue;
		}
		if (lpElement != NULL)
		{
			pTmp = lpElement->FirstChild()->Value();
			strcpy(serial2Device.id, pTmp);
		}

		lpElement = lpServerInfor->FirstChildElement("serialNum");
		if (lpElement != NULL)
		{
			pTmp = lpElement->FirstChild()->Value();
			serial2Device.serialNum = atoi(pTmp);
		}

		lpElement = lpServerInfor->FirstChildElement("chanel");
		if (lpElement != NULL)
		{
			pTmp = lpElement->FirstChild()->Value();
			serial2Device.chanel = atoi(pTmp);
		}

		lpElement = lpServerInfor->FirstChildElement("deviceType");
		if (lpElement != NULL)
		{
			pTmp = lpElement->FirstChild()->Value();
			serial2Device.deviceName =  atoi(pTmp);			
		}

		lpElement = lpServerInfor->FirstChildElement("name");
		if (lpElement != NULL)
		{
			pTmp = lpElement->FirstChild()->Value();
			strcpy(serial2Device.name, pTmp);	
		}

		{ 
			//CAutoLock thisLock(&m_ServerInforLock);
			vecSerialnum2DeviceType.push_back(serial2Device);
		}

		lpServerInfor = lpServerInfor->NextSiblingElement();
	}
	if (myDocument)
	{
		delete myDocument;
		myDocument = NULL;
	}
	if (vecSerialnum2DeviceType.size() > 0)
	{
		return true;
	}

	return false;
}

