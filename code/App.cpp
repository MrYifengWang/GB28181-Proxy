#include "App.h"
#include <signal.h>

#include <eXosip2/eXosip.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
using namespace std;
#include "GB28181Server.h"
#include "Threadepoll.h"
#include "ThreadTimer.h"
#include "ThreadDaemon.h"
#include "ThreadCenter.h"
#include <net/if.h>
#include <curl/curl.h>
#include "ThreadMessageProcess.h"

#define USE_OPENSSL

CApp theApp;

#ifdef USE_OPENSSL
/* we have this global to let the callback get easy access to it */
static pthread_mutex_t *lockarray;

#include <openssl/crypto.h>

static void lock_callback(int mode, int type, const char *file, int line) {
	(void) file;
	(void) line;
	if (mode & CRYPTO_LOCK) {
		pthread_mutex_lock(&(lockarray[type]));
	} else {
		pthread_mutex_unlock(&(lockarray[type]));
	}
}

static unsigned long thread_id(void) {
	unsigned long ret;

	ret = (unsigned long) pthread_self();
	return (ret);
}

static void init_locks(void) {
	int i;

	lockarray = (pthread_mutex_t *) OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
	for (i = 0; i < CRYPTO_num_locks(); i++) {
		pthread_mutex_init(&(lockarray[i]), NULL);
	}

	CRYPTO_set_id_callback((unsigned long (*)())thread_id);CRYPTO_set_locking_callback
	(lock_callback);
}

static void kill_locks(void) {
	int i;

	CRYPTO_set_locking_callback(NULL);
	for (i = 0; i < CRYPTO_num_locks(); i++)
		pthread_mutex_destroy(&(lockarray[i]));

	OPENSSL_free(lockarray);
}
#endif

CApp::CApp() {

	m_loglevel = INFO;
	m_log_queue = NULL;
	m_localUdpPort = 3500;
	m_localCenterUdpPort = 3501;
	m_localSipPort = 3050;
	m_ssrc_count = 0;
	m_logmask = 1;
	m_alarm_srv_ip = "-1";
	theApp.m_logmask |= 1 << (5 - 1);

	m_sip_lastchecktime = time(NULL);
	m_sip_lastkeeptime = m_sip_lastchecktime;

}

CApp::~CApp() {
	eXosip_quit(m_eCtx);
	osip_free(m_eCtx);
}

void CApp::handleCli() {

	while (1) {

		char command[64] = { 0 };
		cin >> command;

		if (strcmp("ivs", command) == 0) {
			puts("test invite to ms server");
		} else if (strcmp("ivd", command) == 0) {
			puts("test invite to dev ");
			m_pserver->testSendInvitePlay(1);
		}
	}
}

void CApp::start() {
//	string tmpip = getLocalIP();
//	m_localIp = tmpip;
//	if (!tmpip.empty()) {
//		puts(tmpip.c_str());
//	}

#ifdef USE_OPENSSL
	init_locks();
#endif
	init();
	ThreadCenter::startThread();
	m_CenterEvent.wait();
	m_log_queue = new MessageQueue();
	ThreadMessageProcess::startThread(m_log_queue);
	initOsip();

	GB28181Server::startThread(&m_pserver);
	sleep(1);
	CThreadepoll::startThread(0);
	CThreadTimer::startThread();
	ThreadDaemon::startThread();
	puts("server started!");
//	handleCli();

	m_tEvent.wait();

#ifdef USE_OPENSSL
	kill_locks();
#endif

}

bool CApp::initOsip() {
	m_eCtx = eXosip_malloc();
	int iRet = eXosip_init(m_eCtx);
	if (iRet != OSIP_SUCCESS) {
		exit(1);
	} else {
		puts("eXosip_client_init successfully!\n");
	}

	eXosip_listen_addr(m_eCtx, IPPROTO_UDP, NULL, theApp.m_localSipPort, AF_INET, 0);
//	eXosip_listen_addr(m_eCtx, IPPROTO_UDP, this->m_localIp.c_str(), theApp.m_localSipPort, AF_INET, 0);
//	eXosip_listen_addr(m_eCtx, IPPROTO_UDP, this->m_sipserver_ip.c_str(), theApp.m_localSipPort, AF_INET, 0);
	return true;
}
#include <eXosip2/eXosip.h>

void CApp::cleanResponse() {
	while (1) {
		eXosip_event_t *je = NULL;

		je = eXosip_event_wait(m_eCtx, 0, 50);
		if (je == NULL) {
			return;
		}

		eXosip_event_free(je);
		break;
	}

}
void CApp::sendInterMessage(string& msg, int agentid) {
	ulu_autoLock lock(m_siplock);

	osip_message_t *message = NULL;
	char dest_call[256] = { 0 };
	char source_call[256] = { 0 };

	snprintf(dest_call, 256, "sip:%s@%s:%d", "34000000002000000010", "127.0.0.1", 15060);
	snprintf(source_call, 256, "sip:%d@%s:%d", agentid, "127.0.0.1", theApp.m_localSipPort);

	int ret = eXosip_message_build_request(m_eCtx, &message, "MESSAGE", dest_call, source_call, NULL);

	if (ret == 0 && message != NULL) {
		osip_message_set_body(message, msg.c_str(), msg.length());
		osip_message_set_content_type(message, "Application/ULU+json");
		eXosip_lock(m_eCtx);
		eXosip_message_send_request(m_eCtx, message);
		eXosip_unlock(m_eCtx);

	}

	cleanResponse();

}

int CApp::responseCallback(char *ptr, size_t size, size_t nmemb, string *buf) {

	unsigned long sizes = size * nmemb;
	if (buf == NULL)
		return 0;
	buf->append(ptr, sizes);
	return sizes;

}

void CApp::shutdown() {
	m_tEvent.signal();
}
string CApp::getipbyname(string hostname) {
	{
		char *ptr, **pptr;
		struct hostent *hptr;
		char str[32];
		string ipstr;

		puts(hostname.c_str());
		if ((hptr = gethostbyname(hostname.c_str())) == NULL) {
			Debug(INFO, " gethostbyname error for host:%s\n", hostname.c_str());
			return ipstr;
		}

		for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
			printf(" alias:%s\n", *pptr);

		switch (hptr->h_addrtype) {
		case AF_INET:
		case AF_INET6:
			pptr = hptr->h_addr_list;
			for (; *pptr != NULL; pptr++) {
				Debug(VERBOSE, " address:%s\n", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
				ipstr = inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
			}
			break;
		default:
			Debug(INFO, "unknown address type\n");
			break;
		}

		return ipstr;
	}
}
void CApp::updateServerLoad(int load, string ip) {

	return;
	ulu_autoLock lock(m_serverlock);

	for (int i = 0; i < theApp.m_streamservers.size(); i++) {
		if (!theApp.m_streamservers[i].m_isdead && theApp.m_streamservers[i].m_streamserver_ip == ip) {
			theApp.m_streamservers[i].m_totalload += load;
			if (theApp.m_streamservers[i].m_totalload < 0) {
				theApp.m_streamservers[i].m_totalload = 0;
			}
			Debug(ERROR, "server load %d:%s is %d", load, ip.c_str(), theApp.m_streamservers[i].m_totalload);
			break;
		}
	}
}
void CApp::addServer(mediaserver& server) {
	ulu_autoLock lock(m_serverlock);

	for (int i = 0; i < theApp.m_streamservers.size(); i++) {
		if (theApp.m_streamservers[i].m_streamserver_ip == server.m_streamserver_ip
				&& theApp.m_streamservers[i].m_streamserver_id == server.m_streamserver_id) {
			Debug(INFO, "replace server:size=%d %s %s", theApp.m_streamservers.size(), theApp.m_streamservers[i].m_streamserver_ip.c_str(),
					theApp.m_streamservers[i].m_streamserver_sip_ip.c_str());
			theApp.m_streamservers.erase(m_streamservers.begin() + i);
			break;
		}
	}

	m_streamservers.push_back(server);

}
int CApp::hasStreamServer() {
	ulu_autoLock lock(m_serverlock);
	return m_streamservers.size();

}

void CApp::delServer(string serverip, int logintm) {
	ulu_autoLock lock(m_serverlock);

	for (int i = 0; i < theApp.m_streamservers.size(); i++) {
		if (theApp.m_streamservers[i].m_streamserver_ip == serverip && (logintm == 0 || logintm == theApp.m_streamservers[i].m_timestamp)) {
			theApp.m_streamservers.erase(theApp.m_streamservers.begin() + i);
			Debug(INFO, "delete stream server %s", serverip.c_str());
			break;
		}
	}
}
//mediaserver* selectServer1(string devip) {
//	//http get
//	mediaserver server;
//	server.m_streamserver_id = "";
//	server.m_streamserver_ip = "";
//	server.m_streamserver_sip_ip = "";
//
//	return &server;
//
//}
string CApp::httpGetStreamServer(string devip) {

	CURL *curl;

	string response;
//	{
//		Json::Value root;
//		Json::Value data;
//
//		data.append("212.129.142.170:8000");
//		root["data"] = data;
//
//		Json::FastWriter writer;
//		string message = writer.write(root);
//		printf("LB find server %s\n", message.c_str());
//		return message;
//	}
	int ret = 0;

	curl = curl_easy_init();
	if (curl) {
		char strURL[512] = { 0 };
		sprintf(strURL,
				"http://sip-stream-serv.biz.gslb.ulucu.com/listrrs?business_channel=sip-stream-serv.biz.gslb.ulucu.com&device_ip=%s",
				devip.c_str());
		Debug(INFO, "request LB:%s", strURL);

		string respBuf;
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_URL, strURL);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CApp::responseCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respBuf);
		int res = curl_easy_perform(curl);
		if (res != 0) {
			Debug(ERROR, "http get streamserver fail!");
			curl_easy_cleanup(curl);
			return response;
		} else {
			Debug(INFO, "http get streamserver:%s  response:%s", strURL, respBuf.c_str());
			Json::Reader reader;
			Json::Value jmessage;
			if (reader.parse(respBuf, jmessage)) {
				if (jmessage["code"].asInt() == 0) {
					response = respBuf;
				}
			}
		}
		curl_easy_cleanup(curl);
	}

	return response;
}
bool CApp::selectServer2(string devip, mediaserver& server) {

	return true;
	for (int i = 0; i < this->m_streamservers.size(); i++) {
		if ("192.168.2.196" == m_streamservers[i].m_streamserver_ip) {
			server = m_streamservers[i];
			return true;
		}
	}

	server.m_streamserver_id = theApp.m_sipserver_id;
	server.m_streamserver_ip = "192.168.2.196";
	server.m_streamserver_sip_ip = "192.168.2.196";
	server.m_streamserver_rtp_tcp_port = 8000;
	server.m_streamserver_rtp_udp_port = 8000;
	server.m_streamserver_sip_ip = "192.168.2.196";
	server.m_streamserver_sip_port = 5060;
	m_streamservers.push_back(server);
	return true;
}

mediaserver* CApp::selectServer1(string devip) {

	string resp = httpGetStreamServer(devip);

	if (resp.empty()) {
		return NULL;
	}
	Json::Reader reader;
	Json::Value jmessage;
	if (reader.parse(resp, jmessage)) {
		Json::Value arr;
		arr = jmessage["data"];

		int size = arr.size();
		if (size <= 0) {
			return NULL;
		}

		string serverstr = arr[0].asString();
		int pos = serverstr.find(":");
		string serverip = serverstr.substr(0, pos);

		Debug(INFO, "LB result:%s %s devip=%s\n", serverstr.c_str(), serverip.c_str(), devip.c_str());
		ulu_autoLock lock(m_serverlock);

		for (int i = 0; i < this->m_streamservers.size(); i++) {
			if (serverip == m_streamservers[i].m_streamserver_ip) {
				puts("found stream server!");
				return &m_streamservers[i];
			}
		}

		Debug(INFO, "stream server lost now,add it: %s devip=%s\n", serverip.c_str(), devip.c_str());

		{
			Json::Value root;
			root["cmd"] = "request_for_serverlist_notify";
			root["aid"] = 0;
			root["serial"] = "123";

			Json::FastWriter writer;
			string response = writer.write(root);

			int tmpsock;
			tmpsock = socket(AF_INET, SOCK_DGRAM, 0);
			if (tmpsock < 0)
				return NULL;
			struct sockaddr_in peerAddr;
			peerAddr.sin_family = AF_INET;
			peerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			peerAddr.sin_port = htons(theApp.m_localCenterUdpPort);
			int ret = sendto(tmpsock, response.c_str(), response.length(), 0, (struct sockaddr*) &peerAddr, sizeof(sockaddr_in));

			close(tmpsock);

		}

		mediaserver server;
		server.m_streamserver_id = theApp.m_sipserver_id;
		server.m_streamserver_ip = serverip;
		server.m_streamserver_sip_ip = serverip;
		server.m_streamserver_rtp_tcp_port = 8000;
		server.m_streamserver_rtp_udp_port = 8000;
		server.m_streamserver_sip_port = 5060;

		m_streamservers.push_back(server);

		return &m_streamservers.back();

	} else {
		return NULL;
	}

}
mediaserver* CApp::selectServer(string devip) {
	ulu_autoLock lock(m_serverlock);
	if (m_streamservers.size() < 1) {
		return NULL;
	}

	int minload = 10000;
	int i;
	int pos = 0;
	for (i = 0; i < this->m_streamservers.size(); i++) {
		if (!m_streamservers[i].m_isdead && m_streamservers[i].m_totalload < minload) {
			minload = m_streamservers[i].m_totalload;
			pos = i;
		}
	}
	return &m_streamservers[pos];
}
bool CApp::isServerIp(string ip) {
	ulu_autoLock lock(m_serverlock);

	for (int i = 0; i < this->m_streamservers.size(); i++) {
		if (!m_streamservers[i].m_isdead && ip == m_streamservers[i].m_streamserver_ip) {
			return true;
		}
	}
	return false;
}

bool CApp::isServerSipIp(string ip) {
	ulu_autoLock lock(m_serverlock);

	for (int i = 0; i < this->m_streamservers.size(); i++) {
		if (!m_streamservers[i].m_isdead && ip == m_streamservers[i].m_streamserver_sip_ip) {
			return true;
		}
	}
	return false;
}

bool CApp::init() {

	m_center_server_ip = "status-gb-stream.ulucu.com";
	m_center_server_ip = getipbyname(m_center_server_ip);
	m_center_server_port = 29023;

	string confPath = "gbserver.cfg";
	char buf[4096] = { 0 };
	int res = 0;
	if (res = access(confPath.c_str(), R_OK) < 0) {
		puts("no config file");
		return false;
	}
	FILE * pFile = fopen(confPath.c_str(), "r");
	if (!pFile) {
		return false;
	}
	int flen = fread(buf, sizeof(char), 4096, pFile);
	fclose(pFile);

	puts(buf);
	if (flen > 0) {
		Json::Reader reader;
		Json::Value jmessage;
		if (reader.parse(string(buf), jmessage)) {
			if (jmessage["center_server_ip"].isString()) {
				m_center_server_ip = jmessage["center_server_ip"].asString();
				m_center_server_ip = getipbyname(m_center_server_ip);
			}
			if (jmessage["center_server_port"].isInt()) {
				m_center_server_port = jmessage["center_server_port"].asInt();
			}

//			m_sipserver_ip = jmessage["gbserver_ip"].asString();
//			m_sipserver_id = jmessage["gbserver_id"].asString();
//			m_sipserver_realm = jmessage["gbserver_realm"].asString();
//			m_sipserver_port = jmessage["gbserver_port"].asInt();
//			m_devmgr_ip = jmessage["gbserver_ip"].asString();
//			m_devmgr_port = jmessage["gbserver_huidian_port"].asInt();
//			m_redis_ip = jmessage["redis_ip"].asString();
//			m_redis_port = jmessage["redis_port"].asInt();
//			m_redis_pswd = jmessage["redis_pswd"].asString();

//			m_mediaserver_sip_ip = getipbyname(m_mediaserver_sip_ip);

		}
	}

	return true;
}

string& CApp::replace_str(string& str, string to_replaced, string newchars) {
	for (string::size_type pos(0); pos != string::npos; pos += newchars.length()) {
		pos = str.find(to_replaced, pos);
		if (pos != string::npos)
			str.replace(pos, to_replaced.length(), newchars);
		else
			break;
	}
	return str;
}

bool CApp::isChannelID(string did) {
	if (did.empty())
		return false;
	if (did.length() != 20)
		return false;
	string devtype = did.substr(10, 3);
	int itype = atoi(devtype.c_str());
	if (itype == 131) {
		return true;
	} else {
		return false;
	}
}

bool CApp::isDevNvr(string did) {
	if (did.empty())
		return false;
	string devtype = did.substr(10, 3);
	int itype = atoi(devtype.c_str());
	if (itype > 110 && itype < 131) {
		return true;
	} else {
		return false;
	}
}
string CApp::makeSirealstr() {
	char buf[32] = { 0 };
	sprintf(buf, "%d", time(NULL));

	string tmserial = buf;
	return tmserial;
}
string CApp::getLocalIP() {

	string localip;
	int sfd, intr;
	struct ifreq buf[16];
	struct ifconf ifc;
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd < 0)
		return localip;
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = (caddr_t) buf;
	if (ioctl(sfd, SIOCGIFCONF, (char *) &ifc))
		return localip;
	intr = ifc.ifc_len / sizeof(struct ifreq);
	while (intr-- > 0 && ioctl(sfd, SIOCGIFADDR, (char *) &buf[intr]))
		;
	close(sfd);

	string tmpip = inet_ntoa(((struct sockaddr_in*) (&buf[intr].ifr_addr))->sin_addr);

	if (tmpip.size() > 3) {
		if (tmpip.substr(0, 3) != "127") {
			localip = tmpip;
		}
		Debug(ERROR, "local ip:%s", tmpip.c_str());

	}

	return localip;

}

bool CApp::parseConfig(Json::Value& jmessage) {

	m_sipserver_ip = jmessage["gbserver_ip"].asString();
	m_sipserver_id = jmessage["gbserver_id"].asString();
	m_sipserver_realm = jmessage["gbserver_realm"].asString();
	m_sipserver_port = jmessage["gbserver_port"].asInt();
	m_devmgr_ip = jmessage["gbserver_ip"].asString();
	m_devmgr_port = jmessage["gbserver_huidian_port"].asInt();
	m_redis_ip = jmessage["redis_ip"].asString();
	m_redis_port = jmessage["redis_port"].asInt();
	m_redis_pswd = jmessage["redis_pswd"].asString();
	if (jmessage["offsite_time"].isInt()) {
		m_xundian_timeout_min = jmessage["offsite_time"].asInt();
	} else {
		m_xundian_timeout_min = 15;
	}
	if (jmessage["config_tm"].isInt()) {
		int curtm = jmessage["config_tm"].asInt();
		if (curtm != m_config_tm) {
			m_config_tm = curtm;
			//reset
		}
	}

	return true;

}

//int parseRTCP() {
//
//	int readPos_;
//	char* recvBuf_;
//	char* rtpHead = recvBuf_ + 4;
//	RTCP_Header * RTCP_Header = rtpHead;
//
//	int len = (RTCP_Header->length + 1) * 4;
//	int tpos = len + 4;
//	if (readPos_ > tpos)
//		memmove(recvBuf_, recvBuf_ + tpos, readPos_ - tpos);
//	readPos_ -= tpos;
//
//	struct RTCP_Header RTCP_rr;
//	RTCP_rr.version = 2;
//	RTCP_rr.length = 7;
//	RTCP_rr.csrc_count = 1;
//	RTCP_rr.padding = 0;
//	RTCP_rr.payloadtype = 201;
//	RTCP_rr.ssrc = 0; //
//	RTCP_rr.flost = 0;
//	RTCP_rr.dlsr = 0;
//	RTCP_rr.enlost = 0;
//	RTCP_rr.lsr = 0;
//	RTCP_rr.jitter = 0;
//	RTCP_rr.ehsn_h = 0; //
//	RTCP_rr.ehsn_l = 0; //
//	RTCP_rr.ssrc_1 = 0; //
//
//	//send(socket,)
//
//}

