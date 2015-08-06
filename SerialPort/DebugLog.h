#ifndef __DEBUG_LOG_H_
#define __DEBUG_LOG_H_


#include <iostream>  //string ͷ�ļ�
#include <string>
#include <atltime.h> //CTIME ͷ�ļ�

using namespace std;

//��������DebugLog�����������Ե�
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

//ÿ��4Сʱ����һ���µ�log
#define MAX_LOG_SPACE_TIME  4*3600*1000

class CDebugLog
{
public:

	CDebugLog(void);

	~CDebugLog(void);

	//��ʼ��log
	void InitWriteLog();    // exeĿ¼�´���filename�ļ�

	//��ӡlog
	static void writeRunLog(const char* str);  // ��filename��дlog

	//����log�ļ�����
	void SetLogName();
    
	//���ϵͳʱ��
	INT64 GetSystemTime();

	//������log���
	static void CreateLogTimeSpace(CDebugLog *pObjDebugLog);

	//��ö���ָ��
	static CDebugLog * GetInstance()
	{
		static CDebugLog instance;
		return &instance;
	}

protected:

	static std::string  m_szLogFileName;//LogName
	char m_szLogName[255];
	INT64   m_n64lastTime;  //����������ʱ��

};
#endif//__DEBUG_LOG_H_
