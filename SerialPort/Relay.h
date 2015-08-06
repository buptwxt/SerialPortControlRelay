#ifndef RELAY_H
#define RELAY_H

#include <map>
#include <iostream>
#include <vector>


//4·�̵���
#define SET_BOARD_01_CHANEL(chanel, switchStatus) {0x3a, 0x88, 0x01, chanel, switchStatus, 0x0d, 0x0a}
#define GET_BOARD_01_CHANEL(chanel) {0x3a, 0x99, 0x01, chanel, 0x0d, 0x0a}
#define GET_BOARD_01_INPUT(chanel)  {0x3a, 0x98, 0x01, chanel, 0x0d, 0x0a}

void ControlDeviceChanel(int relayIndex=1, int chanel=1, int switchStatus=1);

class CSerialPort;

class Relay
{
public:
	Relay();
	//port 1��ʾ����1 baud��ʾ������ databits����λ stopbitsֹͣλ
	int InitRelay(int port=1, int baud=9600, int databits=8, int stopbits=1);
	//chanel 0��ʾ���п��� 1��ʾ����1 2��ʾ����2 3��ʾ����3 4��ʾ����4  switchStatus 1��ʾ�պ� 0��ʾ����
	void SetBoardChanel(int chanel=0, int switchStatus=1);
	bool IsOpen(int relayIndex);
	void GetBoradOutput(int nChanel=0);
	void GetBoradInput(int chanel=0);


private:
	CSerialPort *_serialPort;
};

#endif




