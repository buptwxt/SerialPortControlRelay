#ifndef RELAY_H
#define RELAY_H

#include <map>
#include <iostream>
#include <vector>


//4路继电器
#define SET_BOARD_01_CHANEL(chanel, switchStatus) {0x3a, 0x88, 0x01, chanel, switchStatus, 0x0d, 0x0a}
#define GET_BOARD_01_CHANEL(chanel) {0x3a, 0x99, 0x01, chanel, 0x0d, 0x0a}
#define GET_BOARD_01_INPUT(chanel)  {0x3a, 0x98, 0x01, chanel, 0x0d, 0x0a}

void ControlDeviceChanel(int relayIndex=1, int chanel=1, int switchStatus=1);

class CSerialPort;

class Relay
{
public:
	Relay();
	//port 1表示串口1 baud表示波特率 databits数据位 stopbits停止位
	int InitRelay(int port=1, int baud=9600, int databits=8, int stopbits=1);
	//chanel 0表示所有开关 1表示开关1 2表示开关2 3表示开关3 4表示开关4  switchStatus 1表示闭合 0表示开启
	void SetBoardChanel(int chanel=0, int switchStatus=1);
	bool IsOpen(int relayIndex);
	void GetBoradOutput(int nChanel=0);
	void GetBoradInput(int chanel=0);


private:
	CSerialPort *_serialPort;
};

#endif




