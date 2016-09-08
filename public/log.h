#ifndef BASIC_LOG_H
#define BASIC_LOG_H

#define	LOG_BY_DAY		0x00010000		//!< 按天记录。如果超过限制的大小，一天可能会有多个文件。
#define	LOG_BY_SIZE		0x00020000		//!< 按大小记录。超过限制的大小，新建一个文件。
#define	LOG_BY_ONEFILE	0x00040000		//!< 始终只有一个文件，如果按天，则删除前一天的数据，超过大小限制，删除原来的文件。
#define	LOG_BY_BUFFER	0x00080000		//!< 日志不立刻写盘，先保存在缓存区里面，定时写盘。提高效率。
#define	LOG_BY_OPEN		0x01000000		//!< 日志文件保持打开状态，不关闭。
#define	LOG_BY_NOLIMIT	0x02000000		//!< 不限制文件大小
#define LOG_BY_SAMENAME	0x04000000		//!< 坚决不换名字


//
#define LOG_ADD_TIME	0x00100000		//!< 记录时间
#define LOG_ADD_THREAD	0x00200000		//!< 记录线程ID
////
#define LOG_SIZE_LIMIT	0x0000ffff		//!< 日志文件大小限制	单位：MB
#define LOG_NAME_DAY_S	"S%DAY%"
//
//
#define LOG_ERROR_NAME_EMPTY		-1	//!< 文件名为空
#define LOG_ERROR_OPEN_FILE			-2	//!< 打开文件失败
#define LOG_ERROR_FULL				-3	//!< 日志记录通道已经满了。

void BasicLogEventV(const char* pszLog, ...);
void BasicLogEventV(long lLogChannel, const char* pszLog, ...);
void BasicLogEvent(const char* pszLog);
void BasicLogEvent(long lLogChannel, const char* pszLog);

long BasicSetDefaultLogEventMode(long nOption, const char* pszLogFile);
long BasicSetLogEventMode(long nOption, const char* pszLogFile);
long BasicCloseLogEvent(long lLogChannel);


#endif
