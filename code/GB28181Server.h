#ifndef GB28181SERVER_H
#define GB28181SERVER_H

#include <stdio.h>
#include <string>
#include <list>
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <eXosip2/eXosip.h>

#include "CameraDevice.h"
#include "App.h"
#include "common/mxml.h"
#include "common/ulu_lib.h"
#include "common/json.h"
#include "CallSession.h"
#include "MessageQueue.h"
#include "DB.h"
#include "RedisHandler.h"
#include "common/tinyxml.h"

#define APP_LOG printf
#define QUEUE_SIZE 3
#define ONLINE_EXP_TIME 240
#define REG_TIMEOUT_CHECK 5
typedef enum CALLTYPE {
	TYPE_LIVE = 1, TYPE_PLAYBACK = 2, TYPE_DOWNLOAD = 3,
};

class GB28181Server: public ulu_thread {
public:
	GB28181Server();
	virtual ~GB28181Server();
public:
	static void startThread(GB28181Server** pserver);
	virtual bool onStart();
	void stop();

protected:
	virtual void run();
public:
	void getQueueSize(vector<int> list);
private:
	void onDevStatChange(string sn, int type, int stat);
	//master
	void resetDev(string did);
	void resetAllSession(string did);
	void handleMessage(eXosip_event_t *je);
	void handleNotify(eXosip_event_t *je);
	void handleNotifyAck(eXosip_event_t *je);
	void handleResponse(eXosip_event_t *je);
	void handleMessageSendFail(eXosip_event_t *je);
	void sendInterUDPMessage(osip_message_t *);
	void sendInterJsonMessage(Json::Value& json, char* agentid);
	void sendInterJsonMessage(Json::Value& json, int agentid);
	void sendInterJson(int agentid, string jsonstr);
	void handleKeepAlive(eXosip_event_t *je, mxml_node_t* xml);
	void handleKeepAlive1(eXosip_event_t *je, TiXmlHandle& handle);
	void onChannelStatChange(vector<channelStat>& list);

	void dispatchMessage(Message* msg);
	void handleInterMessage(osip_message_t *);

	void onInterCall(osip_message_t * msg, Json::Value& data);
	void onInterClosePlay(osip_message_t * msg, Json::Value& data);
	void onInterGetDevInfo(osip_message_t * msg, Json::Value& data);
	void onInterRemoteQuery(osip_message_t * msg, Json::Value& data);
	void onInterGetRecordInfo(osip_message_t * msg, Json::Value& data);
	void onInterPtzCtrl(osip_message_t * msg, Json::Value& data);
	void onInterRegAuth(osip_message_t * msg, Json::Value& data);
	string makeSSRC(string serverid, int streamtype, string did);
	string adjustSDP(string orgsdp, string s, string y);
	void recoverDevMap();
	void recoverSesMap();

	void checkRegTimeout();
	void initLocalMessageQueue();
	void delDevFromMap(string did);
	string sipmessage2str(osip_message_t * msg);

private:
	//register
	void handleRegister(eXosip_event_t *je);
	void Register401Unauthorized(struct eXosip_t * peCtx, eXosip_event_t *je);
	void RegisterSuccess(struct eXosip_t * peCtx, eXosip_event_t *je);
	void Register403Failed(struct eXosip_t * peCtx, eXosip_event_t *je);

	void RegisterFailed(struct eXosip_t * peCtx, eXosip_event_t *je);

public:
	void testSendInvitePlay(bool);
	static const char * whitespace_cb(mxml_node_t *node, int where);
private:
	// call
	bool getStreamserver(CallSession*);
	bool checkSrvAnswer(char*, string& keepaliveport);
	void handleCallAnswer(eXosip_event_t *je);
	void handleRequestFail(eXosip_event_t *je);
	void huidianCallback(int err, char* did, char* serverip, int port);
	void ResponseCallDevAck(struct eXosip_t * peCtx, eXosip_event_t *je);
	void ResponseCallSrvAck(struct eXosip_t * peCtx, eXosip_event_t *je, CallSession*, char* sdp = NULL);
	int SendInvitePlay2Srv(struct eXosip_t *peCtx, string clientid, CallSession*, int sestype, string did = string(""), bool liveopened =
			false);
	int SendInvitePlay2dev(struct eXosip_t *peCtx, string did, int sesType, char* sdp = NULL);
	int closeSession(string devid, int sesType, int closetype = 1); //1 all 2 dev 3 srv
	CallSession* getSessionByCid(string did, int idType, int cid, int& sesType); //idtype 1 srvcid 2 devcid
	CallSession* getSession(string did, int type);
	void delSession(string did, int sestype);
	void delSessionBycid(string did, int callid);

	void split(const std::string& s, const std::string& delim, std::vector<std::string>* ret);
	void handleCallMsgResponse(struct eXosip_t * peCtx, eXosip_event_t *je);

private:
	// devinfo query
	int SendQueryCatalog(struct eXosip_t *peCtx, CameraDevice* deviceNode, int sn);
	int SendQueryCatalog1(struct eXosip_t *peCtx, CameraDevice* deviceNode, int sn);
	int SendQueryDeviceStatus(struct eXosip_t *peCtx, CameraDevice deviceNode);
	int SendQueryDeviceInfo(struct eXosip_t *peCtx, CameraDevice* deviceNode, char* agentid);
	int SendQueryDeviceConfig(struct eXosip_t *peCtx, CameraDevice deviceNode);
//	void handleCatalogResponse(eXosip_event_t *je, mxml_node_t* xml);
	void handleCatalogResponse1(eXosip_event_t *je, TiXmlHandle& handle);

	int SendRecordInfoCmd(struct eXosip_t *peCtx, CameraDevice* deviceNode, string channelID, string clientid, string starttime,
			string endtime);
	void handleRecordResponse(eXosip_event_t *je, mxml_node_t* xml);
	void handleRecordResponse1(eXosip_event_t *je, TiXmlHandle& handle);

	void handleDevInfoQuery(eXosip_event_t *je, mxml_node_t* xml);
	void handleDevInfoQuery1(eXosip_event_t *je, TiXmlHandle& handle);

	void handleDevStatus(eXosip_event_t *je, mxml_node_t* xml);
	void handleConfigDownload(eXosip_event_t *je, mxml_node_t* xml);
private:
	void handleDevAlarm1(eXosip_event_t *je, TiXmlHandle& handle);

	int SendAlarmResponse(struct eXosip_t * peCtx, eXosip_event_t *je, const char * deviceId, const char * sn, char * platformIpAddr,
			int platformSipPort);
	int SendQueryDeviceAlarm(struct eXosip_t *peCtx, CameraDevice deviceNode);
	void handleDevAlarmResp(eXosip_event_t *je, mxml_node_t* xml);
private:
	int SendCatlogSubscribe(struct eXosip_t *peCtx, CameraDevice* deviceNode, int catlogSN);
	int SendCatlogSubscribe1(struct eXosip_t *peCtx, CameraDevice* deviceNode, int catlogSN);

	int SendCatlogSubRefresh(CameraDevice* deviceNode);

private:
	// ctrl ptz
	int SendPresetQuery(struct eXosip_t *peCtx, CameraDevice deviceNode);
	void handlePresetQuery(eXosip_event_t *je, mxml_node_t* xml);
	int SendPTZCmd(struct eXosip_t *peCtx, CameraDevice *deviceNode, string channelid, string agentid, unsigned char* ptz);
	int SendFcousCmd(struct eXosip_t *peCtx, CameraDevice *deviceNode, string channelid, string agentid, unsigned char* ptz);

private:

	void Response(struct eXosip_t * peCtx, eXosip_event_t *je, int value);
	void Response403(struct eXosip_t * peCtx, eXosip_event_t *je);
	void Response200(struct eXosip_t * peCtx, eXosip_event_t *je);

	void showsdp(char* buf);
	bool isTcpSdp(char * buf);
private:
	void getDevDetail(string did);
	string utc2strtime(int);
	CameraDevice* findChannel(string channelid,int& stat);
private:

	struct eXosip_t *m_eCtx;
	bool m_is_sip_stop;
	int m_local_com_socket;

	typedef std::map<string, CameraDevice*>::iterator DevIt;
	typedef std::map<string, CallSession>::iterator SesIt;
	std::map<string, CameraDevice*> m_map_devs;
	std::map<string, CallSession> m_map_sessions;
	std::map<string, CallSession> m_map_playback_sessions;
	std::map<string, CallSession> m_map_download_sessions;

	int m_cur_idx;
	MessageQueue m_queue_[QUEUE_SIZE];
	DB m_dev_local_cache;
	RedisHandler m_dev_remote_cache;
	int m_lastCheckTime;

	ulu_mutex m_lock;
};

#endif // GB28181SERVER_H
