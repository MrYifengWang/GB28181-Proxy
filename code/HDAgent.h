#pragma once
#include "HDAgent.h"
#include "App.h"
#include <eXosip2/eXosip.h>
#include "common/tinyxml.h"

class CThreadepoll;
class Agent {
public:
	Agent(SOCKET handle1, int fd, void* pmgr, int agentid, char* peerip, int peerport);
	void handleEvent(unsigned int ievent);

public:
	virtual ~Agent(void);
	virtual void start(int identity = 0);
	virtual void handleCommand(string& command);
	void sendCommand(string& command);
	virtual void release();
	void close();
	void handleSipResponse(osip_message_t *);
	void handleJsonResponse(Json::Value& json);
	void handleSipResponseDevInfo(osip_message_t *, mxml_node_t* xml);
	void handleSipResponseDevInfo1(osip_message_t *, TiXmlHandle& xml);

	void handleSipResponseRecordInfo(osip_message_t *, mxml_node_t* xml);
	void handleSipResponseRecordInfo1(osip_message_t *, TiXmlHandle& xml);
	bool checkKeepalive(int curtime);

private:
	bool handleCommandLogin(string& command);
	bool handleCommandStreamLogin(string& command);
	bool handleCommandUpdateAlarmCfg(string& command);

	bool handleHungup(string& command);
	bool handleQuit(string& command);
	void sendResponse(Json::Value& json, bool log = true);

	void clearSessions();
private:
	bool readCommand();

public:
	int m_agentid;
	CThreadepoll* m_pparent;
	ulu_socket_msg_old m_cSocket;
	unsigned int m_deadtime;
	bool m_isdisconnect;
	int m_efd;
	string m_cur_clientid;
	string m_cur_nvrid;

	int tmpstarttime;
	int stoppos;

	int m_recordStarttime;
	int m_recordEndtime;

	Json::Value m_tmpRecordData;
	string m_peerip;
	int m_peerport;
	int m_agent_type;
	string m_sip_server_id;
	string m_token;
	int m_lastKeepalive;
	int m_stream_login_tm;
	bool m_isdead;

};
