/*
 * CallSession.h
 *
 *  Created on: Mar 4, 2020
 *      Author: wyf
 */

#ifndef CALLSESSION_H_
#define CALLSESSION_H_

#include <eXosip2/eXosip.h>
#include <stdio.h>
#include <list>
#include <string>
#include "App.h"

using namespace std;

typedef enum {
	CALL_INIT, CALL_WAIT_SRV, CALL_WAIT_DEV, CALL_DEV_ACK, CALL_OK
};

class CallSession {
public:
	CallSession();
	virtual ~CallSession();
public:
	string toJsonStr();
	void parsefrom(string jsonstr);

	string m_devid;
	string m_nvrid;
	string m_clientid;

	int m_agentid;
	int m_closeagentid;
	int m_http_port;
	int m_session_status;

	int m_callid_srv;
	int m_callid_dev;
	int m_srv_did;
	int m_dev_did;

	string m_srv_call_id;
	string m_srv_call_tag;
	string m_srv_call_tag_to;
	string m_dev_call_id;
	string m_dev_call_tag;
	string m_dev_call_tag_to;

	int m_sip_cid_number;

	mediaserver m_msserver;
	string m_ses_ms_ip;
	int m_ses_ms_port;
	string m_ses_ms_did;

	int m_ses_stat; //  not use
	int m_with_audio;
	int m_bitrate;

	int m_starttime;
	int m_speed;
	int m_endtime;

	int m_createtime;
	int m_invite_dev_port;
	int m_send_bye_tm;
	int m_trans_type; //0 upd 1 tcp
	int m_try_close_times;

};

#endif /* CALLSESSION_H_ */
