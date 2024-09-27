#include "HDAgent.h"
#include "epoll.h"
#include "App.h"
#include "Threadepoll.h"
#include <sstream>
#include <iostream>
#include "debugLog.h"
using namespace std;

Agent::Agent(SOCKET handle1, int efd, void* pmgr, int agentid, char* peerip, int peerport) {

	m_peerip = peerip;
	m_peerport = peerport;

	tmpstarttime = time(NULL);
	stoppos = 0;
	m_agentid = agentid;
	m_pparent = (CThreadepoll*) pmgr;
	m_efd = efd;
	m_isdisconnect = false;
	m_agent_type = 0;
	m_lastKeepalive = time(NULL);
	m_stream_login_tm = time(NULL);
	m_token = "0";
	m_isdead = false;

//	printf("agent create +++  %d\n", m_agentid);
//	struct sockaddr_in sa;
//	socklen_t len = sizeof(sa);
//	if (!getpeername((int) (SOCKET) m_cSocket, (struct sockaddr *) &sa, &len)) {
//		peerip = inet_ntoa(sa.sin_addr);
//		peerport = ntohs(sa.sin_port);
//
//		struct in_addr in = sa.sin_addr;
//		char str[INET_ADDRSTRLEN];
//
//		inet_ntop(AF_INET, &in, str, sizeof(str));
//
//		puts(str);
//
//	}

	m_cSocket.attach(handle1);
	assert(m_cSocket.enableNonblock(true));
	assert(m_cSocket.enableKeepAlive());
}

Agent::~Agent(void) {
//	Debug(ERROR, "agent release type=%d id=%d from %d to %d ; Host:%s:%d", m_agent_type, m_agentid, tmpstarttime, time(NULL),
//			m_peerip.c_str(), m_peerport);
	clearSessions();
}

void Agent::start(int identity) {

}

void Agent::release() {

	close();
}
void Agent::close() {

	m_pparent->delUser(m_agentid);

	if (epoll_ctl(m_efd, EPOLL_CTL_DEL, (int) (SOCKET) m_cSocket, NULL) == -1) {
		printf("epoll ctl err %d\n", errno);
	}
	m_pparent->m_totalsock--;
	m_cSocket.close();

	delete this;
}
void Agent::handleEvent(unsigned int ievent) {

	if (ievent & EPOLLIN) {
		if (!readCommand()) {
			return;
		}
	} else if (ievent & EPOLLOUT) {
		if (!m_cSocket.onSend()) {
			m_isdisconnect = true;
			m_deadtime = ulu_time::current();
			return;
		}
	} else if (ievent & EPOLLHUP) {
		close();
		return;
	} else if (ievent & EPOLLERR) {
		close();
		return;
	} else {
		printf("ievent %d", ievent);
		assert(0);
	}

	if (m_cSocket.wbufLen > 0) {
		struct epoll_event ev;
		puts(m_cSocket.m_wbuf);

		ev.events = EPOLLIN | EPOLLOUT | EPOLLERR;
		ev.data.ptr = this;
		if (epoll_ctl(m_efd, EPOLL_CTL_MOD, (int) (SOCKET) m_cSocket, &ev) < 0) {
			ctassert(0);
		}
	} else {
		struct epoll_event ev;

		ev.events = EPOLLIN | EPOLLERR;
		ev.data.ptr = this;
		if (epoll_ctl(m_efd, EPOLL_CTL_MOD, (int) (SOCKET) m_cSocket, &ev) < 0) {
			printf("errno= %d", errno);
			ctassert(0);
		}
	}

}

bool Agent::readCommand() {
	if (0 > m_cSocket.readall()) {

		release();
		return false;
	}
	while (1) {
		string command;
		int iret = m_cSocket.readpacket(command);

		if (iret < 0) {
			if (iret == -1) {
				if (errno == 11)
					return true;
			}

			release();
			return false;
		}
		if (iret == 0) {
			break;
		}
		handleCommand(command);
	}

	return true;
}

bool Agent::handleCommandLogin(string& command) {

	Json::Value root;
	root["cmd"] = "client_login_mgr_ack";
	root["serial"] = "123";
	root["error"] = E_DEV_ERROR_OK;
	root["reason"] = "OK";
	Json::Value data;
	data["result"] = "OK";
	root["data"] = data;

	sendResponse(root);

}
bool Agent::handleCommandUpdateAlarmCfg(string& command) {

}
bool Agent::handleCommandStreamLogin(string& command) {

	return false;
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(command, jmessage)) {
		return false;
	}

	{
		Json::Value root;
		root["cmd"] = "stream_login_mgr_ack";
		root["serial"] = "123";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		Json::Value data;
		data["result"] = "OK";
		root["data"] = data;

		sendResponse(root);
	}

	{

		m_agent_type = 1;
		m_sip_server_id = jmessage["data"]["sip_id"].asString();
		m_stream_login_tm = jmessage["data"]["tm"].asInt();
		if (jmessage["data"]["token"].isString()) {
			m_token = jmessage["data"]["token"].asString();
		}
		mediaserver server;
//		server.m_streamserver_ip = theApp.m_mediaserver_ip;
//		server.m_streamserver_rtp_udp_port = theApp.m_mediaserver_rtp_port;
//		server.m_streamserver_sip_ip = theApp.m_mediaserver_sip_ip;
//		server.m_streamserver_sip_port = theApp.m_mediaserver_sip_port;

		server.m_totalload = 0;
		//	server.m_streamserver_sip_ip = this->m_peerip;
		server.m_streamserver_id = m_sip_server_id;
		server.m_timestamp = m_stream_login_tm;
		server.m_isdead = false;
		if (jmessage["data"]["stream_ip"].isString()) {
			server.m_streamserver_ip = jmessage["data"]["stream_ip"].asString();
			server.m_streamserver_ip = theApp.getipbyname(server.m_streamserver_ip);
			server.m_streamserver_sip_ip = server.m_streamserver_ip;
			string tststr = theApp.getipbyname(server.m_streamserver_ip);
			if (tststr.empty()) {
				Debug(ERROR, "gethostbyname fail  ");
			}

		}
		if (jmessage["data"]["stream_sip_port"].isInt()) {
			server.m_streamserver_sip_port = jmessage["data"]["stream_sip_port"].asInt();

		}
		if (jmessage["data"]["stream_rtp_udp_port"].isInt()) {
			server.m_streamserver_rtp_udp_port = jmessage["data"]["stream_rtp_udp_port"].asInt();

		}
		if (jmessage["data"]["stream_rtp_tcp_port"].isInt()) {
			server.m_streamserver_rtp_tcp_port = jmessage["data"]["stream_rtp_tcp_port"].asInt();

		}
		theApp.addServer(server);

		Debug(INFO, "stream server login in:%s %d %s:%d token=%s", this->m_sip_server_id.c_str(), this->m_agent_type,
				this->m_peerip.c_str(), this->m_peerport, m_token.c_str());
		if (true == this->m_pparent->delStreamUser(m_sip_server_id, m_stream_login_tm, m_token)) {
			sleep(2);
		}
	}

	return true;

}
bool Agent::checkKeepalive(int tm) {
	if (m_agent_type != 1) {
		return true;
	} else {
		if (tm - this->m_lastKeepalive > 30 * 3) {
			Debug(ERROR, "stream server no keepalive");
			return false;
		}
	}
	return true;
}
void Agent::clearSessions() {
//	if (m_agent_type == 1 && !m_isdead) {
//		Json::Value root;
//		root["cmd"] = "reset_all_session";
//		root["serverip"] = this->m_peerip;
//
//		Json::FastWriter writer;
//		string message = writer.write(root);
//		theApp.sendInterMessage(message, m_agentid);
//
//		theApp.delServer(this->m_peerip, m_stream_login_tm);
//	}
}

void Agent::handleCommand(string& command) {

	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(command, jmessage)) {
		return;
	}

	string serialstr = "123";
	if (jmessage["serial"].isString()) {
		serialstr = jmessage["serial"].asString();
	}
	string cmdstr = jmessage["cmd"].asString();
	if (cmdstr != "client_keepalive_mgr_req" && cmdstr != "stream_keepalive_mgr_req" && cmdstr != "daemon_login_req") {
		Debug(INFO, "huidian cmd:%s", command.c_str());
		puts(command.c_str());
	}
	if (cmdstr == "daemon_login_req") {
		Json::Value root;
		root["cmd"] = "daemon_login_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;
		sendResponse(root, false);
	} else if (cmdstr == "client_login_mgr_req") {
		handleCommandLogin(command);
	} else if (cmdstr == "stream_login_mgr_req") {
		handleCommandStreamLogin(command);
	} else if (cmdstr == "set_logmask_req") {
		int bit = jmessage["data"]["bit"].asInt();
		int val = jmessage["data"]["value"].asInt();
		if (val == 0) {
			theApp.m_logmask &= (~(1 << (bit - 1)));
		} else {
			theApp.m_logmask |= 1 << (bit - 1);
		}

		Json::Value root;
		root["cmd"] = "set_logmask_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;
		Json::Value data;
		data["mask"] = theApp.m_logmask;
		root["data"] = data;

		sendResponse(root);
	} else if (cmdstr == "print_xundian_cfg_req") {
		string did = jmessage["data"]["did"].asString();
		string ret = theApp.m_xdmgr.printConfig(did);

		Json::Value root;
		root["cmd"] = "print_xundian_cfg_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;
		if (!ret.empty()) {
			Json::Value data;
			data["cfg"] = ret;
			root["data"] = data;
		}

		sendResponse(root);

	} else if (cmdstr == "update_offsite_cycle_req") {
		theApp.m_xdmgr.onConfigUpdate1(command);
		Json::Value root;
		root["cmd"] = "update_offsite_cycle_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;

		sendResponse(root);
	} else if (cmdstr == "stop_server_req") {
		Debug(ERROR, "restart server from remote");
		exit(0);
	} else if (cmdstr == "get_online_req") {
		m_cur_clientid = "0";
		theApp.sendInterMessage(command, m_agentid);

		Json::Value root;
		root["cmd"] = "get_online_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;

		sendResponse(root);

	} else if (cmdstr == "reset_dev_req") {
		m_cur_clientid = "0";
		theApp.sendInterMessage(command, m_agentid);
		;

		Json::Value root;
		root["cmd"] = "reset_dev_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;

		sendResponse(root);

	} else if (cmdstr == "get_device_version_req") {

		Json::Value root;
		root["cmd"] = "get_device_version_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;
		Json::Value data;
		data["version"] = "null";
		root["data"] = data;

		sendResponse(root);

	} else if (cmdstr == "client_keepalive_mgr_req") {

		Json::Value root;
		root["cmd"] = "client_keepalive_mgr_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;
		Json::Value data;
		root["data"] = data;

		sendResponse(root, false);

	} else if (cmdstr == "stream_keepalive_mgr_req") {

		m_lastKeepalive = time(NULL);
		Json::Value root;
		root["cmd"] = "stream_keepalive_mgr_ack";
		root["error"] = E_DEV_ERROR_OK;
		root["reason"] = "OK";
		root["serial"] = serialstr;
		Json::Value data;
		root["data"] = data;

		sendResponse(root, false);

	} else if (cmdstr == "call_invite_req") {
		string did = jmessage["data"]["did"].asString();
		string clientid = jmessage["data"]["clientid"].asString();
		if (did.empty() || did.length() != 20 || clientid == "0") {
			Json::Value root;
			root["cmd"] = "call_invite_ack";
			root["error"] = E_DEV_ERROR_PARA_INVALID;
			root["reason"] = "did is invalid!";
			root["serial"] = serialstr;
			sendResponse(root);
		} else {
			m_cur_clientid = jmessage["data"]["clientid"].asString();
			theApp.sendInterMessage(command, m_agentid);

		}

	} else if (cmdstr == "call_close_req") {
		theApp.sendInterMessage(command, m_agentid);

	} else if (cmdstr == "get_device_information_req") {
		m_cur_clientid = jmessage["data"]["clientid"].asString();
		theApp.sendInterMessage(command, m_agentid);
	} else if (cmdstr == "query_playback_req") {

		string did = jmessage["data"]["nvrid"].asString();
		string stt = jmessage["data"]["starttime"].asString();
		string edt = jmessage["data"]["endtime"].asString();
		m_recordStarttime = atoi(stt.c_str());
		m_recordEndtime = atoi(edt.c_str());
		m_cur_nvrid = did;
//		if (!theApp.isDevNvr(did)) {
//			Json::Value root;
//			Json::Value data;
//			root["cmd"] = "query_playback_ack";
//			root["error"] = E_DEV_ERROR_OPERATION_NOT_SUPPORT;
//			root["reason"] = "dev not surpport";
//			root["serial"] = serialstr;
//
//			data["clientid"] = jmessage["data"]["clientid"].asString();
//			data["nvrid"] = did;
//			data["did"] = jmessage["data"]["did"].asString();
//			data["num"] = 0;
//			root["data"] = data;
//
//			sendResponse(root);
//
//		} else
		{
			m_cur_clientid = jmessage["data"]["clientid"].asString();
			theApp.sendInterMessage(command, m_agentid);
		}
	} else if (cmdstr == "query_recored_req") {
		theApp.sendInterMessage(command, m_agentid);
	} else if (cmdstr == "direction_ctrl_req") {
		theApp.sendInterMessage(command, m_agentid);
		Json::Value root;
		Json::Value data;
		root["cmd"] = "direction_ctrl_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";
		data["did"] = jmessage["data"]["did"].asString();
		root["data"] = data;

		sendResponse(root);

	} else if (cmdstr == "focus_ctrl_req") {
		theApp.sendInterMessage(command, m_agentid);
		Json::Value root;
		Json::Value data;
		root["cmd"] = "focus_ctrl_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";
		data["did"] = jmessage["data"]["did"].asString();
		root["data"] = data;

		sendResponse(root);

	} else if (cmdstr == "zoom_ctrl_req") {
		theApp.sendInterMessage(command, m_agentid);
		Json::Value root;
		Json::Value data;
		root["cmd"] = "zoom_ctrl_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";
		data["did"] = jmessage["data"]["did"].asString();
		root["data"] = data;

		sendResponse(root);

	} else if (cmdstr == "stop_ctrl_req") {
		theApp.sendInterMessage(command, m_agentid);
		Json::Value root;
		Json::Value data;
		root["cmd"] = "stop_ctrl_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";
		data["did"] = jmessage["data"]["did"].asString();
		root["data"] = data;

		sendResponse(root);

	} else if (cmdstr == "get_dev_status_req") {
		theApp.sendInterMessage(command, m_agentid);
		;
		Json::Value root;
		Json::Value data;
		root["cmd"] = "get_dev_status_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";
		sendResponse(root);

	} else if (cmdstr == "get_dev_config_req") {
		theApp.sendInterMessage(command, m_agentid);
		;
		Json::Value root;
		Json::Value data;
		root["cmd"] = "get_dev_config_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";

		sendResponse(root);

	} else if (cmdstr == "get_dev_catalog_req") {
		theApp.sendInterMessage(command, m_agentid);
		;
		Json::Value root;
		Json::Value data;
		root["cmd"] = "get_dev_catalog_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";

		sendResponse(root);

	} else if (cmdstr == "get_dev_alarm_req") {
		theApp.sendInterMessage(command, m_agentid);
		;
		Json::Value root;
		Json::Value data;
		root["cmd"] = "get_dev_alarm_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";

		sendResponse(root);

	} else if (cmdstr == "get_dev_mobil_pos_req") {
		theApp.sendInterMessage(command, m_agentid);
		;
		Json::Value root;
		Json::Value data;
		root["cmd"] = "get_dev_mobil_pos_ack";
		root["error"] = 0;
		root["serial"] = serialstr;
		root["reason"] = "ok";

		sendResponse(root);

	}
}

bool Agent::handleHungup(string& command) {
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(command, jmessage)) {
		return false;
	}
	return true;

}
bool Agent::handleQuit(string& command) {
	m_pparent->delUser(m_agentid);
	delete this;
	return true;
}

void Agent::sendCommand(string& command) {
	if (m_cSocket.wbufLen < 1024 * 200) {
		if (!m_cSocket.asynwrite(command)) {
			m_isdisconnect = true;
			m_deadtime = ulu_time::current();

			return;
		}
	} else {
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLOUT;
		ev.data.ptr = this;
		if (epoll_ctl(m_efd, EPOLL_CTL_MOD, (int) (SOCKET) m_cSocket, &ev) < 0) {
			ctassert(0);
		}
		return;
	}
}

void Agent::sendResponse(Json::Value& json, bool log) {

	Json::FastWriter writer;
	string message = writer.write(json);
	puts(message.c_str());
	if (log) {
		Debug(INFO, "response from:%d to client:%s", m_agentid, message.c_str());
	}

	this->m_cSocket.asynwrite(message);
}
void Agent::handleJsonResponse(Json::Value& jmessage) {

	string cmdstr = jmessage["cmd"].asString();
	if (cmdstr == "call_invite_ack") {
		Json::Value root;
		Json::Value data;

		string result = jmessage["result"].asString();
		root["serial"] = "";
		if (result == "ok") {
			root["cmd"] = "call_invite_ack";
			root["error"] = E_DEV_ERROR_OK;
			root["reason"] = "OK";
			data["url"] = jmessage["url"].asString();
			if (this == NULL) {
				puts("null agent");
				return;
			}
			if (this->m_cur_clientid.empty()) {
				puts("null clientid");
				return;
			}
			data["clientid"] = this->m_cur_clientid;
			data["did"] = jmessage["did"].asString();
			data["live"] = jmessage["type"].asInt();
			data["http_ip"] = jmessage["http_ip"].asString();
			data["http_port"] = jmessage["http_port"].asInt();
			root["data"] = data;
		} else if (result == "devfail") {
			root["cmd"] = "call_invite_ack";
			root["error"] = E_DEV_ERROR_OPENRTPFAIL;
			root["reason"] = "open dev rtp stream fail!";
			root["data"] = data;
		} else if (result == "busy") {
			root["cmd"] = "call_invite_ack";
			root["error"] = E_DEV_ERROR_PLAYBACKBUSY;
			root["reason"] = "other client is using playback!";
			root["data"] = data;
		} else if (result == "interfail") {
			root["cmd"] = "call_invite_ack";
			root["error"] = E_DEV_ERROR_INTERNAL;
			root["reason"] = "invite dev fail!";
			root["data"] = data;
		} else {
			root["cmd"] = "call_invite_ack";
			root["error"] = E_DEV_ERROR_OFFLINE;
			root["reason"] = "device offline!";
			root["data"] = data;
		}
		sendResponse(root);

	} else if (cmdstr == "query_playback_ack") {
		Json::Value root;
		Json::Value data;

		string result = jmessage["result"].asString();
		root["cmd"] = "query_playback_ack";
		data["clientid"] = this->m_cur_clientid;
		data["did"] = jmessage["did"].asString();
		data["nvrid"] = jmessage["nvrid"].asString();
		data["num"] = 0;
		root["data"] = data;
		if (result == "offline") {
			root["error"] = E_DEV_ERROR_OFFLINE;
			root["reason"] = "offline";

		} else if (result == "nochannel") {
			root["error"] = E_DEV_ERROR_NOCHANNEL;
			root["reason"] = "no did in nvr";
		}
		sendResponse(root);

	} else if (cmdstr == "call_close_ack") {
		string result = jmessage["result"].asString();
		Json::Value root;
		Json::Value data;

		root["serial"] = "";
		root["cmd"] = "call_close_ack";
		data["did"] = jmessage["did"].asString();
		root["data"] = data;
		if (result == "ok") {
			root["reason"] = "OK";
			root["error"] = E_DEV_ERROR_OK;
		} else if (result == "closed") {
			if (jmessage["reason"].isString()) {
				root["reason"] = jmessage["reason"].asString();
			} else {
				root["reason"] = "already closed";
			}
			root["error"] = E_DEV_ERROR_NOSESSION;
		} else if (result == "busy") {
			if (jmessage["reason"].isString()) {
				root["reason"] = jmessage["reason"].asString();
			} else {
				root["reason"] = "can not close";
			}
			root["error"] = E_DEV_ERROR_PLAYBACKBUSY;
		} else {
			root["reason"] = "OK";
			root["error"] = E_DEV_ERROR_OK;
		}

		sendResponse(root);

	}

}
void Agent::handleSipResponse(osip_message_t * message) {

	osip_content_type_t * contentType = osip_message_get_content_type(message);

	if (contentType != NULL) {

		if (strcmp(contentType->subtype, "MANSCDP+xml") == 0) {
			osip_body_t *body = NULL;
			osip_message_get_body(message, 0, &body);
			if (body != NULL) {
				TiXmlDocument doc;
				doc.Parse(body->body);
				TiXmlHandle docHandle(&doc);
				TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("CmdType").ToElement();
				if (NULL != devinfo) {
					char* cmdstr = (char*) devinfo->ToElement()->GetText();

					if (strcmp(cmdstr, "RecordInfo") == 0) {
						handleSipResponseRecordInfo1(message, docHandle);
						return;
					} else if (0 == strcmp(cmdstr, "RecordInfo")) {
						handleSipResponseDevInfo1(message, docHandle);
						return;
					}
				}

				mxml_node_t *xml = mxmlLoadString(NULL, body->body, MXML_NO_CALLBACK);
				mxml_node_t * CmdTypeNode = mxmlFindElement(xml, xml, "CmdType", NULL, NULL, MXML_DESCEND);

				if (CmdTypeNode != NULL) {

					const char *CmdType = mxmlGetText(CmdTypeNode, NULL);
					printf("   ------> CmdType=%s \n", CmdType);

					if (0 == strcmp(CmdType, "DeviceInfo")) {
						handleSipResponseDevInfo(message, xml);
					} else if (0 == strcmp(CmdType, "RecordInfo")) {
						handleSipResponseRecordInfo(message, xml);
					}
				}
			}
		}
	}
}

void Agent::handleSipResponseDevInfo1(osip_message_t * message, TiXmlHandle& doc) {

	Json::Value root;
	Json::Value data;

	puts("make devinfo response");
	root["cmd"] = "get_device_information_ack";
	root["error"] = E_DEV_ERROR_OK;
	root["reason"] = "OK";
	root["serial"] = "123";

	osip_body_t *body = NULL;
	osip_message_get_body(message, 0, &body);
	if (body != NULL) {

		TiXmlDocument doc;
		doc.Parse(body->body);
		TiXmlHandle docHandle(&doc);
		{
			TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("DeviceID").ToElement();
			if (NULL != devinfo) {
				char* DeviceID = (char*) devinfo->ToElement()->GetText();
				data["device_id"] = DeviceID;
			}
		}

		{
			TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("DeviceName").ToElement();
			if (devinfo != NULL) {
				const char *DeviceName = (char*) devinfo->ToElement()->GetText();
				printf("DeviceName is %s", DeviceName);
				data["device_name"] = DeviceName;

			}
		}
		{
			TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("Manufacturer").ToElement();
			if (devinfo != NULL) {
				const char *Manufacturer = (char*) devinfo->ToElement()->GetText();
				printf("Manufacturer is %s", Manufacturer);
				data["device_factory"] = Manufacturer;

			}
		}
		{
			TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("Model").ToElement();

			if (devinfo != NULL) {
				const char *Model = (char*) devinfo->ToElement()->GetText();
				printf("Model is %s", Model);
				data["oem_device_type"] = Model;

			}
		}
		{
			TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("Firmware").ToElement();

			if (devinfo != NULL) {
				const char *Firmware = (char*) devinfo->ToElement()->GetText();
				printf("Firmware is %s", Firmware);
				data["oem_master_version"] = Firmware;

			}
		}
		{
			TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("Channel").ToElement();

			if (devinfo != NULL) {
				const char *Channel = (char*) devinfo->ToElement()->GetText();
				printf("Channel is %s", Channel);
				data["Channel"] = Channel;

			}
		}
		{
			TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("Info").ToElement();

			if (devinfo != NULL) {
				const char *Info = (char*) devinfo->ToElement()->GetText();
				printf("Info is %s", Info);
				data["info"] = Info;

			}
		}
	}

	root["data"] = data;
	sendResponse(root);

}
void Agent::handleSipResponseDevInfo(osip_message_t * message, mxml_node_t* xml) {

	Json::Value root;
	Json::Value data;

	puts("make devinfo response");
	root["cmd"] = "get_device_information_ack";
	root["error"] = E_DEV_ERROR_OK;
	root["reason"] = "OK";
	root["serial"] = "123";

	osip_body_t *body = NULL;
	osip_message_get_body(message, 0, &body);
	if (body != NULL) {
		mxml_node_t *xml = mxmlLoadString(NULL, body->body, MXML_NO_CALLBACK);
		mxml_node_t * DeviceIDNode = mxmlFindElement(xml, xml, "DeviceID", NULL, NULL, MXML_DESCEND); //must

		if (DeviceIDNode != NULL) {
			const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);
			data["device_id"] = DeviceID;
		}

		mxml_node_t * DeviceNameNode = mxmlFindElement(xml, xml, "DeviceName", NULL, NULL, MXML_DESCEND); //option
		mxml_node_t * ResultNode = mxmlFindElement(xml, xml, "Result", NULL, NULL, MXML_DESCEND); //must
		mxml_node_t * ManufacturerNode = mxmlFindElement(xml, xml, "Manufacturer", NULL, NULL, MXML_DESCEND); //option
		mxml_node_t * ModelNode = mxmlFindElement(xml, xml, "Model", NULL, NULL, MXML_DESCEND); //option
		mxml_node_t * FirmwareNode = mxmlFindElement(xml, xml, "Firmware", NULL, NULL, MXML_DESCEND); //option
		mxml_node_t * ChannelNode = mxmlFindElement(xml, xml, "Channel", NULL, NULL, MXML_DESCEND); //option
		mxml_node_t * InfoNode = mxmlFindElement(xml, xml, "Info", NULL, NULL, MXML_DESCEND); //option

		if (DeviceNameNode != NULL) {
			const char *DeviceName = mxmlGetText(DeviceNameNode, NULL);
			printf("DeviceName is %s", DeviceName);
			data["device_name"] = DeviceName;

		}
		const char *Result = mxmlGetText(ResultNode, NULL);
		if (ManufacturerNode != NULL) {
			const char *Manufacturer = mxmlGetText(ManufacturerNode, NULL);
			printf("Manufacturer is %s", Manufacturer);
			data["device_factory"] = Manufacturer;

		}
		if (ModelNode != NULL) {
			const char *Model = mxmlGetText(ModelNode, NULL);
			printf("Model is %s", Model);
			data["oem_device_type"] = Model;

		}
		if (FirmwareNode != NULL) {
			const char *Firmware = mxmlGetText(FirmwareNode, NULL);
			printf("Firmware is %s", Firmware);
			data["oem_master_version"] = Firmware;

		}
		if (ChannelNode != NULL) {
			const char *Channel = mxmlGetText(ChannelNode, NULL);
			printf("Channel is %s", Channel);
			data["Channel"] = Channel;

		}
		if (InfoNode != NULL) {
			const char *Info = mxmlGetText(InfoNode, NULL);
			printf("Info is %s", Info);
			data["info"] = InfoNode;

		}
	}

	root["data"] = data;
	sendResponse(root);

}
void Agent::handleSipResponseRecordInfo(osip_message_t * message, mxml_node_t* xml) {

	Json::Value root;
	Json::Value data;
	Json::Value arr;

	root["cmd"] = "query_playback_ack";
	root["error"] = E_DEV_ERROR_OK;
	root["reason"] = "OK";
	root["serial"] = "123";
	data["clientid"] = m_cur_clientid;

	osip_to_t *hto = osip_message_get_to(message);
	osip_body_t *body = NULL;
	osip_message_get_body(message, 0, &body);
	if (body != NULL) {
		puts(body->body);
		mxml_node_t *xml = mxmlLoadString(NULL, body->body, MXML_NO_CALLBACK);
		mxml_node_t * DeviceIDNode = mxmlFindElement(xml, xml, "DeviceID", NULL, NULL, MXML_DESCEND); //must

		if (DeviceIDNode != NULL) {
			const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);
			data["did"] = DeviceID;
		}
		data["nvrid"] = hto->url->username;

//		mxml_node_t * NameNode = mxmlFindElement(xml, xml, "Name", NULL, NULL, MXML_DESCEND); //must
//		if (NameNode != NULL) {
//			const char *Name = mxmlGetText(NameNode, NULL);
//			printf("Name is %s", Name);
//		}
//
//		mxml_node_t * SumNumNode = mxmlFindElement(xml, xml, "SumNum", NULL, NULL, MXML_DESCEND); //must
//		int SumNum = 0;
//
//		if (SumNumNode != NULL) {
//			SumNum = mxmlGetInteger(SumNumNode);
//			printf("SumNum is %d", SumNum);
//		}

		mxml_node_t * RecordListNode = mxmlFindElement(xml, xml, "RecordList", NULL, NULL, MXML_DESCEND);

		if (RecordListNode != NULL) {
			const char *Num = mxmlElementGetAttr(RecordListNode, "Num");
			int count = atoi(Num);
			data["num"] = count;

			mxml_node_t * ItemNode = mxmlFindElement(RecordListNode, RecordListNode, "Item", NULL, NULL, MXML_DESCEND);

			for (int i = 0; i < count; i++) {
				if (i != 0)
					ItemNode = mxmlFindElement(ItemNode, RecordListNode, "Item", NULL, NULL, MXML_DESCEND);

				if (ItemNode != NULL) {

					//	mxml_node_t * NameNode = mxmlFindElement(ItemNode, ItemNode, "Name", NULL, NULL, MXML_DESCEND);
					mxml_node_t * StarttimeNode = mxmlFindElement(ItemNode, ItemNode, "StartTime", NULL, NULL, MXML_DESCEND);
					mxml_node_t * EndtimeNode = mxmlFindElement(ItemNode, ItemNode, "EndTime", NULL, NULL, MXML_DESCEND);

					//	const char *Name = mxmlGetText(NameNode, NULL);
					const char *starttime = mxmlGetText(StarttimeNode, NULL);
					const char *endtime = mxmlGetText(EndtimeNode, NULL);

					Json::Value item;
					item["b"] = starttime;
					item["e"] = endtime;
					item["s"] = endtime;
					item["u"] = endtime;
					arr.append(item);

				}
			}
			data["list"] = arr;
		}

	}

	root["data"] = data;
	sendResponse(root);

}
bool ReadStringAttribute(TiXmlElement* element, const char* AttributeName, string& AttributeValue) {
	TiXmlAttribute* attribute = element->FirstAttribute();
	while (attribute != 0) {
		if (strcmp(AttributeName, attribute->Name()) == 0) {
			AttributeValue = attribute->Value();
			return true;
		}
		attribute = attribute->Next();
	}
	return false;
}

void Agent::handleSipResponseRecordInfo1(osip_message_t * message, TiXmlHandle& xml) {

	int sumnum = 0;
	int num = 0;

	if (m_tmpRecordData.isNull()) {
		Json::Value data;
		Json::Value arr;

		m_tmpRecordData["cmd"] = "query_playback_ack";
		m_tmpRecordData["error"] = E_DEV_ERROR_OK;
		m_tmpRecordData["reason"] = "OK";
		m_tmpRecordData["serial"] = "123";
		m_tmpRecordData["data"] = data;
		m_tmpRecordData["data"]["list"] = arr;
		m_tmpRecordData["data"]["clientid"] = m_cur_clientid;
	}

	osip_to_t *hto = osip_message_get_to(message);
	osip_body_t *body = NULL;
	osip_message_get_body(message, 0, &body);
	if (body != NULL) {
		TiXmlDocument doc;
		doc.Parse(body->body);
		TiXmlHandle docHandle(&doc);
		TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("DeviceID").ToElement();
		if (NULL != devinfo) {
			char* DeviceID = (char*) devinfo->ToElement()->GetText();
			m_tmpRecordData["data"]["did"] = DeviceID;
		}

		TiXmlNode * suminfo = docHandle.FirstChild("Response").FirstChild("SumNum").ToElement();
		if (NULL != suminfo) {
			char* DeviceID = (char*) suminfo->ToElement()->GetText();
			sumnum = atoi(DeviceID);
			m_tmpRecordData["data"]["num"] = sumnum;
		}
		if (!m_cur_nvrid.empty()) {
			m_tmpRecordData["data"]["nvrid"] = this->m_cur_nvrid;
		}

		TiXmlNode * list = docHandle.FirstChild("Response").FirstChild("RecordList").ToElement();

		if (NULL != list) {
			string countStr;
			ReadStringAttribute(list->ToElement(), "Num", countStr);
			num = atoi(countStr.c_str());

			TiXmlNode *nodeEle = list->FirstChild("Item");

			if (nodeEle != NULL) {
				for (; nodeEle != NULL; nodeEle = nodeEle->NextSiblingElement()) {
					if (nodeEle == NULL)
						break;
					if (nodeEle->FirstChild("StartTime") == NULL || nodeEle->FirstChild("EndTime") == NULL)
						break;
					char * starttime = (char*) nodeEle->FirstChild("StartTime")->ToElement()->GetText();
					char * endtime = (char*) nodeEle->FirstChild("EndTime")->ToElement()->GetText();

					int filesize = 0;
					string unit = "KB";
					if (nodeEle->FirstChild("FileSize") != NULL) {
						char * sizestr = (char*) nodeEle->FirstChild("FileSize")->ToElement()->GetText();
						if (sizestr != NULL) {
							long int size = atol(sizestr);

							if (size < 2147483647) {
								filesize = size;
								unit = "B";
							} else {
								filesize = size / 1024;
							}
						}

					}
					string sttimestr, edtimestr;
					if (filesize < 1) {
						int sttm, endtm;
						{
							tm tm_;
							strptime(starttime, "%Y-%m-%dT%H:%M:%S", &tm_);
							sttm = mktime(&tm_);
						}
						{
							tm tm_;
							strptime(endtime, "%Y-%m-%dT%H:%M:%S", &tm_);
							endtm = mktime(&tm_);
						}

						if (theApp.m_logmask & (1 << 5)) {
							Debug(INFO, "st=%d et=%d ;%d %d", sttm, endtm, m_recordStarttime, m_recordEndtime);
						}
						char stbuf[32] = { 0 };
						char edbuf[32] = { 0 };

						if (sttm < m_recordStarttime) {
							sttm = m_recordStarttime;
							time_t t = m_recordStarttime;
							struct tm* l = localtime(&t);
							strftime(stbuf, 32, "%Y-%m-%dT%H:%M:%S", l);
							sttimestr = stbuf;
						}

						if (endtm > m_recordEndtime) {
							endtm = m_recordEndtime;
							time_t t = m_recordEndtime;
							struct tm* l = localtime(&t);
							strftime(edbuf, 32, "%Y-%m-%dT%H:%M:%S", l);
							edtimestr = edbuf;
						}
						if (theApp.m_logmask & (1 << 5)) {
							Debug(INFO, "st=%s et=%s ;%d %d", stbuf, edbuf, m_recordStarttime, m_recordEndtime);
						}

						filesize = (endtm - sttm) * (512 / 8);
						unit = "KB";
					}
					char buf[10] = { 0 };
					sprintf(buf, "%d", filesize);

					Json::Value item;
					if (!sttimestr.empty()) {
						item["b"] = sttimestr;
					} else {
						item["b"] = starttime;
					}
					if (!edtimestr.empty()) {
						item["e"] = edtimestr;
					} else {
						item["e"] = endtime;
					}
					item["s"] = buf;
					item["u"] = unit;
					m_tmpRecordData["data"]["list"].append(item);
				}
			}
		}
	}

	if (m_tmpRecordData["data"]["list"].size() == sumnum) {
		sendResponse(m_tmpRecordData);
	}

}

