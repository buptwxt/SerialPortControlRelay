#ifndef COMMON_H
#define COMMON_H


//串口号到设备号信息结构
typedef struct Serial2Device
{
	char id[20];
	int serialNum;
	int chanel;
	int deviceName; 
	char name[20];
}Serial2Device_S;

#endif