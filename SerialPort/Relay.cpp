#include "SerialPort.h"
#include "Relay.h"
#include <cctype>
#include "Common.h"
//#include "CommData.h"
#include <assert.h>


//16·�̵��� ��
char	SET_CHANEL_01_OPEN[] =  {0x01, 0x05, 0x00, 0x00, 0xFF, 0x00, 0x8C, 0x3A};     //1 ·��
char	SET_CHANEL_02_OPEN[] =  {0x01, 0x05, 0x00, 0x01, 0xFF, 0x00, 0xDD, 0xFA};     //2
char	SET_CHANEL_03_OPEN[] =  {0x01, 0x05, 0x00, 0x02, 0xFF, 0x00, 0x2D, 0xFA};     //3
char	SET_CHANEL_04_OPEN[] =  {0x01, 0x05, 0x00, 0x03, 0xFF, 0x00, 0x7C, 0x3A};     //4
char	SET_CHANEL_05_OPEN[] =  {0x01, 0x05, 0x00, 0x04, 0xFF, 0x00, 0xCD, 0xFB};     //5
char	SET_CHANEL_06_OPEN[] =  {0x01, 0x05, 0x00, 0x05, 0xFF, 0x00, 0x9C, 0x3B};     //6
char	SET_CHANEL_07_OPEN[] =  {0x01, 0x05, 0x00, 0x06, 0xFF, 0x00, 0x6C, 0x3B};     //7
char	SET_CHANEL_08_OPEN[] =  {0x01, 0x05, 0x00, 0x07, 0xFF, 0x00, 0x3D, 0xFB};     //8
char	SET_CHANEL_09_OPEN[] =  {0x01, 0x05, 0x00, 0x08, 0xFF, 0x00, 0x0D, 0xF8};     //9 
char	SET_CHANEL_10_OPEN[] =  {0x01, 0x05, 0x00, 0x09, 0xFF, 0x00, 0x5C, 0x38};     //10 
char	SET_CHANEL_11_OPEN[] =  {0x01, 0x05, 0x00, 0x0A, 0xFF, 0x00, 0xAC, 0x38};     //11
char	SET_CHANEL_12_OPEN[] =  {0x01, 0x05, 0x00, 0x0B, 0xFF, 0x00, 0xFD, 0xF8};     //12 
char	SET_CHANEL_13_OPEN[] =  {0x01, 0x05, 0x00, 0x0C, 0xFF, 0x00, 0x4C, 0x39};     //13
char	SET_CHANEL_14_OPEN[] =  {0x01, 0x05, 0x00, 0x0D, 0xFF, 0x00, 0x1D, 0xF9};     //14
char	SET_CHANEL_15_OPEN[] =  {0x01, 0x05, 0x00, 0x0E, 0xFF, 0x00, 0xED, 0xF9};     //15
char	SET_CHANEL_16_OPEN[] =  {0x01, 0x05, 0x00, 0x0F, 0xFF, 0x00, 0xBC, 0x39};     //16  ·�� 

//16·�̵��� ��
char	SET_CHANEL_01_CLOSE[] = {0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xCA};     //1 ·��
char	SET_CHANEL_02_CLOSE[] = {0x01, 0x05, 0x00, 0x01, 0x00, 0x00, 0x9C, 0x0A};     //2
char	SET_CHANEL_03_CLOSE[] = {0x01, 0x05, 0x00, 0x02, 0x00, 0x00, 0x6C, 0x0A};     //3
char	SET_CHANEL_04_CLOSE[] = {0x01, 0x05, 0x00, 0x03, 0x00, 0x00, 0x3D, 0xCA};     //4
char	SET_CHANEL_05_CLOSE[] = {0x01, 0x05, 0x00, 0x04, 0x00, 0x00, 0x8C, 0x0B};     //5
char	SET_CHANEL_06_CLOSE[] = {0x01, 0x05, 0x00, 0x05, 0x00, 0x00, 0xDD, 0xCB};     //6
char	SET_CHANEL_07_CLOSE[] = {0x01, 0x05, 0x00, 0x06, 0x00, 0x00, 0x2D, 0xCB};     //7
char	SET_CHANEL_08_CLOSE[] = {0x01, 0x05, 0x00, 0x07, 0x00, 0x00, 0x7C, 0x0B};     //8
char	SET_CHANEL_09_CLOSE[] = {0x01, 0x05, 0x00, 0x08, 0x00, 0x00, 0x4C, 0x08};     //9 
char	SET_CHANEL_10_CLOSE[] = {0x01, 0x05, 0x00, 0x09, 0x00, 0x00, 0x1D, 0xC8};     //10 
char	SET_CHANEL_11_CLOSE[] = {0x01, 0x05, 0x00, 0x0A, 0x00, 0x00, 0xED, 0xC8};     //11
char	SET_CHANEL_12_CLOSE[] = {0x01, 0x05, 0x00, 0x0B, 0x00, 0x00, 0xBC, 0x08};     //12 
char	SET_CHANEL_13_CLOSE[] = {0x01, 0x05, 0x00, 0x0C, 0x00, 0x00, 0x0D, 0xC9};     //13
char	SET_CHANEL_14_CLOSE[] = {0x01, 0x05, 0x00, 0x0D, 0x00, 0x00, 0x5C, 0x09};     //14
char	SET_CHANEL_15_CLOSE[] = {0x01, 0x05, 0x00, 0x0E, 0x00, 0x00, 0xAC, 0x09};     //15
char	SET_CHANEL_16_CLOSE[] = {0x01, 0x05, 0x00, 0x0F, 0x00, 0x00, 0xFD, 0xC9};     //16  ·�� 




std::map<int, Relay*>g_mapPortRelay;
extern std::vector<Serial2Device> gvecSerialNum2DeviceType;

#define connect(x,y,z) x##y##z

Relay::Relay()
{
	_serialPort = NULL;

}


int Relay::InitRelay(int port, int baud, int databits, int stopbits)
{
    //��ѯ�Ƿ񴴽��˴��ڶ���û���򴴽�һ����������map�У�����ֱ��ȡ���ڶ���
	std::map<int, Relay*>::iterator iter = g_mapPortRelay.find(port);
	if (iter == g_mapPortRelay.end())
	{
		CSerialPort *serialPort = new CSerialPort();
		if( (*serialPort).InitPort(NULL,port,baud,'N',8,0,EV_RXCHAR, 512))
		{	
			//�����߳�
			(*serialPort).StartMonitoring();
			_serialPort = serialPort;
		}
		else
		{
			printf("���ڴ�ʧ�ܣ����ڴ��ڲ����ڻ��߱�ռ�õ���\n");
			return -1;
		}

		//std::pair<int, CSerialPort*>serialPortItem(port, serialPort);
		std::pair<int, Relay*>serialRelay(port, this);
		g_mapPortRelay.insert(serialRelay);
	}
	//else
	//{
	//	_serialPort = iter->second;
	//}

	return 0;
}


bool isChanelNum(int chanel)
{
	if (chanel > 0 && chanel < 16)
	{
		return true;
	} 
	else
	{
		return false;
	}
}

int Dec2Hex(int chanel)
{ 
	assert(chanel >= 0 && chanel <= 16);

	if((chanel>=0)&&(chanel<=15))
		return chanel;
	else if (chanel == 16)
		return 0x10;
}



void ControlDeviceChanel(int relayIndex, int chanel, int switchStatus)
{
	//std::cout << "in ControlDeviceChanel: port chanel status:" << relayIndex << chanel << switchStatus << std::endl; 
	//��ȡ���ڶ��� 
	std::map<int, Relay*>::iterator iter = g_mapPortRelay.find(relayIndex);
	if (iter != g_mapPortRelay.end())
	{
		Relay *pRelay = iter->second;
		pRelay->SetBoardChanel(chanel, switchStatus);
	}
	else//û�д����̵��������򴴽�һ���̵�������
	{
		Relay *pRelay = new Relay;
		int ret = pRelay->InitRelay(relayIndex);
		if(ret == -1)
		{
			return;
		}

		std::pair<int, Relay*>serialRelay(relayIndex, pRelay);
		g_mapPortRelay.insert(serialRelay);
		pRelay->SetBoardChanel(chanel, switchStatus);
	}
}


bool Relay::IsOpen(int relayIndex)
{

	std::map<int, Relay*>::iterator iter = g_mapPortRelay.find(relayIndex);
	if (iter != g_mapPortRelay.end())
	{
		return true;
	}
	return false;
}


//���������̵�����ͬһ�ӿ� Chanel����ͨ���� switchStatus������״̬
void Relay::SetBoardChanel(int chanel, int switchStatus)
{
	if(!isChanelNum(chanel))
		return;

	int deviceFlag = 1;
	//���ݶ����ҵ����ںţ����ݴ��ں��ҵ����õ��豸���ͣ������豸���͵��ò�ͬ�Ŀ�������
	//std::map<int, CSerialPort*>g_mapPortRelay; ���ں��봮�ڶ���map
	//extern std::vector<Serial2Device> gvecSerialNum2DeviceType; �豸���ͽṹ��
	std::map<int, Relay*>::iterator iter = g_mapPortRelay.begin();
	for (; iter != g_mapPortRelay.end(); iter++)
	{   //��map���ҵ����ڶ���ָ�� ��ôiter->first���Ǵ��ں�
		if (iter->second == this)
		{
			//iter->first
			std::vector<Serial2Device>::iterator iterSerial2Device = gvecSerialNum2DeviceType.begin();
			for ( ; iterSerial2Device != gvecSerialNum2DeviceType.end(); iterSerial2Device++)
			{
				if (iterSerial2Device->serialNum == iter->first)
				{
					deviceFlag = iterSerial2Device->deviceName;
					//printf("**********in SetBoardChanel deviceFlag: %d\n", deviceFlag);
					break;
				}
			}
		}
	}
	


	char _switchStatus = 0xAA;
	switch (switchStatus)
	{
	case 0:
		_switchStatus = 0x00;
		break;
	case 1:
		_switchStatus = 0xFF;
		break;
	}

	//�ж��豸����
	switch(deviceFlag)
	{	
		case 1: 
		{
			char arrayCmd[7] = SET_BOARD_01_CHANEL(chanel, _switchStatus);
			_serialPort->WriteToPort(arrayCmd, 7);
			//printf("**********in SetBoardChanel chanel: %d\n", arrayCmd[3]);
			//printf("**********in SetBoardChanel _switchStatus: %d\n", arrayCmd[4]);
			break;
		}
			
		//16·�̵��� 
		case 2:
			//������
			if (1 == switchStatus)
			{
				switch(chanel)
				{
				case 1:
					_serialPort->WriteToPort(SET_CHANEL_01_OPEN, 8);
					break;
				case 2:
					_serialPort->WriteToPort(SET_CHANEL_02_OPEN, 8);
					break;
				case 3:
					_serialPort->WriteToPort(SET_CHANEL_03_OPEN, 8);
					break;
				case 4:
					_serialPort->WriteToPort(SET_CHANEL_04_OPEN, 8);
					break;
				case 5:
					_serialPort->WriteToPort(SET_CHANEL_05_OPEN, 8);
					break;
				case 6:
					_serialPort->WriteToPort(SET_CHANEL_06_OPEN, 8);
					break;
				case 7:
					_serialPort->WriteToPort(SET_CHANEL_07_OPEN, 8);
					break;
				case 8:
					_serialPort->WriteToPort(SET_CHANEL_08_OPEN, 8);
					break;
				case 9:
					_serialPort->WriteToPort(SET_CHANEL_09_OPEN, 8);
					break;
				case 10:
					_serialPort->WriteToPort(SET_CHANEL_10_OPEN, 8);
					break;
				case 11:
					_serialPort->WriteToPort(SET_CHANEL_11_OPEN, 8);
					break;
				case 12:
					_serialPort->WriteToPort(SET_CHANEL_12_OPEN, 8);
					break;
				case 13:
					_serialPort->WriteToPort(SET_CHANEL_13_OPEN, 8);
					break;
				case 14:
					_serialPort->WriteToPort(SET_CHANEL_14_OPEN, 8);
					break;
				case 15:
					_serialPort->WriteToPort(SET_CHANEL_15_OPEN, 8);
					break;
				case 16:
					_serialPort->WriteToPort(SET_CHANEL_16_OPEN, 8);
					break;
				default:
					break;
				}	
			}
			else //�ر�����
			{
				switch(chanel)
				{
				case 1:
					_serialPort->WriteToPort(SET_CHANEL_01_CLOSE, 8);
					break;
				case 2:
					_serialPort->WriteToPort(SET_CHANEL_02_CLOSE, 8);
					break;
				case 3:
					_serialPort->WriteToPort(SET_CHANEL_03_CLOSE, 8);
					break;
				case 4:
					_serialPort->WriteToPort(SET_CHANEL_04_CLOSE, 8);
					break;
				case 5:
					_serialPort->WriteToPort(SET_CHANEL_05_CLOSE, 8);
					break;
				case 6:
					_serialPort->WriteToPort(SET_CHANEL_06_CLOSE, 8);
					break;
				case 7:
					_serialPort->WriteToPort(SET_CHANEL_07_CLOSE, 8);
					break;
				case 8:
					_serialPort->WriteToPort(SET_CHANEL_08_CLOSE, 8);
					break;
				case 9:
					_serialPort->WriteToPort(SET_CHANEL_09_CLOSE, 8);
					break;
				case 10:
					_serialPort->WriteToPort(SET_CHANEL_10_CLOSE, 8);
					break;
				case 11:
					_serialPort->WriteToPort(SET_CHANEL_11_CLOSE, 8);
					break;
				case 12:
					_serialPort->WriteToPort(SET_CHANEL_12_CLOSE, 8);
					break;
				case 13:
					_serialPort->WriteToPort(SET_CHANEL_13_CLOSE, 8);
					break;
				case 14:
					_serialPort->WriteToPort(SET_CHANEL_14_CLOSE, 8);
					break;
				case 15:
					_serialPort->WriteToPort(SET_CHANEL_15_CLOSE, 8);
					break;
				case 16:
					_serialPort->WriteToPort(SET_CHANEL_16_CLOSE, 8);
					break;
				default:
					break;
				}
			}	
		default:
			break;
	}
	
	//�̵����ķ�Ӧʱ�䣬����20�룬����������������̵���ִ�з�Ӧ������
	Sleep(20);
}

void Relay::GetBoradOutput(int chanel)
{
	if(!isChanelNum(chanel))
		return;


	char arrayCmd[6] = GET_BOARD_01_CHANEL(chanel);
	_serialPort->WriteToPort(arrayCmd, 6);
	Sleep(1000);
}


void Relay::GetBoradInput(int chanel)
{
	if(!isChanelNum(chanel))
			return;

	char arrayCmd[6] = GET_BOARD_01_INPUT(chanel);
	_serialPort->WriteToPort(arrayCmd, 6);
	Sleep(1000);
}