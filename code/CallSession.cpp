/*
 * CallSession.cpp
 *
 *  Created on: Mar 4, 2020
 *      Author: wyf
 */

#include "CallSession.h"
#include "common/json.h"

CallSession::CallSession() {
	// TODO Auto-generated constructor stub
	m_session_status = CALL_INIT;
	m_bitrate = -1;
	m_with_audio = -1;
	m_callid_srv = 0;
	m_callid_dev = -1;
	m_srv_did = 0;
	m_dev_did = -1;
	m_srv_call_id = "";
	m_http_port = 0;
	m_devid = "";
	m_nvrid = "";
	m_clientid = "";
	m_agentid = 0;
	m_session_status = CALL_INIT;
	m_ses_ms_ip = "";
	m_ses_ms_port = 0;
	m_ses_ms_did = "";
	m_ses_stat = 0;
	m_starttime = -1;
	m_endtime = -1;
	m_send_bye_tm = 0;
	m_invite_dev_port = 0;
	m_trans_type = 0;
	m_speed = 1;
	m_try_close_times = 0;

}

CallSession::~CallSession() {
// TODO Auto-generated destructor stub
}

string CallSession::toJsonStr() {
	Json::Value root;
	root["m_devid"] = m_devid;
	root["m_nvrid"] = m_nvrid;
	root["m_clientid"] = m_clientid;
	root["m_callid_srv"] = m_callid_srv;
	root["m_callid_dev"] = m_callid_dev;
	root["m_srv_did"] = m_srv_did;
	root["m_dev_did"] = m_dev_did;
	root["m_http_port"] = m_http_port;
	root["m_agentid"] = m_agentid;
	root["m_session_status"] = m_session_status;
	root["m_srv_call_id"] = m_srv_call_id;
	root["m_ses_ms_ip"] = m_ses_ms_ip;
	root["m_ses_ms_port"] = m_ses_ms_port;
	root["m_ses_ms_did"] = m_ses_ms_did;
	root["m_ses_stat"] = m_ses_stat;
	root["m_with_audio"] = m_with_audio;
	root["m_bitrate"] = m_bitrate;
	root["m_starttime"] = m_starttime;
	root["m_endtime"] = m_endtime;

	Json::FastWriter writer;
	string message = writer.write(root);

	return message;
}
void CallSession::parsefrom(string jsonstr) {
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(jsonstr, jmessage)) {
		return;
	}

	m_session_status = jmessage["m_session_status"].asInt();
	m_bitrate = jmessage["m_bitrate"].asInt();
	m_with_audio = jmessage["m_with_audio"].asInt();
	m_callid_srv = jmessage["m_callid_srv"].asInt();
	m_callid_dev = jmessage["m_callid_dev"].asInt();
	m_srv_did = jmessage["m_srv_did"].asInt();
	m_dev_did = jmessage["m_dev_did"].asInt();
	m_srv_call_id = jmessage["m_srv_call_id"].asString();
	m_http_port = jmessage["m_http_port"].asInt();
	m_devid = jmessage["m_devid"].asString();
	m_nvrid = jmessage["m_nvrid"].asString();
	m_clientid = jmessage["m_clientid"].asString();
	m_agentid = jmessage["m_agentid"].asInt();
	m_session_status = jmessage["m_session_status"].asInt();
	m_ses_ms_ip = jmessage["m_ses_ms_ip"].asString();
	m_ses_ms_port = jmessage["m_ses_ms_port"].asInt();
	m_ses_ms_did = jmessage["m_ses_ms_did"].asString();
	m_ses_stat = jmessage["m_ses_stat"].asInt();
	m_starttime = jmessage["m_starttime"].asInt();
	m_endtime = jmessage["m_endtime"].asInt();
}
