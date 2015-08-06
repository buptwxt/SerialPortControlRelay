#include "Server.h"

#pragma comment(lib,"tiny/tinyxml.lib")

std::vector<Serial2Device> gvecSerialNum2DeviceType;
TCPServer* TCPServer::pInstance = 0;  
//全局队列，存放从网页端接收和从
std::queue<Relay_Cmd_S> gqueue_ReceiveRelayCmd;
//set xml配置
std::queue<std::pair <Serial2Device, SOCKET>> gqueue_SetXMLCfg;
//get xml配置
std::queue<std::pair <Serial2Device, SOCKET>> gqueue_GetXMLCfg;

//求绝对值
#define ABS(cond) (cond>0 ? cond: -cond)

//整数转字符串
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


//产生json字符串，用于传输xml配置信息
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
DESCRIPTION: 校验码
AUTHOR：王鑫堂 
PARAMETERS: pBuf：校验数据,nLen：校验数据的长度,函数返回校验码
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
DESCRIPTION: 合并相邻的单字节的数据转换为对应的双字节的数据
PARAMETERS:  
RETURN: 返回转化后的长度
*-------------------------------------------------------------------------*/
int BytesToString(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	if(pSrc == NULL)
		return -1;
	int i; 
	const char tab[]="0123456789ABCDEF";	// 0x0-0xf的字符查找表
	for (i = 0; i < nSrcLength; i++)
	{
		*pDst++ = tab[*pSrc >> 4];		// 输出高4位
		*pDst++ = tab[*pSrc & 0x0f];	// 输出低4位
		pSrc++;
	}
	// 返回目标字符串长度
	return (nSrcLength * 2);
}

/*--------------------------------------------------------------------------
FUNCTION NAME: StringToBytes
DESCRIPTION: 双字节的数据转换为对应的单字节的数据
PARAMETERS:  
RETURN: 返回转化后的长度
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
		// 输出高4位
		if ((*pSrc >= '0') && (*pSrc <= '9'))
		{
			*pDst = (*pSrc - '0') << 4;
		}
		else
		{
			*pDst = (*pSrc - 'A' + 10) << 4;
		}
		pSrc++;

		// 输出低4位
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
	// 返回目标数据长度
	return (nSrcLength / 2);
}



/*--------------------------------------------------------------------------
FUNCTION NAME: CreateGetXMLhread
DESCRIPTION: 
AUTHOR：王鑫堂 
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
DESCRIPTION: 制作报文头函数
PARAMETERS: headDataType 命令，json， get/set XML
RETURN:  返回包头转化后的长度  "-1"失败
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
	//校验和先设置为初始值
	headPack.dwdCheckCrc16 = SX_CRC16_INIT;
	headPack.byHeadDataType = headDataType;
	headPack.wdTotalLen = iHeadLen + iPackDataLen;

	//申请缓冲区保存待发数据
	char* pHeadData=new char[iHeadLen];
	if (pHeadData == NULL)
	{
		return -1;
	}
	memset(pHeadData,0,iHeadLen);

	//拷贝数据至缓冲区以作校验码生成用
	memcpy(pHeadData, &headPack, iHeadLen);
	//合并头和数据 pPackData + pHeadData => strTemp, 用于计算crc值

	//申请一个大的内存空间，用于存放头和数据信息
	char *pPack = new char[iHeadLen + iPackDataLen];
	memcpy(pPack, pHeadData, iHeadLen);
	memcpy(pPack+iHeadLen, pPackData, iPackDataLen);

	//校验和，头加上数据的长度来校验

	unsigned short crcValue = Crc16(pPack, headPack.wdTotalLen);
	BytesToString((const unsigned char *)(&crcValue), (char *)(&(headPack.dwdCheckCrc16)), sizeof(crcValue));

	//再将数据重新写到缓冲区 str合并了头和数据信息的信息，str用于发送，
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
AUTHOR：王鑫堂 
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

			//获取串口号与设备类型的配置文件
			std::vector<Serial2Device>::iterator iter;
			Serial2Device_S getSerial2Device;
			memset((char *)(&getSerial2Device), '\0', sizeof(Serial2Device_S));
			bool foundFlag = false;
			char *pSendBuf;
			char *pBufValue;
			int totalLen;

			for (iter=gvecSerialNum2DeviceType.begin(); iter<gvecSerialNum2DeviceType.end(); iter++)
			{   //id号一样，或者串口号和chanel同时匹配
				if (!strcmp(iter->id, strSerial2Device.id) || ((iter->serialNum == strSerial2Device.serialNum) && (iter->chanel == strSerial2Device.chanel)))
				{
					memcpy((char *)(&getSerial2Device), static_cast<void *>((&(*iter))), sizeof(Serial2Device_S));
					foundFlag = true;
					break;
				}
			}

			if (foundFlag)
			{
				//把消息传递给客户端 
				//wangxintang
				//std::string idStr, int serialNumValue, int chanelValue, int deviceTypeValue, std::string nameStr
				//SX_EXIST_XML 表示存在xml信息
				pBufValue = GenerateSend2ClientJsonStr(SX_EXIST_XML, getSerial2Device.id, getSerial2Device.serialNum, getSerial2Device.chanel, getSerial2Device.deviceName, getSerial2Device.name);
			}
			else
			{
				//传递没有获取到
				//SX_NOT_EXIST_XML 表示存在xml信息
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
AUTHOR：王鑫堂 
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
AUTHOR：王鑫堂 
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
			//写xml配置文件或者度xml配置文件

            //如果strSerial2Device中含有除id意外的信息，那就set信息

			bool writeFlag = pTCPServer->WriteXMLInfor(strSerial2Device);
			char *pBufValue;
			char *pSendBuf;
			int totalLen;
			Serial2Device_S send2Client;
			memset((char *)(&send2Client), '\0', sizeof(Serial2Device_S));

			if (writeFlag)
			{
				//传递修改成功给客户端
				pBufValue = GenerateSend2ClientJsonStr(SX_EXIST_XML, send2Client.id, send2Client.serialNum, send2Client.chanel, send2Client.deviceName, send2Client.name);
				totalLen = BuildPack(SX_RELAY_SET_XML, &pSendBuf, pBufValue, strlen(pBufValue));
				::send(pairValue.second, pSendBuf, totalLen, 0);
				//修改内存中的串口设备vector
				std::vector<Serial2Device>::iterator iterSerial2Device = gvecSerialNum2DeviceType.begin();
				for ( ; iterSerial2Device != gvecSerialNum2DeviceType.end(); iterSerial2Device++)
				{
					if (iterSerial2Device->serialNum == strSerial2Device.serialNum)
					{
						//修改设备类型和继电器输出口的名称
						iterSerial2Device->deviceName = strSerial2Device.deviceName; 
						memset(iterSerial2Device->name, '\0', strlen(iterSerial2Device->name)+1);
						memcpy(iterSerial2Device->name, strSerial2Device.name, strlen(strSerial2Device.name)); 
					}
				}
			}
			else //write xml失败，id不对 或者 port和Chanel的组合没有对
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
AUTHOR：王鑫堂 
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
DESCRIPTION: 执行命令全局队列中数据，操作继电器的命
AUTHOR：王鑫堂 
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
			//操作串口继电器对应的
			ControlDeviceChanel(relayCmd.port, relayCmd.chanel, relayCmd.chanelStatus);
			gqueue_ReceiveRelayCmd.pop();
		}
		Sleep(5);
	}

	return 0;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: Start
DESCRIPTION: 启动接收网络端和命令端发送的命令字符串线程
AUTHOR：王鑫堂 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
bool TCPServer::Start()
{
	bool bRet = true;
	//启动监听线程
	bRet = CreateAcceptThread();
	if (!bRet)
	{
		return bRet;
	}

	//启动set xml配置消息线程
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

	//启动执行输入命令线程
	bRet = CreateExecuteReceivedRelayCmdhread();
	if (!bRet)
	{
		return bRet;
	}
	return bRet;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: CmdThread
DESCRIPTION: cmd socket线程
AUTHOR：王鑫堂 
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

	//获取终端输入的命令，将命令放到队列队列中
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
				//将解析的port chanel chanelStatus用一个结构体存储起来，
				//ControlDeviceChanel(port, chanel, chanelStatus);
				Relay_Cmd_S relayCmd;
				relayCmd.port = port;
				relayCmd.chanel = chanel;
				relayCmd.chanelStatus = chanelStatus;
				//解析结果放入全局队列
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
DESCRIPTION: 创建cmd input线程
AUTHOR：王鑫堂 
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
DESCRIPTION: accept socket线程
AUTHOR：王鑫堂 
PARAMETERS: 
RETURN: 
*-------------------------------------------------------------------------*/
DWORD WINAPI AcceptThread(LPVOID lpParameter)
{
	SOCKET		mAcceptSocket;	///< 接受的客户端套接字句柄
	sockaddr_in	mAcceptAddr;	///< 接收的客户端地址

	int nAcceptAddrLen = sizeof(mAcceptAddr);
	TCPServer* pTcpServer = (TCPServer*)lpParameter;

	while(true)
	{
		// 以阻塞方式,等待接收客户端连接
		const int liBuffLen=71680;//50kb数据
		char szBuf[liBuffLen] = {0};
		int liOutLen=0;

		mAcceptSocket = ::accept(pTcpServer->GetServerSocket(), (struct sockaddr*)&mAcceptAddr, &nAcceptAddrLen);
		std::cout << "接受客户端IP:" << inet_ntoa(mAcceptAddr.sin_addr) << std::endl;

		while (true)
		{
			int iRecvLen = pTcpServer->RecvData(mAcceptSocket, szBuf+liOutLen, liBuffLen-liOutLen);

			//接收成功，正常处理
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
DESCRIPTION: 创建accept socket线程
AUTHOR：王鑫堂 
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
DESCRIPTION: 获取server实例
AUTHOR：王鑫堂 
PARAMETERS: TCPServer对象
RETURN: 
*-------------------------------------------------------------------------*/
TCPServer* TCPServer::GetTCPServerInstance()  
{  
	if(pInstance == NULL)  
	{   
		//使用资源管理类，在抛出异常的时候，资源管理类对象会被析构，析构总是发生的无论是因为异常抛出还是语句块结束。  
		if(pInstance == NULL)  
		{  
			pInstance = new TCPServer();  
			//初始化
			pInstance->InitServer();
		}  
	}  

	return pInstance;  
} 


/*--------------------------------------------------------------------------
FUNCTION NAME: TCPServer
DESCRIPTION: 私有构造函数，初始化mServerSocket
AUTHOR：王鑫堂 
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
	// 创建套接字
	mServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (mServerSocket == INVALID_SOCKET)
	{
		std::cout << "创建套接字失败!" << std::endl;
		return;
	}

	//读取配置文件获得ip和port
	//1、获取配置文件的路径和名称
	char szPath[256]={0};
	GetModuleFileName(NULL, szPath, 256);
	std::string strPath = szPath;
	int nPoz = strPath.find_last_of('\\');
	strPath = strPath.substr(0,nPoz+1);
	strPath += "IpAndPort.ini";

	//2、从配置文件中获取ip和port
	char cIpArray[20];
	int iIpLen = GetPrivateProfileString("SECTION 1", "ip", "127.0.0.1", cIpArray, 20, strPath.c_str());
	//2.1 获取的ipStr
	//std::cout << "ip: " << cIpArray << std::endl;
	//2.2 获取的端口号
	int port = GetPrivateProfileInt("SECTION 2", "port", 20000, strPath.c_str());
	//std::cout << "port: " << port << std::endl;

	// 填充服务器的IP和端口号
	/*
	mServerAddr.sin_family		= AF_INET;
	mServerAddr.sin_addr.s_addr	= INADDR_ANY;
	mServerAddr.sin_port		= htons((u_short)SERVER_PORT);
	*/

	mServerAddr.sin_family		= AF_INET;
	mServerAddr.sin_addr.s_addr	= INADDR_ANY;
	mServerAddr.sin_port		= htons((u_short)port);


	// 绑定IP和端口
	if ( ::bind(mServerSocket, (sockaddr*)&mServerAddr, sizeof(mServerAddr)) == SOCKET_ERROR)
	{
		std::cout << "绑定IP和端口失败!" << std::endl;
		return;
	}

	// 监听客户端请求,最大同时连接数设置为10.
	if ( ::listen(mServerSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "监听端口失败!" << std::endl;
		return;
	}

	std::cout << "启动TCP服务器成功!" << std::endl;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: ~TCPServer
DESCRIPTION: 析构server
AUTHOR：王鑫堂 
PARAMETERS: NULL
RETURN: 
*-------------------------------------------------------------------------*/

TCPServer::~TCPServer()
{
	::closesocket(mServerSocket);
	std::cout << "关闭TCP服务器成功!" << std::endl;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: RecvData
DESCRIPTION: 接收数据
AUTHOR：王鑫堂 
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
DESCRIPTION: 分析出数据段中包含的port chanel chanelStatus
AUTHOR：王鑫堂 
PARAMETERS: pszRecvData去除报文头后的数据段，格式：com1 chanel1 on , com1 chanel2 off ...
            iLen pszRecvData所指数据段长度
			port 串口号 1-16
			chanel 开关 1-4 & 1-16
			chanelStatus 状态 1：开， 2：关 
RETURN: 
*-------------------------------------------------------------------------*/
void TCPServer::AnalyseCmdPortChanelStatus(char *pszRecvData, 
										   const int & iLen, 
										   int & port, 
										   int & chanel, 
										   int & chanelStatus)
{
	//长度不为正值
	if(NULL == pszRecvData || iLen <= 0)
		return;
	//pszRecvData 格式为com1 chanel1 on, com1 chanel1 off
	std::string recvStr(pszRecvData, pszRecvData+iLen);
	std::string::size_type index = 0;
	std::string::size_type loc = 0;
	loc = recvStr.find(",", index);

	while(std::string::npos != loc)
	{
		//截取子串
		std::string strTemp(recvStr, index, loc-index);
		//解析出port chanel chanelStatus
		bool bRet = AnalyseCmdBuf(strTemp, port, chanel, chanelStatus);
		if(bRet)
		{
			index = loc;

			//用空格代替","，方便找出第二个","的位置
			recvStr.at(index) = ' ';
			loc = recvStr.find(",", index);
		}
		else
		{
			//输入的串格式错误
		}
		
	}

	//最后一个命令字后没有","
	if(std::string::npos == loc)
	{
		//最后一个命令串
		std::string strTemp(recvStr, index, recvStr.length()-index);
		bool bRet = AnalyseCmdBuf(strTemp, port, chanel, chanelStatus);
		if(bRet)
		{
			//字符串格式正确，解析成功
		}
		else
		{
			//输入的串格式错误
		}
	}

	return;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: DealData
DESCRIPTION: 处理接收数据
AUTHOR：王鑫堂 
PARAMETERS: pszRecvData接收字符串， iLen为pszRecvData数据长度， iOutLen未处理数据长度
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
	
	//判断头是否为0x33aa 不符合就直接丢弃
	if (strPackHead.wdPackHead != SX_HEAD_FUNID)
	{
		return false;
	}

	//解析出校验位，判断是否合格, 拿获得的校验位与自己计算获得的检验位比较

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
	//操作继电器输出命令
	if ((SX_RELAY_CMD == strPackHead.byHeadDataType) && (iLen >= ulTotalPackLenth))
	{
		if (iLen >= iHeadSize)
		{		
			Relay_Cmd_S relayCmd;
			memcpy((char *)(&relayCmd), pszRecvData+iHeadSize, strPackHead.wdTotalLen-iHeadSize);

			//std::cout << "had AnalyseCmdPortChanelStatus port chanel status:" << relayCmd.port << relayCmd.chanel << relayCmd.chanelStatus << std::endl;
			//解析出port chanel chanelStatus，然后将解析的结果放到全局cmd队列中

			m_ReceivedCmdCri.Lock();
			gqueue_ReceiveRelayCmd.push(relayCmd);
			m_ReceivedCmdCri.UnLock();
			//2、根据port chanel chanelStatus操作继电器
			//ControlDeviceChanel(port, chanel, chanelStatus);

			iLen -= ulTotalPackLenth;
		}
	}
	else if ((SX_RELAY_JSON == strPackHead.byHeadDataType) && (iLen >= ulTotalPackLenth))
	{
		//解析json串的信息
		//先获得接收到的字符串的内容
		if (iLen >= iHeadSize)
		{
			char *pJsonStr = new char[strPackHead.wdTotalLen-iHeadSize+1];
			memset(pJsonStr, '\0', strPackHead.wdTotalLen-iHeadSize+1);
			memcpy(pJsonStr, pszRecvData+iHeadSize, strPackHead.wdTotalLen-iHeadSize);

			//std::cout << "pJsonStr: " << pJsonStr << std::endl;

			Json::Reader reader;
			Json::Value root;
			Relay_Cmd_S relayCmd;
			// json格式："{\"port\": portValue,\"chanel\": chanelValue,\"chanelStatus\": chanelStatusValue}"
			if (reader.parse(pJsonStr, root))
			{
				relayCmd.port = root["port"].asInt();
				relayCmd.chanel = root["chanel"].asInt();
				relayCmd.chanelStatus = root["chanelStatus"].asInt();
			}

			m_ReceivedCmdCri.Lock();
			gqueue_ReceiveRelayCmd.push(relayCmd);
			m_ReceivedCmdCri.UnLock();
			//2、根据port chanel chanelStatus操作继电器
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
			//解析得到配置信息
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
			//解析得到配置信息
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
	else  //报文头都没有接收完全
	{
		if(iLen < iHeadSize)
		{
			iOutLen=iLen;
		}
		return false;
	}

	//半包的逻辑处理，三种情况
	//包头和数据都完整
	//包头完整，数据不完整
	//包头不完整
	if (iLen > iHeadSize)
	{
		memset(&strPackHead, 0, iHeadSize);
		memcpy(&strPackHead, pszRecvData+ulTotalPackLenth, iHeadSize);
		ULONG ulTempTotalPackLenth = strPackHead.wdTotalLen;
		
		//报文实际长度小于头中的长度标记位
		if (iLen < ulTempTotalPackLenth) //包头完整，数据不完整
		{
			iOutLen=iLen;
			return true;
		}

		return DealData(pszRecvData+ulTotalPackLenth, iLen, iOutLen, acceptSocket); //包头和数据都完整
	}
	else if(iLen <= iHeadSize) //小于包头
	{
		iOutLen = iLen;
		return true;
	}
}


/*--------------------------------------------------------------------------
FUNCTION NAME: Crc16
DESCRIPTION: 生成crc校验码
AUTHOR：王鑫堂 
PARAMETERS: pBuf：校验数据,nLen：校验数据的长度,函数返回校验码
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
DESCRIPTION: 启动server
AUTHOR：王鑫堂 
PARAMETERS: NULL
RETURN: VOID
*-------------------------------------------------------------------------*/
//void TCPServer::run()
//{
//	int nAcceptAddrLen = sizeof(mAcceptAddr);
//	while(true)
//	{
//		// 以阻塞方式,等待接收客户端连接
//		const int liBuffLen=71680;//50kb数据
//		char szBuf[liBuffLen] = {0};
//		int liOutLen=0;
//
//
//
//		mAcceptSocket = ::accept(mServerSocket, (struct sockaddr*)&mAcceptAddr, &nAcceptAddrLen);
//		std::cout << "接受客户端IP:" << inet_ntoa(mAcceptAddr.sin_addr) << std::endl;
//
//		while (true)
//		{
//			//int iRecvLen = ::recv(mAcceptSocket, szBuf, liBuffLen, 0);
//			int iRecvLen = RecvData(szBuf+liOutLen, liBuffLen-liOutLen);
//
//			//接收成功，正常处理
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
DESCRIPTION: 获取串口的设备类型vector
AUTHOR：王鑫堂 
PARAMETERS: NULL
RETURN: 各个串口对应的设备类型的vector
*-------------------------------------------------------------------------*/
std::vector<Serial2Device> TCPServer::getVecSerialnum2DeviceType()
{
	return this->vecSerialnum2DeviceType;
}


/*--------------------------------------------------------------------------
FUNCTION NAME: WriteXMLInfor
DESCRIPTION: 将串口与设备的对应信息写到xml文件或者修改原有的xml信息
AUTHOR：王鑫堂 
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
		//没有找到节点
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
	//std::string lsConfigXmlInfo = lTiXmlPrinter.CStr(); //修改以后，准备发给设备的xml信息

	//std::cout << "lsConfigXmlInfo = " << lsConfigXmlInfo << std::endl;
	loXmlDoc->SaveFile();

	return true;
}

/*--------------------------------------------------------------------------
FUNCTION NAME: ReadXMLInfor
DESCRIPTION: 读取串口的设备类型配置xml文件，将结果保存到vecSerialnum2DeviceType
AUTHOR：王鑫堂 
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
		//CLog::GetInstance()->AddLog("Guard_Process>::读取xml配置参数失败!");
		return false;
	}

	//获得根元素
	TiXmlElement *RootElement = myDocument->RootElement();
	TiXmlElement *lpServerCount = RootElement->FirstChildElement("SerialCount");
	TiXmlElement *lpServerInfor = lpServerCount->FirstChildElement("CSGSDK.dll");//查找ServerCount对应的字段 
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

