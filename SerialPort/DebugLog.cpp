#include "DebugLog.h"
#include <sys/timeb.h>
#include <sys/types.h>

//初始化log
string  CDebugLog::m_szLogFileName = "";//LogName

CDebugLog::CDebugLog()
{    
	m_n64lastTime = 0;
	m_n64lastTime = GetSystemTime();
}

CDebugLog::~CDebugLog()
{

}

/*--------------------------------------------------------------------------
FUNCTION NAME: InitWriteLog(const char* fileName) 
DESCRIPTION:  Log初始化

PARAMETERS:  

*-------------------------------------------------------------------------*/
void CDebugLog::InitWriteLog() 
{   
	SetLogName();
	char szpath[1000];
	int filelen = GetModuleFileName(NULL,szpath,1000);
	int i = filelen;
	while(szpath[i]!='\\')
	{
		i--;
	}
	szpath[i + 1] = '\0';
	FILE * procLogFp;
	std::string Path = szpath;
	Path += m_szLogName;
	procLogFp = fopen(Path.c_str(),"w");
	fclose(procLogFp);
	m_szLogFileName = Path;

}

/*--------------------------------------------------------------------------
FUNCTION NAME: writeRunLog(const char* str)
DESCRIPTION:  //打印log

PARAMETERS:  

*-------------------------------------------------------------------------*/
void CDebugLog::writeRunLog(const char* str)
{
	//  测试代码
	if (NULL == str)
	{
		return;
	}
	
	//创建新的log，时间判断
    CreateLogTimeSpace(CDebugLog::GetInstance());
	
	
	FILE * procLogFp = NULL;
	//测试
	procLogFp=fopen(m_szLogFileName.c_str(),"a");
	if (NULL == procLogFp)
	{
		return;
	}
	CTime now=CTime::GetCurrentTime(); // 获取系统当前时间
	//根据设备地址和端口号设备名，端口名
	std::string strTime = now.Format("%Y年%m月%d日 %H:%M:%S  "); 
	std::string buf ;
	buf += strTime;
	buf +=" ";
	buf += str ;
	buf += +"\r\n";
	fputs(buf.c_str(),procLogFp);
	//m_n64lastTime = GetSystemTime();
	fclose(procLogFp);

}

/*--------------------------------------------------------------------------
FUNCTION NAME:void CDebugLog::SetLogName()
DESCRIPTION:  设置log文件名字

PARAMETERS:  

*-------------------------------------------------------------------------*/
void CDebugLog::SetLogName()
{
	char szTemp[255] = {0};
	struct tm *tm;
	time_t timer;
	time(&timer);
	tm = localtime(&timer);
	sprintf(szTemp,"SerialPort[%d年-%d月-%d日%.2d时%.2d分%.2d秒].log",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,\
		tm->tm_hour,tm->tm_min,tm->tm_sec);
	strcpy(m_szLogName,szTemp);
}

/*--------------------------------------------------------------------------
FUNCTION NAME:INT64 CClientItem::GetSystemTime()
DESCRIPTION:  获得系统时间 
PARAMETERS:  
RETURN:       
*-------------------------------------------------------------------------*/
INT64 CDebugLog::GetSystemTime()
{
	struct timeb loTimeb;
	//memset(&loTimeb, 0 , sizeof(timeb));
	ftime(&loTimeb);
	return ((INT64)loTimeb.time * 1000) + loTimeb.millitm;
}

/*--------------------------------------------------------------------------
FUNCTION NAME:
DESCRIPTION:  处理创建log间隔
PARAMETERS:  
RETURN:       
*-------------------------------------------------------------------------*/
void CDebugLog::CreateLogTimeSpace(CDebugLog *pObjDebugLog)
{   
	INT64 n64LogTimeSpace = (pObjDebugLog->GetSystemTime()) - (pObjDebugLog->m_n64lastTime);
	if ( n64LogTimeSpace > MAX_LOG_SPACE_TIME)
	{
		pObjDebugLog->m_n64lastTime = pObjDebugLog->GetSystemTime();
		pObjDebugLog->InitWriteLog();
	}
}