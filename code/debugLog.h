#ifndef __DEBUG__FACE__H__
#define __DEBUG__FACE__H__

#define DEBUG_COMMN
//#define DEBUG_CONSOLE
#define DEBUG_FILE
//#define DEBUG_TELNET
#define FORMAT "%-20s | %-20s | %-5d | "
#define FORMAT_MACRO __FILE__,__FUNCTION__,__LINE__
#define FORMAT_INFO file,function,lineNumber

#define MAX_DEBUG_BUF_LENGTH (1024)

#ifdef DEBUG_FILE
#define FILE_NAME "./debugFile.txt"
#endif

#include <string>
#include "common/ulu_lib.h"
using namespace std;
#include "App.h"

enum E_DEBUG_LEVER {
	VERBOSE = 1, DEBUG, INFO, WARN, ERROR, NOFORMT,
};
class telnetServer;
class debugFace {
private:
	debugFace();
	~debugFace();
public:
	static debugFace* GetInstancePtr();
	char * getHumanTime();
	void debug(E_DEBUG_LEVER level, const char *pszFormat, ...);
	void debug1(E_DEBUG_LEVER level, char *pszFormat, ...);
	void writefile(char* buf, int len);
	void writefile(string);

//	void debug(E_DEBUG_LEVER level,char *pszFormat,va_list pArgList);
private:
	mutable ulu_mutex lock_log;
	string m_tRoot;

private:
#if defined(DEBUG_FILE)
	int m_fileFd;
#elif defined(DEBUG_TELNET)
	telnetServer *m_pTelnetServer;
#endif
	char *m_debugBuf;
	static debugFace *m_pStSelf;
};

//	pDebug->debug(level,"| %s | %-20s | %-20s | %-5d | ",pDebug->getHumanTime(),__FILE__,__FUNCTION__,__LINE__);
//pDebug->debug1(level,"| %s |",pDebug->getHumanTime());
#if 1
#define Debug(level,pszFormat,...) \
do{ \
	debugFace *pDebug = debugFace::GetInstancePtr();\
	if(level != NOFORMT)\
	{ \
	pDebug->debug1(level,pszFormat,##__VA_ARGS__);\
	} \
	else \
	{ \
	    pDebug->debug1(level,pszFormat,##__VA_ARGS__);\
	}\
}while(0)
#else
#define Debug(level,pszFormat,...)\
do{ \
	debugFace *pDebug = debugFace::GetInstancePtr();\
	if(level != NOFORMT)\
	{ \
    fprintf(stderr,"| %s | %-20s | %-20s | %-4d | ",pDebug->getHumanTime(),__FILE__,__FUNCTION__,__LINE__);\
    fprintf(stderr,pszFormat,##__VA_ARGS__);\
	}\
	else \
	{ \
		fprintf(stderr,"| %s | ",pDebug->getHumanTime());\
	    fprintf(stderr,pszFormat,##__VA_ARGS__);\
	}\
}while(0)

#endif

#endif
