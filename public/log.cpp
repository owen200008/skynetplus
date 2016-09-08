#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include "public_threadlock.h"

#ifdef USE_3RRD_MALLOC
extern void* s_3rd_malloc_res(size_t size);
extern void* s_3rd_free_res(void* ptr);
#else
#define s_3rd_malloc_res malloc
#define s_3rd_free_res free
#endif

static int g_nLogLimit = (10*1024*1024);		//日志文件大小限制
class WriteLogDataBuffer
{
public:
	WriteLogDataBuffer()
	{
		InitMember();
	}
	void InitMember()
	{
		memset(this, 0, sizeof(*this));
	}

	void InitLogData(const char* pText)
	{
		m_pText = pText;
	}
	void CopyLogData(WriteLogDataBuffer& logData)
	{
		if (logData.m_pText != NULL)
		{
			int nLen = strlen(logData.m_pText)
			m_pText = s_3rd_malloc_res(m_lDataLen);
			strcpy(m_pText, logData.m_pText);
		}
	}
	void ClearLogData()
	{
		if(m_pText)
		{
			s_3rd_free_res(m_pText);
		}
		InitMember();
	}
public:
	char*			m_pText;
};

static long ReplaceFileName(char* szFileName, const char* lpszKey, const char* lpszToday)
{
	int nNameLen = strlen(szFileName);
	int nDayLen = strlen(lpszToday);
	char* pKeyPos = strstr(szFileName, lpszKey);
	char* pKeyEnd = NULL;
	int nLeft = 0;
	if (pKeyPos != NULL)
	{
		pKeyEnd = pKeyPos + strlen(lpszKey);
	}
	else
	{
		pKeyPos = strrchr(szFileName, '.');
		if (pKeyPos == NULL)
		{
			pKeyPos = &szFileName[nNameLen];
			*pKeyPos = '.';
			pKeyPos++;
			pKeyEnd = pKeyPos;
		}
		else
		{
			pKeyEnd = pKeyPos;
		}
	}
	nLeft = nNameLen - (pKeyEnd - szFileName);
	if (nLeft > 0)
	{
		char* pNewPos = pKeyPos + nDayLen;
		memmove(pNewPos, pKeyEnd, nLeft * sizeof(char));
	}
	strncpy(pKeyPos, lpszToday, nDayLen);
	return (pKeyPos - szFileName);
}

static void GetTodayString(char* szToday)
{
	CTime cur = CTime::GetCurrentTime();
	sprintf(szToday, "%d%02d%02d_000", cur.GetYear(), cur.GetMonth(), cur.GetDay());
}

#define LOG_DATA_LEN		12			//文件里面日期的长度
class CBasicLogChannel
{
public:
	CBasicLogChannel()
	{
		m_nLogOption = 0;
		m_nSizeLimit = 0;
		m_fLog = NULL;
		m_lReplacePos = 0;
		memset(m_szLogFileName, 0, sizeof(m_szLogFileName));
		memset(m_szToday, 0, sizeof(m_szToday));
	}
	long InitLogChannel(long nOption, const char* lpszLogFile)
	{
		_SetLogChannel(nOption, lpszLogFile);
		return OpenLogFile();
	}
	long ChangeLogChannel(long nOption, LPCTSTR lpszLogFile)
	{
		CSpinLockFunc lock(&m_spinLock);
		lock.LockAndSleep();
		CloseLogFile();
		_SetLogChannel(nOption, lpszLogFile);
		return OpenLogFile();
	}
	void CheckChannel(const char* lpszToday)
	{
		CSpinLockFunc lock(&m_spinLock);
		lock.LockAndSleep();
		if (m_nLogOption & LOG_BY_DAY)
		{
			if (strncmp(m_szToday, lpszToday, 8) != 0)		//日期不一致，需要更新文件名
			{
				CloseLogFile();
				if (m_nLogOption & LOG_BY_ONEFILE)
				{
					DeleteFile(m_szLogFileName);
				}
				if (!(m_nLogOption & LOG_BY_SAMENAME))
				{
					m_lReplacePos = ReplaceFileName(m_szLogFileName, m_szToday, lpszToday);
				}
				strcpy(m_szToday, lpszToday);
				OpenLogFile();
			}
		}
	}
protected:
	long	m_nLogOption;					//日志的属性，见定义 LOG_*
	long	m_nSizeLimit;					//日志文件的大小限制。0表示不限制
	FILE*	m_fLog;							//日志文件句柄
	char	m_szLogFileName[MAX_PATH];
	char	m_szToday[16];					//当天的日期和序号。格式：YYMMDD_XXX  如 20110720_000
	long	m_lReplacePos;					//替换日期或者序号的起始位置
	static long	m_lLastFileNo;				//对于当天多个文件自动生成的文件编号。
	SpinLock m_spinLock;
	std::vector<WriteLogDataBuffer>	m_vtLogData;
};
long	CBasicLogChannel::m_lLastFileNo = 0;		//对于当天多个文件自动生成的文件编号。

