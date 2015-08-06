#ifndef __DEBUG_LOG_H_
#define __DEBUG_LOG_H_


#include <iostream>  //string 头文件
#include <string>
#include <atltime.h> //CTIME 头文件

using namespace std;

//下面两个DebugLog都是用来调试的
#define DebugLog(DebugMessag) \
	CDebugLog::writeRunLog(DebugMessag)	

//#ifdef _DEBUG
//#define DebugLog(fmt, ...)\
//{\
//	char __debug_msg_buf__[1024];\
//	sprintf_s(__debug_msg_buf__, fmt, __VA_ARGS__);\
//	OutputDebugString(__debug_msg_buf__);\
//}
//#else
//#define DebugLog(fmt, ...)
//#endif // _DEBUG

//每隔4小时产生一个新的log
#define MAX_LOG_SPACE_TIME  4*3600*1000

class CDebugLog
{
public:

	CDebugLog(void);

	~CDebugLog(void);

	//初始化log
	void InitWriteLog();    // exe目录下创建filename文件

	//打印log
	static void writeRunLog(const char* str);  // 像filename中写log

	//设置log文件名字
	void SetLogName();
    
	//获得系统时间
	INT64 GetSystemTime();

	//处理创建log间隔
	static void CreateLogTimeSpace(CDebugLog *pObjDebugLog);

	//获得对象指针
	static CDebugLog * GetInstance()
	{
		static CDebugLog instance;
		return &instance;
	}

protected:

	static std::string  m_szLogFileName;//LogName
	char m_szLogName[255];
	INT64   m_n64lastTime;  //更新最后访问时间

};
#endif//__DEBUG_LOG_H_
