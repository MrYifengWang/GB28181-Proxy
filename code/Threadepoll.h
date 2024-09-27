#ifndef Threadepoll_h__
#define Threadepoll_h__

#include "common/epoll.h"
#include "HDAgent.h"
#include "common/ulu_lib.h"
#include <eXosip2/eXosip.h>

#define  MAXEPOLLSIZE 5000

class CThreadepoll: public ulu_thread {
public:

	CThreadepoll(int identity);
	virtual ~CThreadepoll(void);

public:
	static void startThread(int identity);

protected:
	virtual bool onStart();
	virtual void run();
	void initOsip();
private:
	void checkTimeout();
	void handleMessage(string& message);
	void handleSipResponse(char* buf, int len);
	void handleJsonResponse(char* buf, int len);

public:
//	void cleanResponse();
	Agent* findUser(int uname);
	bool delUser(int uname);
	bool delStreamUser(string sipid, int tm, string token);
	void responseJson();
	void checkOldStreamServer(string ip, int tm);
	int m_totalsock;

private:
	vector<Agent*> m_agents;
	ulu_socket_msg_old m_tListener;
	int m_tSocket;
	int m_epoll;
	int m_identity;
//	struct eXosip_t *m_eCtx;
	int m_lastchecktime;
	struct epoll_event m_events[MAXEPOLLSIZE];

};
#endif
