#ifndef __CRISECTION_H
#define __CRISECTION_H

#include <Windows.h>

class CCriticalSection
{

private:
	CRITICAL_SECTION m_oSection;
public:
	CCriticalSection()
	{

		InitializeCriticalSection(&m_oSection);

	};
	~CCriticalSection()
	{

		DeleteCriticalSection(&m_oSection);

	}
	__inline void  Lock()
	{

		EnterCriticalSection(&m_oSection);

	}
	__inline void UnLock()
	{

		LeaveCriticalSection(&m_oSection);
	};
};


class CCriticalAutoLock
{
public:
	CCriticalAutoLock(CCriticalSection& aCriticalSection)
	{
		m_pCriticalSection = &aCriticalSection;
		m_pCriticalSection->Lock();
	};	
	~CCriticalAutoLock(){
		m_pCriticalSection->UnLock();
	}
private:
	CCriticalSection *m_pCriticalSection;
};

#endif // __CRISECTION_H
