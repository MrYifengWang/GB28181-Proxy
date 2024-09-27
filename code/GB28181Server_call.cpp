/*
 * GB28181Server_call.cpp
 *
 *  Created on: Mar 4, 2020
 *      Author: wyf
 */
#include "GB28181Server.h"
#include "common/base64.h"
//#define DEBUG_BYE 1

int my_osip_message_set_cseq(osip_message_t * sip, const char *hvalue) {
	int i;

	if (hvalue == NULL || hvalue[0] == '\0')
		return OSIP_SUCCESS;

	if (sip->cseq != NULL) {
		osip_cseq_free(sip->cseq);
		sip->cseq = NULL;
	}

	//return OSIP_BADPARAMETER;
	i = osip_cseq_init(&(sip->cseq));
	if (i != 0)
		return i;

	sip->message_property = 2;
	i = osip_cseq_parse(sip->cseq, hvalue);
	if (i != 0) {
		osip_cseq_free(sip->cseq);
		sip->cseq = NULL;
		return i;
	}

	return OSIP_SUCCESS;
}
int my_osip_call_id_parse(osip_call_id_t * callid, const char *hvalue) {
	{
		const char *host;
		const char *end;

		callid->number = NULL;
		callid->host = NULL;

		host = strchr(hvalue, '@'); /* SEARCH FOR '@' */
		end = hvalue + strlen(hvalue);

		if (host == NULL)
			host = end;
		else {
			if (end - host + 1 < 2)
				return OSIP_SYNTAXERROR;
			callid->host = (char *) osip_malloc(end - host);
			if (callid->host == NULL)
				return OSIP_NOMEM;
			osip_clrncpy(callid->host, host + 1, end - host - 1);

			puts(callid->host);
		}
		if (host - hvalue + 1 < 2)
			return OSIP_SYNTAXERROR;
		callid->number = (char *) osip_malloc(host - hvalue + 1);
		if (callid->number == NULL)
			return OSIP_NOMEM;
		osip_clrncpy(callid->number, hvalue, host - hvalue);
		puts(callid->number);

		return OSIP_SUCCESS; /* ok */
	}

}
int GB28181Server::closeSession(string devid, int sesType, int closetype) {
	CallSession* pses = getSession(devid, sesType);

	if (pses == NULL) {
		Debug(ERROR, "ses not found when close:%s", devid.c_str());
		return -1;
	}
	pses->m_try_close_times++;
	pses->m_send_bye_tm = time(NULL);

	DevIt it;
	//if (sesType == TYPE_PLAYBACK || sesType == TYPE_DOWNLOAD) {
	it = m_map_devs.find(pses->m_nvrid);
	//}
//	else {
//		it = m_map_devs.find(devid);
//	}
	if (it == m_map_devs.end()) {
		Debug(ERROR, "dev offline when close:%s", devid.c_str());
		return -1;
	}

	if (!pses->m_msserver.m_streamserver_ip.empty()) {
		theApp.updateServerLoad(-1, pses->m_msserver.m_streamserver_ip);
	}

	int step = 0;
	bool forceclose = false;

	if (pses->m_callid_dev != -1 && pses->m_dev_did != -1) {

		step = 1;
		Debug(INFO, "send BYE to dev:%d %s %d", pses->m_callid_dev, devid.c_str(), sesType);
		if (pses->m_invite_dev_port == it->second->m_dev_remote_port) {
			eXosip_call_terminate(m_eCtx, pses->m_callid_dev, pses->m_dev_did);
		} else {

			forceclose = true;
			osip_message_t *message = NULL;
			char dest_call[256] = { 0 };
			char source_call[256] = { 0 };

			if (!pses->m_dev_call_tag_to.empty()) {
				snprintf(dest_call, 256, "sip:%s@%s:%d;tag=%s", devid.c_str(), it->second->m_dev_remote_ip.c_str(),
						it->second->m_dev_remote_port, pses->m_dev_call_tag_to.c_str());
			} else {
				snprintf(dest_call, 256, "sip:%s@%s:%d", devid.c_str(), it->second->m_dev_remote_ip.c_str(), it->second->m_dev_remote_port);
			}
			if (!pses->m_dev_call_tag.empty()) {
				snprintf(source_call, 256, "sip:%s@%s:%d;tag=%s", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(),
						theApp.m_sipserver_port, pses->m_dev_call_tag.c_str());
			} else {
				snprintf(source_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(),
						theApp.m_sipserver_port);
			}
			Debug(INFO, "BYE: %s %s", dest_call, source_call);
			if ( OSIP_SUCCESS == eXosip_message_build_request(m_eCtx, &message, "BYE", dest_call, source_call, NULL)) {

				my_osip_call_id_parse(message->call_id, pses->m_dev_call_id.c_str());

				printf("server did = %s callid = %s  %s  %s\n", pses->m_devid.c_str(), pses->m_dev_call_id.c_str(),
						message->call_id->number, message->call_id->host);

				char buf[20] = { 0 };
				sprintf(buf, "%d BYE", it->second->m_cseq++);
				my_osip_message_set_cseq(message, buf);

				eXosip_message_send_request(m_eCtx, message);
			}
			eXosip_call_terminate(m_eCtx, pses->m_callid_dev, pses->m_dev_did);
		}

	} else {
		Debug(INFO, "send BYE error :%d %s %d", pses->m_callid_dev, devid.c_str(), sesType);
		if (sesType == TYPE_LIVE) {
			resetDev(devid);
		} else if (sesType == TYPE_PLAYBACK) {
			string nvrid = pses->m_nvrid;
			if (!nvrid.empty()) {
				Debug(ERROR, "reset nvr 2:%s channelid:%s", nvrid.c_str(), devid.c_str());
				resetDev(nvrid);
			}
		} else {
			string nvrid = pses->m_nvrid;
			if (!nvrid.empty() && devid != nvrid) {
				Debug(ERROR, "reset nvr 3:%s channelid:%s", nvrid.c_str(), devid.c_str());
				resetDev(nvrid);
			}

		}
	}

	//sed bye to server
	int servflag = 0;
	if (closetype != 2) {
		servflag = 1;

		eXosip_call_terminate(m_eCtx, pses->m_callid_srv, pses->m_srv_did);
		osip_message_t *message = NULL;
		char dest_call[256] = { 0 };
		char source_call[256] = { 0 };

		snprintf(dest_call, 256, "sip:%s@%s:%d", pses->m_msserver.m_streamserver_id.c_str(), pses->m_msserver.m_streamserver_sip_ip.c_str(),
				pses->m_msserver.m_streamserver_sip_port);
		snprintf(source_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(), theApp.m_sipserver_port);

		if ( OSIP_SUCCESS == eXosip_message_build_request(m_eCtx, &message, "MESSAGE", dest_call, source_call, NULL)) {

//			Debug(ERROR, "send Bye to server:%s", dest_call);

			Json::Value root;
			string playtype;
			root["did"] = devid;
			root["type"] = sesType;
			Json::FastWriter writer;
			string params = writer.write(root);
			osip_message_set_content_type(message, "application/ULU+json");

			unsigned char base64Body[1024] = { 0 };
			int len = b64_encode((unsigned char*) params.c_str(), params.length(), base64Body);
			puts((char*) base64Body);
			osip_message_set_body(message, (char*) base64Body, len);
			eXosip_message_send_request(m_eCtx, message);
		}
	}

	if ((servflag == 1 && step == 0) || forceclose == true) {
		Debug(INFO, "del session %s type=%d\n", devid.c_str(), sesType);
		delSession(devid, sesType);
		m_dev_local_cache.deleteSesByDid(sesType, devid);
		return -1;
	}
	return 0;
}

void GB28181Server::handleCallMsgResponse(struct eXosip_t * peCtx, eXosip_event_t *je) {

	char *remoteIpAddr = je->remote_ipaddr; //[64] = { 0 };
	int remotePort = je->remote_port;
	if (strcmp(remoteIpAddr, "127.0.0.1") == 0 || theApp.isServerIp(remoteIpAddr)) {
		return;
	}

	osip_to_t *hto = osip_message_get_to(je->request);
	osip_cseq_t* hseq = osip_message_get_cseq(je->request);
	if (hseq != NULL) {
		if (strcmp(hseq->method, "BYE") == 0) {
			osip_call_id_t * callid = osip_message_get_call_id(je->request);
			if (callid != NULL) {
				Debug(INFO, "BYE Response from OK:%s  %s", hto->url->username, callid->number);
				delSessionBycid(hto->url->username, atoi(callid->number));
			}
		} else {
			Debug(ERROR, "other call msg:%s", sipmessage2str(je->request).c_str());;
		}
	}

}

void GB28181Server::onInterClosePlay(osip_message_t * msg, Json::Value& data) {

	string did = data["did"].asString();
	string clientid = data["clientid"].asString();
	int type = data["live"].asInt();

	osip_from_t *from = osip_message_get_from(msg);
	char* strid = from->url->username;

	CallSession* pses = getSession(did, type);
	if (pses == NULL) {
		Json::Value root;
		root["cmd"] = "call_close_ack";
		root["type"] = type;
		root["did"] = did;
		root["nvrid"] = ""; //pses->m_nvrid;
		root["result"] = "closed";
		root["reason"] = "session already closed!";

		sendInterJsonMessage(root, strid);
		return;
	} else {

		pses->m_closeagentid = atoi(strid);

		if (type == TYPE_LIVE) {
			if (clientid != "0") {
				Debug(ERROR, "close live by client:%s dev:%s", pses->m_clientid.c_str(), did.c_str());
			}
			bool canclose = false;
			if (pses->m_http_port == 0 || pses->m_session_status != CALL_OK) {
				int closetime = 10;
				if (time(NULL) - pses->m_createtime > closetime) {
					canclose = true;
				}
			} else {
				canclose = true;
			}
			if (canclose) {
				closeSession(did, type);
			} else {
				Json::Value root;
				root["cmd"] = "call_close_ack";
				root["type"] = type;
				root["did"] = did;
				root["nvrid"] = ""; //pses->m_nvrid;
				root["result"] = "busy";
				root["reason"] = "last call not complete!";

				sendInterJsonMessage(root, strid);
				return;
			}
		} else if (type == TYPE_PLAYBACK) {
			if (pses->m_clientid == clientid || clientid == "0") {

				bool canclose = false;
				if (pses->m_http_port == 0 || pses->m_session_status != CALL_OK) {
					int closetime = 20;
					if (clientid == "0") {
						closetime = 90;
					}
					if (time(NULL) - pses->m_createtime > closetime) {
						canclose = true;
					}
				} else {
					canclose = true;
				}

				if (canclose) {
					Debug(INFO, " close playback session 1:%s", did.c_str());

					if (-1 == closeSession(did, type)) {

						Debug(INFO, " close playback session fail:%s", did.c_str());
						Json::Value root;
						root["cmd"] = "call_close_ack";
						root["type"] = type;
						root["did"] = did;
						root["nvrid"] = ""; //pses->m_nvrid;
						root["result"] = "closed";
						root["reason"] = "playback session already closed!";

						sendInterJsonMessage(root, strid);
						return;
					}
				} else {
					Json::Value root;
					root["cmd"] = "call_close_ack";
					root["type"] = type;
					root["did"] = did;
					root["nvrid"] = ""; //pses->m_nvrid;
					root["result"] = "busy";
					root["reason"] = "last call not complete!";

					sendInterJsonMessage(root, strid);
					return;
				}
			} else if (clientid != "0") {

				bool canclose = false;
				if (pses->m_http_port == 0 || pses->m_session_status != CALL_OK) {
					int closetime = 30;

					if (time(NULL) - pses->m_createtime > closetime) {
						canclose = true;
					}
				}
				if (canclose) {
					Debug(INFO, " close playback session 2:%s", did.c_str());

					if (-1 == closeSession(did, type)) {

						Debug(INFO, " close playback session fail:%s", did.c_str());
						Json::Value root;
						root["cmd"] = "call_close_ack";
						root["type"] = type;
						root["did"] = did;
						root["nvrid"] = ""; //pses->m_nvrid;
						root["result"] = "closed";
						root["reason"] = "playback session already closed!";

						sendInterJsonMessage(root, strid);
						return;
					}
				} else {
					Json::Value root;
					root["cmd"] = "call_close_ack";
					root["type"] = type;
					root["did"] = did;
					root["nvrid"] = pses->m_nvrid;
					root["result"] = "busy";
					char buf[256] = { 0 };
					sprintf(buf, "this is client %s's session", pses->m_clientid.c_str());
					root["reason"] = buf;

					sendInterJsonMessage(root, strid);
					return;
				}

			} else if (clientid == "0") {
				Debug(INFO, " close playback session 3:%s", did.c_str());

				closeSession(did, type);
			}

		} else if (type == TYPE_DOWNLOAD) {

			if (pses->m_clientid == clientid || clientid == "0") {

				bool canclose = false;
				if (pses->m_http_port == 0 || pses->m_session_status != CALL_OK) {
					int closetime = 20;
					if (clientid == "0") {
						closetime = 90;
					}
					if (time(NULL) - pses->m_createtime > closetime) {
						canclose = true;
					}
				} else {
					canclose = true;
				}

				if (canclose) {
					Debug(ERROR, " close download session 1:%s", did.c_str());

					if (-1 == closeSession(did, type)) {

						Debug(ERROR, " close download session fail:%s", did.c_str());
						Json::Value root;
						root["cmd"] = "call_close_ack";
						root["type"] = type;
						root["did"] = did;
						root["nvrid"] = ""; //pses->m_nvrid;
						root["result"] = "closed";
						root["reason"] = "download session already closed!";

						sendInterJsonMessage(root, strid);
						return;
					}
					if (pses->m_try_close_times >= 3) {
						delSession(did, 3);
						m_dev_local_cache.deleteSesByDid(3, did);
					}
				} else {
					Json::Value root;
					root["cmd"] = "call_close_ack";
					root["type"] = type;
					root["did"] = did;
					root["nvrid"] = ""; //pses->m_nvrid;
					root["result"] = "busy";
					root["reason"] = "last call not complete!";

					sendInterJsonMessage(root, strid);
					return;
				}
			} else if (clientid != "0") {

				bool canclose = false;
				if (pses->m_http_port == 0 || pses->m_session_status != CALL_OK) {
					int closetime = 30;

					if (time(NULL) - pses->m_createtime > closetime) {
						canclose = true;
					}
				}

				if (canclose) {
					Debug(ERROR, " close download session 2:%s", did.c_str());

					if (-1 == closeSession(did, type)) {

						Debug(ERROR, " close download session fail:%s", did.c_str());
						Json::Value root;
						root["cmd"] = "call_close_ack";
						root["type"] = type;
						root["did"] = did;
						root["nvrid"] = ""; //pses->m_nvrid;
						root["result"] = "closed";
						root["reason"] = "download session already closed!";

						sendInterJsonMessage(root, strid);
						return;
					}
				} else {
					Json::Value root;
					root["cmd"] = "call_close_ack";
					root["type"] = type;
					root["did"] = did;
					root["nvrid"] = pses->m_nvrid;
					root["result"] = "busy";
					char buf[256] = { 0 };
					sprintf(buf, "this is client %s's session", pses->m_clientid.c_str());
					root["reason"] = buf;

					sendInterJsonMessage(root, strid);
				}

			} else if (clientid == "0") {
				Debug(ERROR, " close download session 3:%s", did.c_str());

				closeSession(did, type);
			}

		}
	}
	return;
}

void GB28181Server::onInterGetDevInfo(osip_message_t * msg, Json::Value& data) {
	osip_from_t *from = osip_message_get_from(msg);
	char* strid = from->url->username;

	string did = data["did"].asString();
	string clientid = data["clientid"].asString();

	DevIt it = this->m_map_devs.find(did);
	if (it != m_map_devs.end()) {
		SendQueryDeviceInfo(m_eCtx, it->second, strid);
	}
}
void GB28181Server::onInterRemoteQuery(osip_message_t * msg, Json::Value& message) {
	osip_from_t *from = osip_message_get_from(msg);
	char* strid = from->url->username;

	string subcmd = message["data"]["subcmd"].asString();
	int aid = message["aid"].asInt();

	int infocount = 0;
	string response;
	if (subcmd == "get_device_stat_detail_req") {
		string did = message["data"]["did"].asString();

		Json::Value root;
		root["cmd"] = "query_from_gbserver_ack";
		root["aid"] = aid;
		root["serial"] = message["message"].asString();

		Json::Value data;
		data["subcmd"] = "get_device_stat_detail_req";
		data["did"] = did;

		printf("query did=%s---\n", did.c_str());
		DevIt it = this->m_map_devs.find(did);
		if (it != m_map_devs.end()) {
			infocount++;
			data["stat"] = "online";
			data["ulu_sn"] = it->second->m_uluSN;
			data["remote_ip"] = it->second->m_dev_remote_ip;
			data["remote_port"] = it->second->m_dev_remote_port;
			data["dev_type"] = it->second->m_dev_type;
			//	data["workstat"] = it->second->m_dev_status;
			data["lastkeep_tm"] = utc2strtime(it->second->m_last_keep_time);
		} else {
			if (theApp.isChannelID(did)) {
				int stat = 0;
				CameraDevice* pdev = findChannel(did, stat);
				if (pdev != NULL) {
					infocount++;
					if (stat == 0) {
						data["stat"] = "offline";
					} else {
						data["stat"] = "online";
					}
					data["ulu_sn"] = pdev->m_uluSN;
					data["remote_ip"] = pdev->m_dev_remote_ip;
					data["remote_port"] = pdev->m_dev_remote_port;
					data["dev_type"] = 3;
					data["nvrid"] = pdev->m_devid;
					data["lastkeep_tm"] = utc2strtime(pdev->m_last_keep_time);
				} else {
					data["stat"] = "offline";
				}
			} else {
				data["stat"] = "offline";
			}
		}
		{
			CallSession* pses = getSession(did, TYPE_LIVE);
			if (pses != NULL) {
				infocount++;
				Json::Value session;
				session["streamserverip"] = pses->m_msserver.m_streamserver_ip;
				session["did"] = pses->m_devid;
				session["nvrid"] = pses->m_nvrid;
				session["transtype"] = pses->m_trans_type;
				session["session_starttime"] = utc2strtime(pses->m_createtime);

				data["livesession"] = session;
			}
		}
		{
			CallSession* pses = getSession(did, TYPE_PLAYBACK);
			if (pses != NULL) {
				infocount++;
				Json::Value session;
				session["streamserverip"] = pses->m_msserver.m_streamserver_ip;
				session["did"] = pses->m_devid;
				session["nvrid"] = pses->m_nvrid;
				session["transtype"] = pses->m_trans_type;
				session["starttime"] = utc2strtime(pses->m_starttime);
				session["endtime"] = utc2strtime(pses->m_endtime);
				session["speed"] = pses->m_speed;
				session["session_starttime"] = utc2strtime(pses->m_createtime);
				session["clientid"] = pses->m_clientid;

				data["playback_session"] = session;
			}
		}
		{
			CallSession* pses = getSession(did, TYPE_DOWNLOAD);
			if (pses != NULL) {
				infocount++;
				Json::Value session;
				session["streamserverip"] = pses->m_msserver.m_streamserver_ip;
				session["did"] = pses->m_devid;
				session["nvrid"] = pses->m_nvrid;
				session["transtype"] = utc2strtime(pses->m_trans_type);
				session["starttime"] = utc2strtime(pses->m_starttime);
				session["endtime"] = pses->m_endtime;
				session["speed"] = pses->m_speed;
				session["session_starttime"] = utc2strtime(pses->m_createtime);
				session["clientid"] = pses->m_clientid;

				data["download_session"] = session;
			}
		}

		root["data"] = data;

		Json::FastWriter writer;
		response = writer.write(root);

		puts(response.c_str());

	}

	if (infocount > 0) {
		if (m_local_com_socket == 0) {
			m_local_com_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (m_local_com_socket < 0)
				return;
		}
		struct sockaddr_in peerAddr;
		peerAddr.sin_family = AF_INET;
		peerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		peerAddr.sin_port = htons(theApp.m_localCenterUdpPort);
		int ret = sendto(m_local_com_socket, response.c_str(), response.length(), 0, (struct sockaddr*) &peerAddr, sizeof(sockaddr_in));
	}
}

void GB28181Server::onInterCall(osip_message_t * msg, Json::Value& data) {

	osip_from_t *from = osip_message_get_from(msg);
	char* strid = from->url->username;

	string clientid = data["clientid"].asString();

	int transtype = 0;
	if (data["transtype"].isInt()) {
		transtype = data["transtype"].asInt();
	}
//	transtype = 1;
	SesIt it;

	if (data["live"].isObject()) {
		string did = data["did"].asString();
		string nvrid;
		if (data["nvrid"].isString()) {
			string nid = data["nvrid"].asString();
			if (nid.length() == 20) {
				nvrid = nid;
			}
		}

		DevIt dit;
		if (!nvrid.empty()) {
			dit = m_map_devs.find(nvrid);
		} else {
			dit = m_map_devs.find(did);
		}

		if (dit == m_map_devs.end()) {
			Json::Value root;
			root["cmd"] = "call_invite_ack";
			root["type"] = TYPE_LIVE;
			root["did"] = did;
			root["result"] = "offline";

			sendInterJsonMessage(root, strid);
			return;
		} else {

		}
		it = m_map_sessions.find(did);
		if (it != m_map_sessions.end()) {

			if (!nvrid.empty()) {
				it->second.m_nvrid = nvrid;
			} else {
				it->second.m_nvrid = did;
			}
			it->second.m_devid = did;
			it->second.m_agentid = atoi(strid);
			it->second.m_clientid = clientid;
			it->second.m_bitrate = data["live"]["rate"].asInt();
			it->second.m_with_audio = data["live"]["audio"].asInt();
			it->second.m_trans_type = transtype;
			it->second.m_createtime = time(NULL);
			it->second.m_sip_cid_number = 0;
			it->second.m_callid_srv = SendInvitePlay2Srv(this->m_eCtx, clientid, &it->second, 1, did, true);
			Debug(INFO, "realplay:did=%s %s cid=%d", did.c_str(), it->second.m_nvrid.c_str(), it->second.m_callid_srv);
		} else {
			CallSession session;
			if (!nvrid.empty()) {
				session.m_nvrid = nvrid;
			} else {
				session.m_nvrid = did;
			}
			session.m_devid = did;
			session.m_agentid = atoi(strid);
			session.m_clientid = clientid;
			session.m_bitrate = data["live"]["rate"].asInt();
			session.m_with_audio = data["live"]["audio"].asInt();
			session.m_trans_type = transtype;
			session.m_createtime = time(NULL);
			session.m_sip_cid_number = 0;
			session.m_callid_srv = SendInvitePlay2Srv(this->m_eCtx, clientid, &session, 1, did);
			Debug(INFO, "realplay new session:did=%s cid=%d", did.c_str(), session.m_callid_srv);
			m_map_sessions[did] = session;

		}
	} else if (data["playback"].isObject()) {
		string did = data["did"].asString();
		string nvrid = data["nvrid"].asString();
		if (nvrid.empty() || nvrid.length() < 20) {
			nvrid = did;
		}

		DevIt dit = m_map_devs.find(nvrid);
		if (dit == m_map_devs.end()) {
			Json::Value root;
			root["cmd"] = "call_invite_ack";
			root["type"] = TYPE_PLAYBACK;
			root["did"] = did;
			root["nvrid"] = nvrid;
			root["result"] = "offline";

			sendInterJsonMessage(root, strid);
			Debug(ERROR, "playback a offline dev:%s", did.c_str());

			return;
		} else {
			if (did != nvrid) {
				int channelstat = dit->second->findChannelStat(did);
				Debug(INFO, "channel stat:did=%s nvrid=%s stat=%d\n", did.c_str(), nvrid.c_str(), channelstat);

				//	if (channelstat < 1) {
//				Json::Value root;
//				root["cmd"] = "call_invite_ack";
//				root["type"] = TYPE_PLAYBACK;
//				root["did"] = did;
//				root["nvrid"] = nvrid;
//				root["result"] = "channel offline";
//
//				sendInterJsonMessage(root, strid);
//				Debug(ERROR, "playback a offline channel %s:%s %d", nvrid.c_str(), did.c_str(), channelstat);
//
//				return;
				//}
			}

			int sttm = atoi(data["playback"]["starttime"].asString().c_str());
			int edtm = atoi(data["playback"]["endtime"].asString().c_str());

			if (edtm - sttm < 5) {
				Json::Value root;
				root["cmd"] = "call_invite_ack";
				root["type"] = TYPE_PLAYBACK;
				root["did"] = did;
				root["nvrid"] = nvrid;
				root["result"] = "wrong playback time";

				sendInterJsonMessage(root, strid);
				Debug(ERROR, "playback : wrong time:%s", did.c_str());

				return;
			}

			printf("open play back %s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());

			CallSession* pses = getSession(did, TYPE_PLAYBACK);
			if (pses != NULL) {
				if (pses->m_send_bye_tm != 0 && time(NULL) - pses->m_send_bye_tm > 60) {
					CallSession session;
					session.m_devid = did;
					session.m_nvrid = nvrid;
					session.m_agentid = atoi(strid);
					session.m_clientid = clientid;
					session.m_starttime = atoi(data["playback"]["starttime"].asString().c_str());
					session.m_endtime = atoi(data["playback"]["endtime"].asString().c_str());
					session.m_callid_srv = SendInvitePlay2Srv(this->m_eCtx, clientid, &session, 2, did);
					session.m_createtime = time(NULL);
					session.m_trans_type = transtype;
					session.m_sip_cid_number = 0;

					m_map_playback_sessions[did] = session;
					Debug(INFO, "replace playback session:%s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());
				} else {
					if (pses->m_clientid != clientid) {
						Debug(ERROR, "open playback %s %s olduser:%s newuser:%s\n", nvrid.c_str(), did.c_str(), pses->m_clientid.c_str(),
								clientid.c_str());

						Json::Value root;
						root["cmd"] = "call_invite_ack";
						root["type"] = TYPE_PLAYBACK;
						root["did"] = did;
						root["nvrid"] = nvrid;
						root["result"] = "busy";

						puts("playback in use");
						Debug(INFO, "playback in use:%s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());

						sendInterJsonMessage(root, strid);
						return;
					} else {
						if (pses->m_http_port == 0) {
							Json::Value root;
							root["cmd"] = "call_invite_ack";
							root["type"] = TYPE_PLAYBACK;
							root["did"] = did;
							root["nvrid"] = nvrid;
							root["result"] = "busy";

							puts("playback in use");
							Debug(ERROR, "playback in use:%s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());

							sendInterJsonMessage(root, strid);
							return;
						} else {
							Json::Value root;
							root["cmd"] = "call_invite_ack";
							root["result"] = "ok";
							root["did"] = did;
							root["server"] = pses->m_msserver.m_streamserver_ip;
							root["type"] = TYPE_PLAYBACK;
							root["http_port"] = pses->m_http_port;
							root["http_ip"] = pses->m_msserver.m_streamserver_ip;

							char buf[256] = { 0 };
							root["nvrid"] = pses->m_nvrid.c_str();
							sprintf(buf, "rtmp://%s/playback/%s", pses->m_msserver.m_streamserver_ip.c_str(), did.c_str());
							root["url"] = buf;

							sendInterJsonMessage(root, strid);
						}
					}
				}
			} else {
				CallSession session;
				session.m_devid = did;
				session.m_nvrid = nvrid;
				session.m_agentid = atoi(strid);
				session.m_clientid = clientid;
				session.m_starttime = atoi(data["playback"]["starttime"].asString().c_str());
				session.m_endtime = atoi(data["playback"]["endtime"].asString().c_str());
				session.m_callid_srv = SendInvitePlay2Srv(this->m_eCtx, clientid, &session, 2, did);
				session.m_createtime = time(NULL);
				session.m_trans_type = transtype;
				session.m_sip_cid_number = 0;

				m_map_playback_sessions[did] = session;
				Debug(ERROR, "add playback session:%s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());

			}
		}

	} else if (data["download"].isObject()) {

		string did = data["did"].asString();
		string nvrid = data["nvrid"].asString();
		if (nvrid.empty() || nvrid.length() < 20) {
			nvrid = did;
		}

		DevIt dit = m_map_devs.find(nvrid);
		if (dit == m_map_devs.end()) {
			Json::Value root;
			root["cmd"] = "call_invite_ack";
			root["type"] = TYPE_DOWNLOAD;
			root["did"] = did;
			root["nvrid"] = nvrid;
			root["result"] = "offline";

			sendInterJsonMessage(root, strid);
			Debug(ERROR, "download a offline dev:%s nvrid=%s", did.c_str(), nvrid.c_str());

			return;
		} else {

			printf("open download %s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());

			CallSession* pses = getSession(did, TYPE_DOWNLOAD);
			if (pses != NULL) {
				if (pses->m_send_bye_tm != 0 && time(NULL) - pses->m_send_bye_tm > 60) {
					CallSession session;
					session.m_devid = did;
					session.m_nvrid = nvrid;
					session.m_agentid = atoi(strid);
					session.m_clientid = clientid;
					session.m_starttime = atoi(data["download"]["starttime"].asString().c_str());
					session.m_endtime = atoi(data["download"]["endtime"].asString().c_str());
					session.m_speed = data["download"]["speed"].asInt();
					if (session.m_speed > 4 || session.m_speed < 1)
						session.m_speed = 4;
					session.m_callid_srv = SendInvitePlay2Srv(this->m_eCtx, clientid, &session, 2, did);
					session.m_createtime = time(NULL);
					session.m_trans_type = transtype;
					session.m_sip_cid_number = 0;

					m_map_download_sessions[did] = session;
					Debug(ERROR, "replace download session:%s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());
				} else {
					if (pses->m_clientid != clientid) {
						Debug(ERROR, "open download fail: %s %s olduser:%s newuser:%s\n", nvrid.c_str(), did.c_str(),
								pses->m_clientid.c_str(), clientid.c_str());

						Json::Value root;
						root["cmd"] = "call_invite_ack";
						root["type"] = TYPE_DOWNLOAD;
						root["did"] = did;
						root["nvrid"] = nvrid;
						root["result"] = "busy";

						puts("download in use");
						Debug(ERROR, "download in use:%s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());

						sendInterJsonMessage(root, strid);
						return;
					} else {
						if (pses->m_http_port == 0) {
							Json::Value root;
							root["cmd"] = "call_invite_ack";
							root["type"] = TYPE_DOWNLOAD;
							root["did"] = did;
							root["nvrid"] = nvrid;
							root["result"] = "busy";

							puts("download in use");
							Debug(ERROR, "download in use:%s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());

							sendInterJsonMessage(root, strid);
							return;
						} else {
							Json::Value root;
							root["cmd"] = "call_invite_ack";
							root["result"] = "ok";
							root["did"] = did;
							root["server"] = pses->m_msserver.m_streamserver_ip;
							root["type"] = TYPE_DOWNLOAD;
							root["http_port"] = pses->m_http_port;
							root["http_ip"] = pses->m_msserver.m_streamserver_ip;

							char buf[256] = { 0 };
							root["nvrid"] = pses->m_nvrid.c_str();
							sprintf(buf, "rtmp://%s/download/%s", pses->m_msserver.m_streamserver_ip.c_str(), did.c_str());
							root["url"] = buf;

							sendInterJsonMessage(root, strid);
						}
					}
				}
			} else {
				CallSession session;
				session.m_devid = did;
				session.m_nvrid = nvrid;
				session.m_agentid = atoi(strid);
				session.m_clientid = clientid;
				session.m_starttime = atoi(data["download"]["starttime"].asString().c_str());
				session.m_endtime = atoi(data["download"]["endtime"].asString().c_str());
				session.m_speed = data["download"]["speed"].asInt();
				if (session.m_speed > 4 || session.m_speed < 1)
					session.m_speed = 4;
				session.m_callid_srv = SendInvitePlay2Srv(this->m_eCtx, clientid, &session, 3, did);
				session.m_createtime = time(NULL);
				session.m_trans_type = transtype;
				session.m_sip_cid_number = 0;

				m_map_download_sessions[did] = session;
				Debug(INFO, "add download session:%s %s %s\n", nvrid.c_str(), did.c_str(), clientid.c_str());

			}
		}

	}

}

bool GB28181Server::checkSrvAnswer(char* sdp, string& keepaliveport) {
	std::vector<std::string> itemList;
	split(string(sdp), "\r\n", &itemList);

	bool exist = false;
	bool start = false;
	int pos;
	for (unsigned int i = 0; i < itemList.size(); i++) {
		string item = itemList[i];
		//	puts(item.c_str());
		if ((pos = item.find("exist=")) != string::npos) {
			string val = item.substr(6);
			printf("exist=%s", val.c_str());
			if (atoi(val.c_str()) == 1) {
				exist = true;
			}
		}
		if ((pos = item.find("http_port=")) != string::npos) {
			string val = item.substr(10);
			printf("http_port=%s", val.c_str());
			keepaliveport = val;
		}

		if ((pos = item.find("mediaStart=")) != string::npos) {
			string val = item.substr(11);
			printf("mediaStart=%s\n", val.c_str());
			if (atoi(val.c_str()) == 1) {
				start = true;
			}
		}
	}

	Debug(INFO, "stream stat:exist=%d & start=%d\n", exist, start);

	return (exist && start);
}
void GB28181Server::handleRequestFail(eXosip_event_t *je) {
	if (je->request == NULL)
		return;

	osip_to_t *hto = osip_message_get_to(je->request);
	osip_from_t *hfrom = osip_message_get_from(je->request);
	int cid = je->cid;

	Debug(ERROR, "dev ack :call fail: host=%s from=%s toid=%s", hfrom->url->host, hfrom->url->username, hto->url->username);
	printf("call fail: host=%s %s %s toid=%s\n", hfrom->url->host, hfrom->url->scheme, hfrom->url->username, hto->url->username);

	string did = hto->url->username;
	int sesType = 0;
	string nvrid;
	CallSession* pses = getSessionByCid(did, 2, cid, sesType);

	if (pses != NULL) {

		Json::Value root;
		root["cmd"] = "call_invite_ack";
		root["result"] = "devfail";
		root["did"] = did;
		root["server"] = pses->m_msserver.m_streamserver_ip;
		root["type"] = sesType;

		if (sesType == TYPE_LIVE) {
		} else if (sesType == TYPE_PLAYBACK) {
			root["nvrid"] = pses->m_nvrid.c_str();
			nvrid = pses->m_nvrid;
		} else if (sesType == TYPE_DOWNLOAD) {
			root["nvrid"] = pses->m_nvrid.c_str();
			nvrid = pses->m_nvrid;

		}

		char agentid[20] = { 0 };
		sprintf(agentid, "%d", pses->m_agentid);
		sendInterJsonMessage(root, agentid);
	}

	if (sesType == TYPE_LIVE) {
		resetDev(did);
	} else if (sesType == TYPE_PLAYBACK) {
		string nvrid = pses->m_nvrid;
		if (!nvrid.empty()) {
			Debug(ERROR, "reset nvr 4:%s channelid:%s", nvrid.c_str(), did.c_str());
			resetDev(nvrid);
		}
	} else {
		string nvrid = pses->m_nvrid;
		if (!nvrid.empty() && did != nvrid) {
			Debug(ERROR, "reset nvr 5:%s channelid:%s", nvrid.c_str(), did.c_str());
			resetDev(nvrid);
		}

	}

}
bool GB28181Server::isTcpSdp(char * buf) {
	std::vector<std::string> itemList;
	split(string(buf), "\r\n", &itemList);

	for (unsigned int i = 0; i < itemList.size(); i++) {
		string item = itemList[i];
		if (item.find("m=") != string::npos) {
			puts(item.c_str());
			if (item.find("TCP") == string::npos) {
				return false;
			} else {
				return true;
			}
		}
	}
	return false;
}
void GB28181Server::showsdp(char* buf) {
	std::vector<std::string> itemList;
	split(string(buf), "\r\n", &itemList);

	for (unsigned int i = 0; i < itemList.size(); i++) {
		string item = itemList[i];
		if (item.find("m=") != string::npos) {
			puts(item.c_str());
			if (item.find("TCP") == string::npos) {
			} else {
			}
		}
	}
	sdp_message_t* sdp;
	sdp_message_init(&sdp);
	sdp_message_parse(sdp, buf);
	printf("dev sdp id: %s %s %s %s\n", sdp->o_nettype, sdp->o_addrtype, sdp->o_addr, sdp->o_username);
	sdp_message_free(sdp);
}
void GB28181Server::handleCallAnswer(eXosip_event_t *je) {

	if (je->request == NULL)
		return;

	char * sdpstr = NULL;
	osip_body_t *body = NULL;
	osip_message_get_body(je->response, 0, &body);
	if (body != NULL) {
		sdpstr = body->body;
	}

	osip_to_t *hto = osip_message_get_to(je->request);
	osip_from_t *hfrom = osip_message_get_from(je->request);

	char *remoteIpAddr = je->remote_ipaddr; //[64] = { 0 };
	int remotePort = je->remote_port;

	int cid = je->cid;
	if (theApp.isServerSipIp(remoteIpAddr) || strcmp(remoteIpAddr, "127.0.0.1") == 0) {

		printf("server answer: host=%s %s %s toid=%s\n", hfrom->url->host, hfrom->url->scheme, hfrom->url->username, hto->url->username);
		Debug(INFO, "server call ack: did = %d host=%s from=%s toid=%s", je->did, remoteIpAddr, hfrom->url->username, hto->url->username);
		string did = hfrom->url->username;
		int sesType = 0;
		CallSession* pses = getSessionByCid(did, 1, cid, sesType);
		unsigned char decodeSDP[1024] = { 0 };
		int sdplen = b64_decode((unsigned char*) sdpstr, strlen(sdpstr), decodeSDP);

		printf("server ack:%s\n %s\n", sdpstr, (char*) decodeSDP);
		if (sesType != 0 && pses != NULL && sdplen > 0) {

			osip_call_id_t * callid = osip_message_get_call_id(je->request);
			if (callid != NULL) {
				char * pcallid;
				osip_call_id_to_str(callid, &pcallid);

				printf("server did = %s callid = %s %s %s %s\n", did.c_str(), pcallid, callid->number, callid->host,
						pses->m_srv_call_id.c_str());
				pses->m_srv_call_id = pcallid;
				osip_free(pcallid);

			} else {
				puts("!!!!!!!!!!!!!callid error");
			}

			{
				osip_from_t *hfrom = osip_message_get_from(je->request);

				osip_uri_param_t * ptag;
				osip_uri_param_get_byname(&(hfrom->gen_params), "tag", &ptag);
				if (ptag != NULL) {
					printf("from header:%s %s\n", ptag->gname, ptag->gvalue);
					pses->m_srv_call_tag = ptag->gvalue;
				}
			}

			{
				osip_to_t *hto = osip_message_get_to(je->response);

				osip_uri_param_t * ptag;
				osip_uri_param_get_byname(&(hto->gen_params), "tag", &ptag);
				if (ptag != NULL) {
					printf("from header:%s %s\n", ptag->gname, ptag->gvalue);
					pses->m_srv_call_tag_to = ptag->gvalue;
				}
			}
			pses->m_session_status = CALL_WAIT_DEV;
			pses->m_srv_did = je->did;
			Debug(ERROR, "server call :did %d", je->did);
			string mediaKeepPort;
			bool ret = checkSrvAnswer((char*) decodeSDP, mediaKeepPort);
			pses->m_http_port = atoi(mediaKeepPort.c_str());

			int devcid = 0;
			if (!ret) {
				if (decodeSDP != NULL) {
					devcid = SendInvitePlay2dev(m_eCtx, did, sesType, (char*) decodeSDP);
				} else {
					devcid = SendInvitePlay2dev(m_eCtx, did, sesType);
				}

				if (devcid < 0) {
					Debug(ERROR, "invite to dev , return err cid %s", did);
					Json::Value root;
					root["cmd"] = "call_invite_ack";
					root["result"] = "devfail";
					root["did"] = did;
					root["server"] = pses->m_msserver.m_streamserver_ip;
					root["type"] = sesType;
					root["http_port"] = pses->m_http_port;
					root["http_ip"] = pses->m_msserver.m_streamserver_ip;

					if (sesType != TYPE_LIVE) {
						root["nvrid"] = pses->m_nvrid.c_str();
					}

					root["url"] = "";
					char agentid[20] = { 0 };
					sprintf(agentid, "%d", pses->m_agentid);
					sendInterJsonMessage(root, agentid);

					closeSession(did, sesType, 2);

				}
			} else {
				osip_message_t *ack = NULL;
				eXosip_call_build_ack(m_eCtx, je->did, &ack);
				eXosip_call_send_ack(m_eCtx, je->did, ack);

				{
					Json::Value root;
					root["cmd"] = "call_invite_ack";
					root["result"] = "ok";
					root["did"] = did;
					root["server"] = pses->m_msserver.m_streamserver_ip;
					root["type"] = sesType;
					root["http_port"] = pses->m_http_port;
					root["http_ip"] = pses->m_msserver.m_streamserver_ip;

					char buf[256] = { 0 };
					if (sesType == TYPE_LIVE) {
						sprintf(buf, "rtmp://%s/live/%s", pses->m_msserver.m_streamserver_ip.c_str(), did.c_str());
					} else if (sesType == TYPE_PLAYBACK) {
						root["nvrid"] = pses->m_nvrid.c_str();
						sprintf(buf, "rtmp://%s/playback/%s", pses->m_msserver.m_streamserver_ip.c_str(), did.c_str());
					} else if (sesType == TYPE_DOWNLOAD) {
						root["nvrid"] = pses->m_nvrid.c_str();
						sprintf(buf, "rtmp://%s/download/%s", pses->m_msserver.m_streamserver_ip.c_str(), did.c_str());
					}

					root["url"] = buf;
					char agentid[20] = { 0 };
					sprintf(agentid, "%d", pses->m_agentid);
					sendInterJsonMessage(root, agentid);
				}

			}

		} else {
			Debug(ERROR, "serv ack:id = %s, cid=%d", did.c_str(), cid);
		}

	} else {
		Debug(ERROR, "dev call ack: host=%s from=%s toid=%s", remoteIpAddr, hfrom->url->username, hto->url->username);

		printf("dev answer: host=%s %s %s toid=%s\n", hfrom->url->host, hfrom->url->scheme, hfrom->url->username, hto->url->username);

		string did = hto->url->username;

		int sesType = 0;
		CallSession* pses = getSessionByCid(did, 2, cid, sesType);

		if (pses != NULL) {
			pses->m_dev_did = je->did;
			Debug(ERROR, " dev ack: devid=%s did= %d cid = %d  sestype=%d jecid=%d\n", did.c_str(), je->did, pses->m_callid_dev, sesType,
					cid);
		} else {
			Debug(ERROR, "no session found:%s %d", did.c_str(), sesType);
			return;
		}
		{
			osip_from_t *hfrom = osip_message_get_from(je->request);

			//osip_to_param_get_byname
			osip_uri_param_t * ptag;
			osip_uri_param_get_byname(&(hfrom->gen_params), "tag", &ptag);
			if (ptag != NULL) {
				printf("from header:%s %s\n", ptag->gname, ptag->gvalue);
				pses->m_dev_call_tag = ptag->gvalue;
			}
		}

		{
			osip_to_t *hto = osip_message_get_to(je->response);

			osip_uri_param_t * ptag;
			osip_uri_param_get_byname(&(hto->gen_params), "tag", &ptag);
			if (ptag != NULL) {
				printf("from header:%s %s\n", ptag->gname, ptag->gvalue);
				pses->m_dev_call_tag_to = ptag->gvalue;
			}
		}
//		{
//			osip_cseq_t* hseq = osip_message_get_cseq(je->request);
//			if (hseq != NULL) {
//				pses->m_callid_dev = atoi(hseq->number);
//			}
//
//		}

		osip_call_id_t * callid = osip_message_get_call_id(je->response);
		if (callid != NULL) {
			char * pcallid;
			osip_call_id_to_str(callid, &pcallid);

			printf("server did = %s callid = %s %s %s %s\n", did.c_str(), pcallid, callid->number, callid->host,
					pses->m_dev_call_id.c_str());
			pses->m_dev_call_id = pcallid;
			pses->m_sip_cid_number = atoi(callid->number);
			osip_free(pcallid);

		} else {
			puts("!!!!!!!!!!!!!callid error");
		}

		string jsonstr = pses->toJsonStr();
		m_dev_local_cache.insertSes(sesType, did, jsonstr);

		char buf[1024] = { 0 };
		sprintf(buf, "%suluip=%s\r\ndeviceid=%s\r\nmediatype=%d\r\n", sdpstr, remoteIpAddr, did.c_str(), sesType);
		string sdpstr = buf;
		Debug(INFO, "dev sdp = %s ", theApp.replace_str(sdpstr, string("\r\n"), string(" ")).c_str());
		//	puts(buf);

		ResponseCallDevAck(m_eCtx, je);
		ResponseCallSrvAck(m_eCtx, je, pses, buf);

		pses->m_session_status = CALL_OK;
		if (pses->m_trans_type == 1) {

//			if (!isTcpSdp(sdpstr)) {
//				Debug(ERROR, "dev sdp , no tcp %s", did.c_str());
//				Json::Value root;
//				root["cmd"] = "call_invite_ack";
//				root["result"] = "devfail";
//				root["did"] = did;
//				root["server"] = pses->m_msserver.m_streamserver_ip;
//				root["type"] = sesType;
//				root["http_port"] = pses->m_http_port;
//				root["http_ip"] = pses->m_msserver.m_streamserver_ip;
//				if (sesType != TYPE_LIVE) {
//					root["nvrid"] = pses->m_nvrid.c_str();
//				}
//				root["url"] = "";
//				char agentid[20] = { 0 };
//				sprintf(agentid, "%d", pses->m_agentid);
//				sendInterJsonMessage(root, agentid);
//			}
		}
		{
			Json::Value root;
			root["cmd"] = "call_invite_ack";
			root["result"] = "ok";
			root["did"] = did;
			root["server"] = pses->m_msserver.m_streamserver_ip;
			root["type"] = sesType;
			root["http_port"] = pses->m_http_port;
			root["http_ip"] = pses->m_msserver.m_streamserver_ip;

			char buf[256] = { 0 };
			if (sesType == TYPE_LIVE) {
				sprintf(buf, "rtmp://%s/live/%s", pses->m_msserver.m_streamserver_ip.c_str(), did.c_str());
			} else if (sesType == TYPE_PLAYBACK) {
				root["nvrid"] = pses->m_nvrid.c_str();
				sprintf(buf, "rtmp://%s/playback/%s", pses->m_msserver.m_streamserver_ip.c_str(), did.c_str());
			} else if (sesType == TYPE_DOWNLOAD) {
				root["nvrid"] = pses->m_nvrid.c_str();
				sprintf(buf, "rtmp://%s/download/%s", pses->m_msserver.m_streamserver_ip.c_str(), did.c_str());
			}

			root["url"] = buf;
			char agentid[20] = { 0 };
			sprintf(agentid, "%d", pses->m_agentid);
			sendInterJsonMessage(root, agentid);
		}
	}

}
void GB28181Server::ResponseCallSrvAck(struct eXosip_t * eCtx, eXosip_event_t *je, CallSession* pses, char* sdp) {

	osip_from_t *hfrom = osip_message_get_to(je->request);
	string did = hfrom->url->username;

	osip_message_t *callack_;
	Debug(ERROR, "ack to server:%d %s", pses->m_srv_did, did.c_str());
	if (OSIP_SUCCESS == eXosip_call_build_ack(eCtx, pses->m_srv_did, &callack_)) {

		if (sdp != NULL) {
			osip_message_set_content_type(callack_, "application/sdp");
			unsigned char base64Body[1024] = { 0 };
			int len = b64_encode((unsigned char*) sdp, strlen(sdp), base64Body);
			puts((char*) base64Body);
			osip_message_set_body(callack_, (char*) base64Body, len);
		}
		eXosip_call_send_ack(eCtx, pses->m_srv_did, callack_);
	}
}

void GB28181Server::ResponseCallDevAck(struct eXosip_t * peCtx, eXosip_event_t *je) {

	osip_message_t *ack = NULL;
	eXosip_call_build_ack(m_eCtx, je->did, &ack);
	eXosip_call_send_ack(m_eCtx, je->did, ack);

}

void GB28181Server::testSendInvitePlay(bool isdev) {

}

string GB28181Server::makeSSRC(string serverid, int streamtype, string did) {
	theApp.m_ssrc_count++;
	if (theApp.m_ssrc_count > 9999) {
		theApp.m_ssrc_count = 0;
	}

	string realmid = serverid.substr(4, 5);
//string dev_ssrc_id = did.substr(10, 3);
	string dev_id = did.substr(15, 5);

	char buf[20] = { 0 };
//	sprintf(buf, "%d%s%04d", streamtype, realmid.c_str(), theApp.m_ssrc_count);
	sprintf(buf, "%d%04d%s", streamtype == 1 ? 0 : 1, theApp.m_ssrc_count, dev_id.c_str());
	return string(buf);

}
void GB28181Server::delSessionBycid(string did, int callid) {
	Debug(ERROR, "Bye ack from :%s cid=%d", did.c_str(), callid);
	SesIt it;
	{
		it = m_map_sessions.find(did);
		if (it != m_map_sessions.end()) {

			if (callid == it->second.m_sip_cid_number) {

				puts("live ack");
				Json::Value root;
				root["cmd"] = "call_close_ack";
				root["type"] = TYPE_LIVE;
				root["did"] = did;
				root["nvrid"] = "";
				root["result"] = "ok";
				sendInterJsonMessage(root, it->second.m_closeagentid);
				Debug(INFO, "complete live session :%s", did.c_str());

				m_map_sessions.erase(it);
				m_dev_local_cache.deleteSesByDid(1, did);
				return;
			}
		}
	}
	{
		it = m_map_playback_sessions.find(did);
		if (it != m_map_playback_sessions.end()) {
			if (callid == it->second.m_sip_cid_number) {
				puts("playback ack");
				Json::Value root;
				root["cmd"] = "call_close_ack";
				root["type"] = TYPE_PLAYBACK;
				root["did"] = did;
				root["nvrid"] = it->second.m_nvrid;
				root["result"] = "ok";
				sendInterJsonMessage(root, it->second.m_closeagentid);
				Debug(INFO, "complete playback session :%s", did.c_str());

				m_map_playback_sessions.erase(it);
				m_dev_local_cache.deleteSesByDid(2, did);

				return;
			}
		}
	}
	{
		it = m_map_download_sessions.find(did);
		if (it != m_map_download_sessions.end()) {
			if (callid == it->second.m_sip_cid_number) {
				Json::Value root;
				root["cmd"] = "call_close_ack";
				root["type"] = TYPE_DOWNLOAD;
				root["did"] = did;
				root["nvrid"] = it->second.m_nvrid;
				root["result"] = "ok";
				sendInterJsonMessage(root, it->second.m_closeagentid);

				m_map_download_sessions.erase(it);
				m_dev_local_cache.deleteSesByDid(3, did);

				return;
			}
		}
	}
	return;
}
void GB28181Server::delSession(string did, int type) {
	Debug(INFO, "delete session:did=%s type=%d", did.c_str(), type);
	SesIt it;
	if (type == TYPE_LIVE) {
		it = m_map_sessions.find(did);
		if (it != m_map_sessions.end()) {
			m_map_sessions.erase(it);
			return;
		}
	}
	if (type == TYPE_PLAYBACK) {
		it = m_map_playback_sessions.find(did);
		if (it != m_map_playback_sessions.end()) {
			m_map_playback_sessions.erase(it);
			return;
		}
	}
	if (type == TYPE_DOWNLOAD) {
		it = m_map_download_sessions.find(did);
		if (it != m_map_download_sessions.end()) {
			m_map_download_sessions.erase(it);
			return;
		}
	}
	return;
}
CallSession* GB28181Server::getSession(string did, int type) {
	SesIt it;
	if (type == TYPE_LIVE) {
		it = m_map_sessions.find(did);
		if (it != m_map_sessions.end()) {
			return &(it->second);
		}
	}
	if (type == TYPE_PLAYBACK) {
		it = m_map_playback_sessions.find(did);
		if (it != m_map_playback_sessions.end()) {
			return &(it->second);
		}
	}
	if (type == TYPE_DOWNLOAD) {
		it = m_map_download_sessions.find(did);
		if (it != m_map_download_sessions.end()) {
			return &(it->second);
		}
	}
	return NULL;
}

CallSession* GB28181Server::getSessionByCid(string did, int idType, int cid, int& sesType) {
	SesIt it;
	{
		it = m_map_sessions.find(did);
		if (it != m_map_sessions.end()) {
			sesType = TYPE_LIVE;
			int sescallid = 0;
			if (idType == 1) {
				sescallid = it->second.m_callid_srv;
			} else if (idType == 2) {
				sescallid = it->second.m_callid_dev;
			}

			if (sescallid == cid) {
				return &(it->second);
			}
		}
	}
	{
		it = m_map_playback_sessions.find(did);
		if (it != m_map_playback_sessions.end()) {
			sesType = TYPE_PLAYBACK;
			int sescallid = 0;
			if (idType == 1) {
				sescallid = it->second.m_callid_srv;
			} else if (idType == 2) {
				sescallid = it->second.m_callid_dev;
			}
			if (sescallid == cid) {
				return &(it->second);
			}
		}
	}
	{
		it = m_map_download_sessions.find(did);
		if (it != m_map_download_sessions.end()) {
			sesType = TYPE_DOWNLOAD;
			int sescallid = 0;
			if (idType == 1) {
				sescallid = it->second.m_callid_srv;
			} else if (idType == 2) {
				sescallid = it->second.m_callid_dev;
			}
			if (sescallid == cid) {
				return &(it->second);
			}
		}
	}
	return 0;

}

void GB28181Server::split(const std::string& s, const std::string& delim, std::vector<std::string>* ret) {

	size_t last = 0;
	size_t index = s.find_first_of(delim, last);

	while (index != std::string::npos) {
		ret->push_back(s.substr(last, index - last));
		last = index + delim.length();
		index = s.find_first_of(delim, last);
	}
	if (index - last > 0) {
		ret->push_back(s.substr(last, index - last));
	}
}

string GB28181Server::adjustSDP(string orgsdp, string s, string y) {
	std::vector<std::string> itemList;
	split(orgsdp, "\r\n", &itemList);

	bool sFound = false;
	bool yFound = false;
	for (unsigned int i = 0; i < itemList.size(); i++) {
		string item = itemList[i];
		if (item.find("s=") != string::npos) {
			char buf[64] = { 0 };
			sprintf(buf, "s=%s", s.c_str());
			itemList[i] = buf;
			sFound = true;
		}
		if (item.find("y=") != string::npos) {
			char buf[64] = { 0 };
			sprintf(buf, "y=%s", y.c_str());
			itemList[i] = buf;
			yFound = true;
		}
	}
	if (!sFound) {
		char buf[64] = { 0 };
		sprintf(buf, "s=%s", s.c_str());
		itemList.push_back(string(buf));
	}
	if (!yFound) {
		char buf[64] = { 0 };
		sprintf(buf, "y=%s", y.c_str());
		itemList.push_back(string(buf));
	}

	string dstSDP;
	for (unsigned int i = 0; i < itemList.size() - 1; i++) {
		dstSDP.append(itemList[i]);
		dstSDP.append("\r\n");
	}
	return dstSDP;
}

bool GB28181Server::getStreamserver(CallSession* pses) {

	DevIt it = this->m_map_devs.find(pses->m_nvrid);
	if (it == m_map_devs.end()) {
		return false;
	}

	if (!pses->m_msserver.m_streamserver_ip.empty()) {
		return true;
	}
//	mediaserver server;
//	if (true == theApp.selectServer1(it->second->m_dev_remote_ip, server)) {
//		pses->m_msserver = server;
//	} else {
//		return false;
//	}
	mediaserver* pserver = theApp.selectServer1(it->second->m_dev_remote_ip);
	if (pserver == NULL) {
		return false;
	}
	pses->m_msserver = *pserver;
	return true;
}
int GB28181Server::SendInvitePlay2Srv(struct eXosip_t *peCtx, string clientid, CallSession* pses, int sesType, string did,
		bool liveopened) {

	if (m_map_devs.find(pses->m_nvrid) == m_map_devs.end()) {
		return -1;
	}

	if (getStreamserver(pses) == false) {
		return -1;
	}

	char dest_call[256] = { 0 };
	char source_call[256] = { 0 };
	char route[256] = { 0 };
	char subject[256] = { 0 };

	snprintf(dest_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), pses->m_msserver.m_streamserver_sip_ip.c_str(),
			pses->m_msserver.m_streamserver_sip_port);
	snprintf(source_call, 256, "sip:%s@%s:%d", did.c_str(), theApp.m_sipserver_ip.c_str(), theApp.m_sipserver_port);
	snprintf(route, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), pses->m_msserver.m_streamserver_sip_ip.c_str(),
			pses->m_msserver.m_streamserver_sip_port);
	snprintf(subject, 256, "%s:0,%s:0", clientid.c_str(), did.c_str());
//Debug(ERROR,"----->> invite ms, %s,%s,%s\n", dest_call, source_call, subject);

	osip_message_t *invite = NULL;

	int ret = eXosip_call_build_initial_invite(peCtx, &invite, dest_call, source_call, route, subject);

	if (ret != 0) {
		Debug(ERROR, "eXosip_call_build_initial_invite failed, %s,%s,%s", dest_call, source_call, subject);
		return -1;
	}
	{
		Json::Value root;
		string playtype;
		if (sesType == TYPE_LIVE)
			playtype = "live";
		else if (sesType == TYPE_PLAYBACK)
			playtype = "playback";
		else if (sesType == TYPE_DOWNLOAD)
			playtype = "download";
		char buf[256] = { 0 };
		sprintf(buf, "rtmp://%s/%s/%s", pses->m_msserver.m_streamserver_ip.c_str(), playtype.c_str(), did.c_str());
		root["url"] = buf;
		root["did"] = did;
		root["type"] = sesType;
		Json::FastWriter writer;
		string message = writer.write(root);
		osip_message_set_content_type(invite, "application/json");

		unsigned char base64Body[1024] = { 0 };
		int len = b64_encode((unsigned char*) message.c_str(), message.length(), base64Body);
		printf("invite to serv:%s\n", (char*) base64Body);
		osip_message_set_body(invite, (char*) base64Body, len);

		Debug(INFO, "invite srv : |%s |%s |%s", dest_call, source_call, message.c_str());

	}
	if (liveopened == false) {
		theApp.updateServerLoad(1, pses->m_msserver.m_streamserver_ip);
	}
	return (eXosip_call_send_initial_invite(peCtx, invite));
}

int GB28181Server::SendInvitePlay2dev(struct eXosip_t *peCtx, string did, int sesType, char* sdp) {

	CallSession* pses = getSession(did, sesType);
	DevIt it1 = m_map_devs.find(pses->m_nvrid);
	if (it1 == m_map_devs.end()) {
		Debug(ERROR, "call dev fail, offline =%d", pses->m_nvrid.c_str());
		return -1;
	}
	CameraDevice* dev = it1->second;

	char dest_call[256] = { 0 };
	char source_call[256] = { 0 };
	char route[256] = { 0 };
	char subject[256] = { 0 };

	pses->m_invite_dev_port = dev->m_dev_remote_port;
	snprintf(dest_call, 256, "sip:%s@%s:%d", did.c_str(), dev->m_dev_remote_ip.c_str(), dev->m_dev_remote_port);
	snprintf(source_call, 256, "sip:%s@%s", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str());
	snprintf(route, 256, "sip:%s@%s:%d", did.c_str(), dev->m_dev_remote_ip.c_str(), dev->m_dev_remote_port);
	snprintf(subject, 256, "%s:0,%s:0", did.c_str(), theApp.m_sipserver_id.c_str());

	printf("----->> invite dev, %s,%s,%s %s\n", dest_call, source_call, subject, route);
	osip_message_t *invite = NULL;
	int ret = eXosip_call_build_initial_invite(peCtx, &invite, dest_call, source_call, route, subject);

	if (ret != 0) {
		fprintf(stderr, "eXosip_call_build_initial_invite failed, %s,%s,%s", dest_call, source_call, subject);
		Debug(ERROR, "call dev fail,exosip fail =%d", pses->m_nvrid.c_str());
		return -1;
	}

//sdpstr
	stringstream sdpstr("");
	sdpstr << "v=0\r\n";
	if (pses->m_trans_type == 1) {
		sdpstr << "o=" << theApp.m_sipserver_id.c_str() << " 0 0 IN IP4 " << pses->m_msserver.m_streamserver_ip << "\r\n";

	} else {
		sdpstr << "o=" << theApp.m_sipserver_id.c_str() << " 0 0 IN IP4 " << pses->m_msserver.m_streamserver_ip << "\r\n";
	}
	if (sesType == TYPE_LIVE) {
		sdpstr << "s=Play\r\n";

	} else if (sesType == TYPE_PLAYBACK) {
		sdpstr << "s=Playback\r\n";
		if (pses->m_nvrid != did) {
			sdpstr << "u=" << did << ":4\r\n";
		}
	} else if (sesType == TYPE_DOWNLOAD) {
		sdpstr << "s=Download\r\n";
		if (pses->m_nvrid != did) {
			sdpstr << "u=" << did << ":4\r\n";
		}
	}
	if (pses->m_trans_type == 1) {
		sdpstr << "c=IN IP4 " << pses->m_msserver.m_streamserver_ip << "\r\n";
	} else {
		sdpstr << "c=IN IP4 " << pses->m_msserver.m_streamserver_ip << "\r\n";
	}
	if (sesType == TYPE_LIVE) {
		sdpstr << "t=0 0\r\n";
	}
	if (sesType == TYPE_PLAYBACK || sesType == TYPE_DOWNLOAD) {
		sdpstr << "t=" << pses->m_starttime << " " << pses->m_endtime << "\r\n";
	}

	if (sesType == TYPE_LIVE) {
		if (m_map_sessions[did].m_with_audio == 1) {
			if (pses->m_trans_type == 1) {
				sdpstr << "m=audio " << 0 << " TCP/RTP/AVP 0\r\n";
			} else {
				sdpstr << "m=audio " << 0 << " RTP/AVP 0\r\n";
			}
			sdpstr << "a=rtpmap:0 PCMU/8000\r\n";
		}
	} else if (sesType == TYPE_PLAYBACK || sesType == 3) {
		if (pses->m_trans_type == 1) {
			sdpstr << "m=audio 0 TCP/RTP/AVP 0\r\n";
		} else {
			sdpstr << "m=audio 0 RTP/AVP 0\r\n";
		}
		sdpstr << "a=rtpmap:0 PCMU/8000\r\n";
	}
	if (pses->m_trans_type == 1) {
		sdpstr << "m=video " << pses->m_msserver.m_streamserver_rtp_tcp_port << " TCP/RTP/AVP 96 97 98\r\n";
	} else {
		sdpstr << "m=video " << pses->m_msserver.m_streamserver_rtp_udp_port << " RTP/AVP 96 97 98\r\n";
	}
	sdpstr << "a=rtpmap:96 PS/90000\r\n";
	sdpstr << "a=rtpmap:97 MPEG4/90000\r\n";
	sdpstr << "a=rtpmap:98 H264/90000\r\n";
	sdpstr << "a=recvonly\r\n";
	if (pses->m_trans_type == 1) {
		sdpstr << "a=setup:passive\r\n";
		sdpstr << "a=connection:new\r\n";
	}
	if (sesType == TYPE_DOWNLOAD) {
		sdpstr << "a=downloadspeed:" << pses->m_speed << "\r\n";
	}
	sdpstr << "y=" << makeSSRC(theApp.m_sipserver_id, sesType, did).c_str() << "\r\n";
//sdpstr << "v/2/5/20/2/512a/1//8\r\n";
//	if (sesType == 1) {
//		sdpstr << "v/2/" << (pses->m_bitrate == 1000 ? 5 : 4) << "/20/2/" << (pses->m_bitrate == 1000 ? 500 : 300);
//		if (pses->m_with_audio == 1) {
//			sdpstr << "a/1//8";
//		} else {
//			sdpstr << "a///";
//		}
//		sdpstr << "\r\n";
//	}
//1———MPEG-4 2———H.264 3———SVAC 4———3GP
//1———QCIF 2———CIF 3———4CIF 4———D1 5———720P 6———1080P/I
//1———固定码率(CBR) 2———可变码率(VBR)
//audio
//1———G.711 2———G.723.1 3———G.729 4———G.722.1
//3--8k 4--16k
//	if (sdp != NULL) {
//		string devsdp = adjustSDP(string(sdp), "play", getSSRC(theApp.SERVER_SIP_ID, 0));
//		printf("sdp 2 dev=%s\n", devsdp.c_str());
//		osip_message_set_body(invite, devsdp.c_str(), devsdp.length());
//	} else
	{
		osip_message_set_body(invite, sdpstr.str().c_str(), sdpstr.str().length());
	}

	osip_message_set_content_type(invite, "APPLICATION/SDP");
	eXosip_lock(peCtx);
	int call_id = eXosip_call_send_initial_invite(peCtx, invite);
	eXosip_unlock(peCtx);

	pses->m_callid_dev = call_id;

	Debug(INFO, "invite dev %d: |%s |%s |%s", call_id, dest_call, source_call, sdpstr.str().c_str());

	return call_id;
}
