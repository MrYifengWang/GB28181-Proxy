#ifndef App_h__
#define App_h__

#include "common/ulu_lib.h"
#include <signal.h>
#include "debugLog.h"
#include "XunDianStatus.h"
#define GB_CENTER_ADDR "127.0.0.1"
#define GB_CENTER_PORT 29023
typedef enum {
	STAT_NEW, STAT_REG_PENDING, STAT_REG_OK, STAT_FAIL, STAT_RESET
};
typedef enum {
	MSG_JSON, MSG_SIP, MSG_XML, MSG_STR
};
typedef enum {
	CMD_NULL, CMD_REG, CMD_REG_REFRESH, CMD_ALARM, CMD_DEVSTAT, CMD_CHANNELSTAT, CMD_LOG
};

typedef enum E_DEV_ERROR_CODE {
	E_DEV_ERROR_OK = 0, //
	E_DEV_ERROR_OPERATION_NOT_SUPPORT = -1000, //
	E_DEV_ERROR_PARA_INVALID,
	E_DEV_ERROR_INTERNAL,
	E_DEV_ERROR_FILE_NOT_EXIST,
	E_DEV_ERROR_PROTOCOL_MISTAKE,
	E_DEV_ERROR_PROTOCOL_VERSION_MISMATCH,
	E_DEV_ERROR_REALPLAY_ALREADY,
	E_DEV_ERROR_REALPLAY_FAILED,
	E_DEV_ERROR_DOWNLOAD_ALREADY,
	E_DEV_ERROR_DOWNLOAD_FAILED,
	E_DEV_ERROR_PLAYBACK_ALREADY,
	E_DEV_ERROR_PLAYBACK_FAILED,
	E_DEV_ERROR_CAPTURE_FAILED,
	E_DEV_ERROR_INTERNAL_LAPI,
	E_DEV_ERROR_OFFLINE,	//-986
	E_DEV_ERROR_OPENRTPFAIL,	//-985
	E_DEV_ERROR_PLAYBACKBUSY,	//-984
	E_DEV_ERROR_NOCHANNEL,	//-983
	E_DEV_ERROR_NOSESSION,	//-982
	E_DEV_ERROR_NOSTREAMSERVER,	//-981

};
typedef enum LOG_MASK_BIT {
	BIT_1_REG = 0, BIT_2_UNKNOWN_MSG = 1, BIT_3_KEEP = 2, BIT_4_RECORDINFO = 3, BIT_5_QUERY = 4, BIT_7_MOTION = 6,
};
struct mediaserver {
	string m_streamserver_ip;
	string m_streamserver_sip_ip;
	string m_streamserver_id;
	int m_streamserver_sip_port;
	int m_streamserver_rtp_udp_port;
	int m_streamserver_rtp_tcp_port;
	int m_totalload;
	int m_timestamp;
	bool m_isdead;

};
//
//struct RTCP_Header {
//	unsigned short csrc_count :5;
//	unsigned short padding :1;
//	unsigned short version :2; //1 char
//	unsigned short payloadtype :8; //2 char
//	unsigned short length; //3,4 char
//	unsigned int ssrc; //5,6,7,8 char
//	unsigned int ssrc_1;
//	unsigned int flost :8;
//	unsigned int enlost :24;
//	unsigned int ehsn_l :16;
//	unsigned int ehsn_h :16;
//	unsigned int jitter;
//	unsigned int lsr;
//	unsigned int dlsr;
//
//};

struct channelStat {
	channelStat() {
		m_last_report_time = time(NULL);
		stat = -1;
	}
	string nvrid;
	string channelid;
	int stat;
	int m_last_report_time;
};
class MessageQueue;
class GB28181Server;
class CApp {
public:
	CApp();
	virtual ~CApp();

private:
	bool init();
	bool initOsip();

public:
	void start();
	bool loginCenterServer();
	void keepAlive();
	void shutdown();
	void handleCli();
	bool isDevNvr(string did);
	bool isChannelID(string did);
	string& replace_str(string& str, string to_replaced, string newchars);

	static int responseCallback(char *ptr, size_t size, size_t nmemb, string *buf);
	mediaserver* selectServer(string devip);
	mediaserver* selectServer1(string devip);
	bool selectServer2(string devip, mediaserver&);

	string httpGetStreamServer(string devip);

	bool parseConfig(Json::Value& jmessage);

	bool isServerIp(string ip);
	bool isServerSipIp(string ip);
	void addServer(mediaserver& server);
	void delServer(string serverip, int logintm = 0);
	void updateServerLoad(int load, string ip);
	int hasStreamServer();
	string getipbyname(string hostname);
	string makeSirealstr();
	void sendInterMessage(string& msg, int agentid = 100000);
	void cleanResponse();
	string getLocalIP();

public:
	int m_localUdpPort;
	int m_localCenterUdpPort;
	int m_localSipPort;
	string m_localIp;

	string m_center_server_ip;
	int m_center_server_port;

	string m_sipserver_id;
	string m_test_pswd;
	string m_sipserver_realm;
	string m_sipserver_ip;
	int m_sipserver_port;

//	string m_mediaserver_ip; //mediaserver
//	string m_mediaserver_sip_ip; //mediaserver
//	int m_mediaserver_sip_port;
//	int m_mediaserver_rtp_port;
	vector<mediaserver> m_streamservers;
	mutable ulu_mutex m_serverlock;
	mutable ulu_mutex m_siplock;

	string m_devmgr_ip;
	int m_devmgr_port;
	string m_alarm_srv_ip;
	int m_alarm_srv_port;
	int m_ssrc_count;
	int m_SerailNumber;
	string m_redis_ip;
	int m_redis_port;
	string m_redis_pswd;
	int m_xundian_timeout_min;
	int m_config_tm;

	/*
	 * 1 reg
	 * 2
	 * 3 keepalive
	 * 7 motion alarm
	 */
	int m_logmask;
	int m_loglevel;
	XunDianStatus m_xdmgr;

	int m_sip_lastchecktime;
	int m_sip_lastkeeptime;
	ulu_event m_CenterEvent;

	MessageQueue* m_log_queue;
	bool m_bstart;

private:
	ulu_event m_tEvent;
	struct eXosip_t *m_eCtx;

public:
	GB28181Server* m_pserver; //not safe, just for cli test

};

extern CApp theApp;

#endif
