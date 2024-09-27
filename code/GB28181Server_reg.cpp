/*
 * GB28181Server_reg.cpp
 *
 *  Created on: Mar 4, 2020
 *      Author: wyf
 */

#include "GB28181Server.h"

char *GB_NONCE = (char *) "6fe9ba44a76be22a";

void GB28181Server::Register401Unauthorized(struct eXosip_t * peCtx, eXosip_event_t *je) {

	int iRet = 0;
	osip_message_t * pSRegister = NULL;

	osip_www_authenticate_t * header = NULL;
	osip_www_authenticate_init(&header);

	if (header == NULL)
		return;

	osip_www_authenticate_set_auth_type(header, osip_strdup("Digest"));
	osip_www_authenticate_set_realm(header, osip_enquote(theApp.m_sipserver_realm.c_str()));
	osip_www_authenticate_set_nonce(header, osip_enquote(GB_NONCE)); // need modify

	char *pDest = NULL;
	osip_www_authenticate_to_str(header, &pDest);

	if (pDest == NULL)
		return;

	iRet = eXosip_message_build_answer(peCtx, je->tid, 401, &pSRegister);
	if (iRet == 0 && pSRegister != NULL) {
		osip_message_set_www_authenticate(pSRegister, pDest);
		osip_message_set_content_type(pSRegister, "Application/MANSCDP+xml");
		eXosip_lock(peCtx);
		eXosip_message_send_answer(peCtx, je->tid, 401, pSRegister);
		eXosip_unlock(peCtx);
	}

	osip_www_authenticate_free(header);
	osip_free(pDest);
}

void GB28181Server::RegisterSuccess(struct eXosip_t * peCtx, eXosip_event_t *je) {
	int iRet = 0;
	osip_message_t * pSRegister = NULL;
	iRet = eXosip_message_build_answer(peCtx, je->tid, 200, &pSRegister);

	char timeCh[32] = { 0 };

	struct timeval tv;
	struct timezone tz;

	struct tm *p;

	gettimeofday(&tv, &tz);
	p = localtime(&tv.tv_sec);

	sprintf(timeCh, "%d-%02d-%02dT%02d:%02d:%02d.%03d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
			tv.tv_usec / 1000);

	osip_message_set_topheader(pSRegister, "date", timeCh);

	if (iRet == 0 && pSRegister != NULL) {
		eXosip_lock(peCtx);
		eXosip_message_send_answer(peCtx, je->tid, 200, pSRegister);
		eXosip_unlock(peCtx);
	}
}

void GB28181Server::RegisterFailed(struct eXosip_t * peCtx, eXosip_event_t *je) {
	int iRet = 0;
	osip_message_t * pSRegister = NULL;
	iRet = eXosip_message_build_answer(peCtx, je->tid, 401, &pSRegister);
	if (iRet == 0 && pSRegister != NULL) {
		eXosip_lock(peCtx);
		eXosip_message_send_answer(peCtx, je->tid, 401, pSRegister);
		eXosip_unlock(peCtx);
	}
}
void GB28181Server::Register403Failed(struct eXosip_t * peCtx, eXosip_event_t *je) {
	int iRet = 0;
	osip_message_t * pSRegister = NULL;
	iRet = eXosip_message_build_answer(peCtx, je->tid, 403, &pSRegister);
	if (iRet == 0 && pSRegister != NULL) {
		eXosip_lock(peCtx);
		eXosip_message_send_answer(peCtx, je->tid, 403, pSRegister);
		eXosip_unlock(peCtx);
	}
}

void GB28181Server::onInterRegAuth(osip_message_t * msg, Json::Value& data) {

	string did = data["did"].asString();
	int ret = data["result"].asInt();

	if (ret == 1) {
		char timeCh[32] = { 0 };
		struct timeval tv;
		struct timezone tz;
		struct tm *p;

		gettimeofday(&tv, &tz);
		p = localtime(&tv.tv_sec);
		sprintf(timeCh, "%d-%02d-%02dT%02d:%02d:%02d.%03d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
				tv.tv_usec / 1000);

		DevIt it = m_map_devs.find(did);
		if (it == m_map_devs.end()) {
			return;
		}
		if (data["SN"].isString()) {
			it->second->m_uluSN = data["SN"].asString();
		}

		osip_message_t *reg_ok_ack_;
		if ( OSIP_SUCCESS == eXosip_message_build_answer(m_eCtx, m_map_devs[did]->m_reg_tid, 200, &reg_ok_ack_)) {
			osip_message_set_topheader(reg_ok_ack_, "date", timeCh);
			eXosip_message_send_answer(m_eCtx, m_map_devs[did]->m_reg_tid, 200, reg_ok_ack_);

			if (m_map_devs[did]->m_dev_status == STAT_NEW) {
				Json::Value root;
				root["devip"] = m_map_devs[did]->m_dev_remote_ip;
				root["devport"] = m_map_devs[did]->m_dev_remote_port;
				root["status"] = m_map_devs[did]->m_dev_status;
				root["updatetime"] = m_map_devs[did]->m_last_keep_time = m_map_devs[did]->m_last_update_time = time(NULL);
				root["devtype"] = m_map_devs[did]->m_dev_type = theApp.isDevNvr(did) ? 2 : 1;
				root["SN"] = m_map_devs[did]->m_uluSN;
				Json::FastWriter writer;
				string message = writer.write(root);
				printf("insert item %s\n", message.c_str());
				m_dev_local_cache.insertDev(did, message);
				m_dev_remote_cache.setDevItem(m_map_devs[did]);
				onDevStatChange(m_map_devs[did]->m_uluSN, m_map_devs[did]->m_dev_type, 1);

				Debug(ERROR, "dev reg OK:%s from %s %d", did.c_str(), m_map_devs[did]->m_dev_remote_ip.c_str(),
						m_map_devs[did]->m_dev_remote_port);
			}
			m_map_devs[did]->m_dev_status = STAT_REG_OK;
		}

		if (theApp.isDevNvr(did)) {
			m_map_devs[did]->m_querySN = theApp.m_SerailNumber++;
			SendQueryCatalog(m_eCtx, it->second, m_map_devs[did]->m_querySN);
			//	SendCatlogSubscribe(m_eCtx, it->second, it->second->m_querySN);

		}

	} else {
		DevIt it = m_map_devs.find(did);
		if (it == m_map_devs.end()) {
			Debug(ERROR, "no did for reg:%s", did.c_str());
			return;
		}
		osip_message_t *reg_fail_ack_;
		if ( OSIP_SUCCESS == eXosip_message_build_answer(m_eCtx, m_map_devs[did]->m_reg_tid, 500, &reg_fail_ack_)) {
			eXosip_message_send_answer(m_eCtx, m_map_devs[did]->m_reg_tid, 500, reg_fail_ack_);
			DevIt it = m_map_devs.find(did);
			if (it == m_map_devs.end()) {
				return;
			}
			m_map_devs[did]->m_dev_status = STAT_FAIL;
		}
		delDevFromMap(did);
		//redirect test
//		if ( OSIP_SUCCESS == eXosip_message_build_answer(m_eCtx, m_map_devs[did]->m_reg_tid, 301, &reg_fail_ack_)) {
//			osip_message_set_contact(reg_fail_ack_, "sip:192.168.2.196:15060");
//			eXosip_message_send_answer(m_eCtx, m_map_devs[did]->m_reg_tid, 301, reg_fail_ack_);
//			m_map_devs[did]->m_dev_status = STAT_FAIL;
//		}

	}

}

void GB28181Server::handleRegister(eXosip_event_t *je) {

	osip_authorization_t * Sdest = NULL;
	osip_message_get_authorization(je->request, 0, &Sdest);

	osip_from_t *hfrom = osip_message_get_from(je->request);
	char*pUsername = hfrom->url->username;
	if (strlen(pUsername) != 20) {
		Debug(ERROR, "err sip id=%s", pUsername);
		return;
	}
	osip_call_id_t * callid = osip_message_get_call_id(je->request);

	if (Sdest == NULL) {
		Register401Unauthorized(m_eCtx, je);
	} else {

		int expires = 3600;
		osip_header_t* hexp;
		osip_message_get_expires(je->request, 0, &hexp);
		if (hexp != NULL) {
			expires = atoi(hexp->hvalue);
		}

		if (expires == 0) {
			Debug(ERROR, "dev unregister id=%s ip=%s", pUsername, hfrom->url->host);

			DevIt it = this->m_map_devs.find(pUsername);
			if (it != m_map_devs.end()) {
				Debug(ERROR, "dev stat time out 1:%s", it->second->m_uluSN.c_str());
				int ret = m_dev_remote_cache.delDevItem((char*) it->second->m_uluSN.c_str());
				onDevStatChange(it->second->m_uluSN, it->second->m_dev_type, 0);
				{
					delDevFromMap(string(pUsername));
					m_dev_local_cache.deleteDevByDid(string(pUsername));
				}
			}
		} else {

			osip_via_t * s_via = NULL;
			osip_message_get_via(je->request, 0, &s_via);
			if (theApp.m_logmask & (1 << BIT_1_REG)) {
				Debug(ERROR, "dev register id=%s ip=%s ", pUsername, s_via->host);
			}
			char *remoteIpAddr = je->remote_ipaddr; //[64] = { 0 };
			int remotePort = je->remote_port;
			if (strcmp(remoteIpAddr, "127.0.0.1") == 0) {
				puts("inter auth");
				return;
			}

			//delDevFromMap(string(pUsername));

			CameraDevice* node;
			DevIt it = m_map_devs.find(pUsername);
			if (it == m_map_devs.end()) {
				Debug(ERROR, "dev stat: not found old id %s", pUsername);
				node = new CameraDevice();
				node->m_dev_status = STAT_NEW;
			} else {
				node = it->second;
				node->m_dev_status = STAT_REG_PENDING;

			}

			node->m_devid = pUsername;
			node->m_dev_remote_ip = remoteIpAddr;
			node->m_dev_remote_port = remotePort;
			node->m_reg_tid = je->tid;
			node->m_last_keep_time = time(NULL);
			node->m_last_update_time = 0;
			node->m_dev_type = 0;
			//node->m_uluSN = "-1";
			node->m_cseq = 30;

			Message* msg;
			if (it == m_map_devs.end()) {
				m_map_devs.insert(make_pair(pUsername, node));
				msg = new Message(sipmessage2str(je->request), MSG_SIP, CMD_REG);
			} else {
				msg = new Message(sipmessage2str(je->request), MSG_SIP, CMD_REG_REFRESH);
			}

			dispatchMessage(msg);

		}

	}

}
void GB28181Server::delDevFromMap(string did) {
	DevIt it = m_map_devs.find(did);
//	vector<string> channels;
	if (it != m_map_devs.end()) {
//		if (it->second->m_dev_type == 2) {
//			channels = it->second->m_channels;
//		}
		delete (it->second);
		Debug(ERROR, "del local stat %s", did.c_str());
		m_map_devs.erase(it);
	}

}
void GB28181Server::resetDev(string did) {

	Debug(ERROR, "reset dev by server:%s", did.c_str());
	DevIt it = this->m_map_devs.find(did);
	if (it != m_map_devs.end()) {
		int ret = m_dev_remote_cache.delDevItem((char*) it->second->m_uluSN.c_str());
		onDevStatChange(it->second->m_uluSN, it->second->m_dev_type, 0);

		{
			delDevFromMap(did);
			m_dev_local_cache.deleteDevByDid(did);
		}
	}

}

void GB28181Server::resetAllSession(string serverip) {

	SesIt iter;
	{
		iter = m_map_sessions.begin();
		while (iter != m_map_sessions.end()) {
			if (iter->second.m_msserver.m_streamserver_ip == serverip || iter->second.m_msserver.m_streamserver_sip_ip == serverip) {
				closeSession(iter->second.m_devid, 1, 2);
				m_map_sessions.erase(iter++);
			} else
				iter++;
		}

	}
	{

		iter = m_map_playback_sessions.begin();
		while (iter != m_map_playback_sessions.end()) {
			if (iter->second.m_msserver.m_streamserver_ip == serverip || iter->second.m_msserver.m_streamserver_sip_ip == serverip) {
				closeSession(iter->second.m_devid, 2, 2);
				m_map_playback_sessions.erase(iter++);
			} else
				iter++;
		}

	}
	{
		iter = m_map_download_sessions.begin();
		while (iter != m_map_download_sessions.end()) {
			if (iter->second.m_msserver.m_streamserver_ip == serverip || iter->second.m_msserver.m_streamserver_sip_ip == serverip) {
				closeSession(iter->second.m_devid, 3, 2);
				m_map_download_sessions.erase(iter++);
			} else
				iter++;
		}

	}

}

