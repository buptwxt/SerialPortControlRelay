//#include "Relay.h"
#include "stdafx.h"
#include "Server.h"
#include "CommData.h"
#include <algorithm>
#include <conio.h>


extern std::vector<Serial2Device> gvecSerialNum2DeviceType;
extern std::queue<Relay_Cmd_S> gqueue_ReceiveRelayCmd;

int main(int atgc, char *argv[])
{
	TCPServer *pServer = TCPServer::GetTCPServerInstance();
	CDebugLog::GetInstance()->InitWriteLog();

	//���������ȡ����XML�ļ�
	bool ret = pServer->ReadXMLInfor();
	if (!ret)
	{
		return -1;
	}
	//��ȡXML�ļ���Ϣ
	
	gvecSerialNum2DeviceType = pServer->getVecSerialnum2DeviceType();
	/*Serial2Device serial2Device = {"0101", 1, 1, 1, "serial_01_chanel_01"};
	ret = pServer->WriteXMLInfor(serial2Device);
	if (!ret)
	{
		return -1;
	}*/

	
 	//pServer->run();
	//std::cout << "server is running" << std::endl;

	//����������
	ret = pServer->Start();
	if (!ret)
	{
		return -1;
	}

	//�ȴ��û�����,
	while(!_kbhit())
	{
		std::string buf;	
		getline(std::cin, buf);

		DebugLog(buf.c_str());
		//std::cout << "input " << buf << std::endl;
		//DebugLog("cmd input: %s\n", buf->c_str());
		
		if (!buf.compare("stop"))
		{
			break;
		}
		
		int port, chanel, chanelStatus;
		bool bRet = AnalyseCmdBuf(buf.c_str(), port, chanel, chanelStatus);

		//std::cout << "after compare " << buf << std::endl;
		if (bRet)
		{
			//��������port chanel chanelStatus��һ���ṹ��洢������
			//ControlDeviceChanel(port, chanel, chanelStatus);
			Relay_Cmd_S relayCmd;
			relayCmd.port = port;
			relayCmd.chanel = chanel;
			relayCmd.chanelStatus = chanelStatus;
			//�����������ȫ�ֶ���
			pServer->m_ReceivedCmdCri.Lock();
			gqueue_ReceiveRelayCmd.push(relayCmd);
			pServer->m_ReceivedCmdCri.UnLock();
		}

		Sleep(100);
	}

	return 0;
}