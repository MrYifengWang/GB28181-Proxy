#include "GB28181Server.h"
#include "MessageQueue.h"
#include "ThreadMessageProcess.h"

GB28181Server::GB28181Server() {
	m_is_sip_stop = false;
	m_cur_idx = 0;
	m_local_com_socket = 0;
	m_lastCheckTime = 0;

}
GB28181Server::~GB28181Server() {
}

void GB28181Server::startThread(GB28181Server** pserver) {
	GB28181Server* server = new GB28181Server();
	*pserver = server;
	server->setAutoDelete(true);
	server->start();
}

void GB28181Server::stop() {
}
char * getMessageStr() {

}
void GB28181Server::recoverSesMap() {

	for (int i = 1; i <= 3; i++) {
		std::vector<db_dev_item_> list;
		m_dev_local_cache.queryAllSes(i, list);
		int curtime = time(NULL);

		for (int j = 0; j < list.size(); j++) {

			CallSession ses;
			ses.parsefrom(list[j].jsonStr);
			if (i == 1) {
				m_map_sessions[list[j].devid] = ses;
			} else if (i == 2) {
				m_map_playback_sessions[list[j].devid] = ses;
			} else if (i == 3) {
				m_map_download_sessions[list[j].devid] = ses;
			}
		}
	}
}

void GB28181Server::recoverDevMap() {
	m_dev_local_cache.initSqlDB();

	std::vector<db_dev_item_> list;
	m_dev_local_cache.queryAllDev(list);
	int curtime = time(NULL);

	for (int i = 0; i < list.size(); i++) {
		//	if (curtime - list[i].updateTime > 320) {
		Json::Reader reader;
		Json::Value jmessage;
		if (reader.parse(list[i].jsonStr, jmessage)) {
			string sn = jmessage["SN"].asString();
			int devtype = jmessage["devtype"].asInt();
			Debug(INFO, "init,clear time out dev:%s", sn.c_str());
			int ret = m_dev_remote_cache.delDevItem((char*) sn.c_str());
			onDevStatChange(sn, devtype, 0);
			m_dev_local_cache.deleteDevByDid(list[i].devid);

			Debug(INFO, "init,clear time out dev:%s ret=%d", sn.c_str(), ret);
		}

		continue;
		//	}
		printf("recover %s %s\n", list[i].devid.c_str(), list[i].jsonStr.c_str());
		/*
		 Json::Reader reader;
		 Json::Value jmessage;
		 if (reader.parse(list[i].jsonStr, jmessage)) {
		 CameraDevice* node = new CameraDevice();
		 node->m_devid = list[i].devid.c_str();
		 node->m_dev_remote_ip = jmessage["devip"].asString();
		 node->m_dev_remote_port = jmessage["devport"].asInt();
		 node->m_dev_status = STAT_RESET;
		 node->m_dev_type = jmessage["devtype"].asInt();
		 node->m_uluSN = jmessage["SN"].asString();
		 node->m_last_keep_time = curtime;
		 m_map_devs.insert(make_pair(list[i].devid, node));

		 }
		 */

	}
	m_dev_local_cache.deleteAllDev();
}
void GB28181Server::initLocalMessageQueue() {
	Debug(DEBUG, "queue size %d", QUEUE_SIZE);
	for (int i = 0; i < QUEUE_SIZE; i++) {
		ThreadMessageProcess::startThread(&m_queue_[i]);
	}
}
void testmap() {
	std::map<int, int> cache;
	for (int i = 1; i < 10; ++i)
		cache[i] = i * 10;

	std::map<int, int>::iterator it = cache.begin();
	it = cache.begin();
	while (it != cache.end()) {
		printf("before--%d\n", it->second);
		it++;
	}
	it = cache.begin();

	while (it != cache.end()) {
		if (it->second == 50 || it->second == 70) {
			printf("del----%d\n", it->second);

			cache.erase(it++);
		} else
			it++;
	}
	it = cache.begin();
	while (it != cache.end()) {
		printf("after--%d\n", it->second);
		it++;
	}

}

void testHttpauth() {
	return;
	string ha1;
	string ha2;

//HA1
	{
		string str = "sip:31010450231328000006@172.30.216.85:15060:34020000:ulucu888";
		ulu_md5::make(str, ha1);
		puts(ha1.c_str());
	}

	stringstream ha2str("");
	ha2str << "REGISTER" << ":" << "sip:34020000022000000001@172.30.216.85:15060";
	ulu_md5::make(ha2str.str(), ha2);
	printf("--md:%s\n", ha2str.str().c_str());

	//resp
	stringstream respstr("");
	respstr << ha1 << ":" << "6fe9ba44a76be22a" << ":" << ha2;
	string out;
	ulu_md5::make(respstr.str(), out);

	puts(out.c_str());

}

bool GB28181Server::onStart() {
//	testmap();
//	testHttpauth();
	initLocalMessageQueue();

	recoverDevMap();

	{
		struct sigaction act;
		memset(&act, 0, sizeof(act));
		sigemptyset(&act.sa_mask);
		act.sa_handler = SIG_IGN;
		if (sigaction(SIGPIPE, &act, NULL) < 0) {

		}
	}

	int iRet = 0;

	TRACE_INITIALIZE(6, NULL);

	m_eCtx = eXosip_malloc();
	iRet = eXosip_init(m_eCtx);
	if (iRet != OSIP_SUCCESS) {
		Debug(ERROR, "Can't initialize eXosip!");
		return false;
	} else {
		Debug(ERROR, "eXosip_init successfully!\n");
	}

	iRet = eXosip_listen_addr(m_eCtx, IPPROTO_UDP, NULL, 15060, AF_INET, 0);
	if (iRet != OSIP_SUCCESS) {
		Debug(ERROR, "eXosip_listen_addr error!");
		return false;
	}

	return true;

}
bool isInList(string did, vector<string>& list) {
	for (int i; i < list.size(); i++) {
		if (list[i] == did) {
			return true;
		}
	}
	return false;
}
void GB28181Server::checkRegTimeout() {

	int curtime = time(NULL);
	if (curtime - m_lastCheckTime > REG_TIMEOUT_CHECK) {

		m_lastCheckTime = curtime;
		DevIt iter = m_map_devs.begin();
		while (iter != m_map_devs.end()) {
			if (curtime - iter->second->m_last_keep_time > ONLINE_EXP_TIME) {
				int ret = m_dev_remote_cache.delDevItem((char*) iter->second->m_uluSN.c_str());
				onDevStatChange(iter->second->m_uluSN, iter->second->m_dev_type, 0);
				m_dev_local_cache.deleteDevByDid(iter->second->m_devid);

				Debug(ERROR, "dev stat time out:%s %s ret=%d", iter->second->m_uluSN.c_str(), iter->second->m_devid.c_str(), ret);
				theApp.m_xdmgr.deleteConfig(iter->second->m_devid);
				delete (iter->second);
				m_map_devs.erase(iter++);

			} else
				iter++;
		}

	}
}
void GB28181Server::getQueueSize(vector<int> list) {
	for (int i = 0; i < QUEUE_SIZE; i++) {
		list.push_back(m_queue_[i].size());
	}
}
void GB28181Server::onDevStatChange(string sn, int type, int stat) {

	Json::Value root;
	root["sn"] = sn;
	root["type"] = type;
	root["stat"] = stat;

	Json::FastWriter writer;
	string message = writer.write(root);

	Debug(ERROR, "on dev stat change:%s %d %d", sn.c_str(), type, stat);

	Message* msg = new Message(message, MSG_JSON, CMD_DEVSTAT);
	dispatchMessage(msg);

}

void GB28181Server::run() {

	/*
	 Json::Value m_tmpRecordData;
	 if (m_tmpRecordData.isNull()) {
	 Json::Value data;
	 Json::Value arr;

	 m_tmpRecordData["cmd"] = "query_playback_ack";
	 m_tmpRecordData["error"] = E_DEV_ERROR_OK;
	 m_tmpRecordData["reason"] = "OK";
	 m_tmpRecordData["serial"] = "123";
	 m_tmpRecordData["data"] = data;
	 m_tmpRecordData["data"]["list"] = arr;
	 m_tmpRecordData["data"]["clientid"] = 4;
	 printf("--------dd-----size=%d\n", m_tmpRecordData["data"]["list"].size());

	 }

	 m_tmpRecordData["data"]["did"] = 1;
	 m_tmpRecordData["data"]["num"] = 2;
	 m_tmpRecordData["data"]["nvrid"] = 3;

	 for (int i = 1; i < 6; i++) {
	 Json::Value item;
	 item["b"] = i;
	 item["e"] = i;
	 m_tmpRecordData["data"]["list"].append(item);
	 }

	 if (m_tmpRecordData["data"]["list"].size() != 0) {
	 printf("-------------size=%d\n", m_tmpRecordData["data"]["list"].size());
	 Json::FastWriter writer;
	 string message = writer.write(m_tmpRecordData);
	 puts(message.c_str());
	 }
	 return;
	 */

	Debug(ERROR, "gbserver start\n\n");

	while (!m_is_sip_stop) {

		checkRegTimeout();
		eXosip_event_t *je = NULL;

		je = eXosip_event_wait(m_eCtx, 0, 1000);
		if (je == NULL) {

			continue;
		}

		//	printf("-----event type:%d did=%d\n", je->type, je->did);

		switch (je->type) {
		case EXOSIP_SUBSCRIPTION_ANSWERED: {
			handleNotifyAck(je);
			break;
		}

		case EXOSIP_SUBSCRIPTION_REQUESTFAILURE: {
			printf("subscrbie fail:%s\n", this->sipmessage2str(je->request).c_str());
			break;
		}

		case EXOSIP_SUBSCRIPTION_NOTIFY: {

			handleNotify(je);
			break;
		}

		case EXOSIP_MESSAGE_REQUESTFAILURE: {
			Debug(ERROR, "message request fail:%s", sipmessage2str(je->request).c_str());
			handleMessageSendFail(je);
			break;
		}

		case EXOSIP_MESSAGE_NEW: {
			if (MSG_IS_REGISTER(je->request)) {
				handleRegister(je);
			} else if (MSG_IS_MESSAGE(je->request)) {
				handleMessage(je);
			} else if (strncmp(je->request->sip_method, "BYE", 4) != 0) {
				Debug(ERROR, "recv BYE:%s", sipmessage2str(je->request).c_str());
				Response200(m_eCtx, je);
			}
			break;
		}
		case EXOSIP_MESSAGE_ANSWERED: {
			puts("EXOSIP_MESSAGE_ANSWERED ");
			handleResponse(je);
			break;
		}
		case EXOSIP_CALL_ANSWERED: {
			handleCallAnswer(je);
			break;
		}
		case EXOSIP_CALL_NOANSWER: {

			Debug(ERROR, "not handle:call no answer :%s", sipmessage2str(je->request).c_str());

			//need response to client
			break;
		}
		case EXOSIP_CALL_RINGING: {
			Response200(m_eCtx, je);
			break;
		}
		case EXOSIP_CALL_PROCEEDING: {
			Response200(m_eCtx, je);
			break;
		}
		case EXOSIP_CALL_SERVERFAILURE:
		case EXOSIP_CALL_GLOBALFAILURE: {
			Debug(ERROR, "not handle:call server fail :%s", sipmessage2str(je->request).c_str());
			break;
		}
		case EXOSIP_CALL_REQUESTFAILURE: {
			Debug(ERROR, "request fail :%s", sipmessage2str(je->request).c_str());
			handleRequestFail(je);
			break;
		}
		case EXOSIP_CALL_MESSAGE_ANSWERED: {

			handleCallMsgResponse(m_eCtx, je);
			Response200(m_eCtx, je);
			break;
		}
		case EXOSIP_CALL_RELEASED: {
			//release dialog
			puts("EXOSIP_CALL_RELEASED");
			Response200(m_eCtx, je);
			break;
		}
		case EXOSIP_MESSAGE_REDIRECTED: {

			puts("EXOSIP_MESSAGE_REDIRECTED");
			Debug(ERROR, "not handle:redirection :%s", sipmessage2str(je->request).c_str());

			Response200(m_eCtx, je);
			break;
		}
		case EXOSIP_CALL_CLOSED: {
			printf("EXOSIP_CALL_CLOSED:\n%s", sipmessage2str(je->request).c_str());
			Response200(m_eCtx, je);
			break;
		}
		case EXOSIP_CALL_MESSAGE_NEW: {
			printf("EXOSIP_CALL_MESSAGE_NEW:\n%s", sipmessage2str(je->request).c_str());
			Response200(m_eCtx, je);
			break;
		}
		default: {
			Debug(ERROR, "not handle:other sip message: type = %d %s %s", je->type, je->textinfo, sipmessage2str(je->request).c_str());
			Response200(m_eCtx, je);
			break;
		}
		}
		eXosip_event_free(je);
	}

	eXosip_quit(m_eCtx);
	osip_free(m_eCtx);
	m_eCtx = NULL;

	Debug(ERROR, "Finished!\n");

}
void GB28181Server::dispatchMessage(Message* msg) {
	m_cur_idx++;
	if (m_cur_idx >= QUEUE_SIZE)
		m_cur_idx = 0;
	m_queue_[m_cur_idx].push(msg);
}

void GB28181Server::handleInterMessage(osip_message_t * msg) {

}

void GB28181Server::handleMessage(eXosip_event_t *je) {

	osip_content_type_t * contentType = osip_message_get_content_type(je->request);

	if (contentType != NULL) {

		if (strcmp(contentType->subtype, "MANSCDP+xml") == 0) {
			osip_body_t *body = NULL;
			osip_message_get_body(je->request, 0, &body);
			if (body != NULL) {

				TiXmlDocument doc;
				doc.Parse(body->body);
				TiXmlHandle docHandle(&doc);
				TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("CmdType").ToElement();
				if (NULL != devinfo) {
					char* cmdstr = (char*) devinfo->ToElement()->GetText();
					if (strcmp(cmdstr, "RecordInfo") == 0) {
						handleRecordResponse1(je, docHandle);
						return;
					} else if (0 == strcmp(cmdstr, "DeviceInfo")) {
						handleDevInfoQuery1(je, docHandle);
						return;
					} else if (0 == strcmp(cmdstr, "Catalog")) {
						handleCatalogResponse1(je, docHandle);
						return;
					} else {
					}
				}
				TiXmlNode * notifyType = docHandle.FirstChild("Notify").FirstChild("CmdType").ToElement();
				if (NULL != notifyType) {
					char* cmdstr = (char*) notifyType->ToElement()->GetText();
					//	printf("notify type = %s", cmdstr);

					if (0 == strcmp(cmdstr, "Alarm")) {
						//	puts(body->body);
						Response200(m_eCtx, je);
						handleDevAlarm1(je, docHandle);
						return;
					} else if (0 == strcmp(cmdstr, "Keepalive")) {
						handleKeepAlive1(je, docHandle);
						return;
					}
				}

				mxml_node_t *xml = mxmlLoadString(NULL, body->body, MXML_NO_CALLBACK);
				mxml_node_t * CmdTypeNode = mxmlFindElement(xml, xml, "CmdType", NULL, NULL, MXML_DESCEND);

				if (CmdTypeNode != NULL) {

					const char *CmdType = mxmlGetText(CmdTypeNode, NULL);

					{
						//Debug(DEBUG, "   ---> CmdType=%s \n", CmdType);

						Debug(ERROR, "sip message type:%s body=%s", CmdType, body->body);

						if (0 == strcmp(CmdType, "PresetQuery")) {
							handlePresetQuery(je, xml);
						} else if (0 == strcmp(CmdType, "DeviceInfo")) {
							handleDevInfoQuery(je, xml);
						} else if (0 == strcmp(CmdType, "DeviceStatus")) {

							handleDevStatus(je, xml);
						} else if (0 == strcmp(CmdType, "ConfigDownload")) {
							handleConfigDownload(je, xml);
						} else if (0 == strcmp(CmdType, "RecordInfo")) {
							handleRecordResponse(je, xml);
						} else if (0 == strcmp(CmdType, "Alarm")) {

							if (theApp.m_logmask & (1 << BIT_5_QUERY)) {
								Debug(ERROR, "dev info:%s", sipmessage2str(je->request).c_str());
							}
						} else if (0 == strcmp(CmdType, "MobilePosition")) {
							if (theApp.m_logmask & (1 << BIT_5_QUERY)) {
								Debug(ERROR, "dev info:%s", sipmessage2str(je->request).c_str());
							}
						} else {

						}
					}
				}
			} else {
				Debug(ERROR, "no xml body");
			}
		} else if (strcmp(contentType->subtype, "ULU+json") == 0) {

			Response200(m_eCtx, je);
			osip_body_t *body1 = NULL;
			osip_message_get_body(je->request, 0, &body1);

			if (body1 != NULL) {
				char* pdata;
				size_t dlen;
				osip_body_to_str(body1, &pdata, &dlen);
				if (dlen > 0) {
					Json::Reader reader;
					Json::Value jmessage;
					if (reader.parse(string(pdata), jmessage)) {

						string strcmd = jmessage["cmd"].asString();

						//		Debug(ERROR, "inter command:%s\n", strcmd.c_str());

						if (strcmd == "sip_service_check") {
							Json::Value root;
							root["cmd"] = "sip_service_check_ack";
							sendInterJsonMessage(root, 0);
						} else if (strcmd == "call_invite_req") {
							onInterCall(je->request, jmessage["data"]);
						} else if (strcmd == "call_close_req") {
							onInterClosePlay(je->request, jmessage["data"]);
						} else if (strcmd == "reg_auth_ack") {
							onInterRegAuth(je->request, jmessage["data"]);
						} else if (strcmd == "reset_dev_req") {
							string did = jmessage["data"]["did"].asString();
							resetDev(did);
						} else if (strcmd == "reset_all_session") {
							string sipid = jmessage["serverip"].asString();
							resetAllSession(sipid);
						} else if (strcmd == "get_device_information_req") {
							onInterGetDevInfo(je->request, jmessage["data"]);
						} else if (strcmd == "query_from_gbserver_req") {
							onInterRemoteQuery(je->request, jmessage);
						} else if (strcmd == "query_playback_req") {
							onInterGetRecordInfo(je->request, jmessage["data"]);
						} else if (strcmd == "direction_ctrl_req" || strcmd == "stop_ctrl_req" || strcmd == "focus_ctrl_req"
								|| strcmd == "zoom_ctrl_req") {
							onInterPtzCtrl(je->request, jmessage);
						} else if (strcmd == "get_dev_status_req") {
							string did = jmessage["data"]["did"].asString();
							DevIt it = m_map_devs.find(did);
							if (it != m_map_devs.end()) {
								SendQueryDeviceStatus(m_eCtx, *(it->second));
							}

						} else if (strcmd == "get_dev_config_req") {
							string did = jmessage["data"]["did"].asString();
							DevIt it = m_map_devs.find(did);
							if (it != m_map_devs.end()) {
								SendQueryDeviceConfig(m_eCtx, *(it->second));
							}

						} else if (strcmd == "get_dev_catalog_req") {
							string did = jmessage["data"]["did"].asString();
							DevIt it = m_map_devs.find(did);
							if (it != m_map_devs.end()) {
								SendQueryCatalog(m_eCtx, it->second, 10);
							}

						} else if (strcmd == "get_dev_alarm_req") {
							string did = jmessage["data"]["did"].asString();
							DevIt it = m_map_devs.find(did);
							if (it != m_map_devs.end()) {

							}

						} else if (strcmd == "get_dev_mobil_pos_req") {
							string did = jmessage["data"]["did"].asString();
							DevIt it = m_map_devs.find(did);
							if (it != m_map_devs.end()) {

							}

						} else if (strcmd == "get_online_req") {

							vector<string> nvrlist;
							DevIt iter;
							for (iter = m_map_devs.begin(); iter != m_map_devs.end(); iter++) {

								Debug(ERROR, "online dev: sn:%s id:%s tp:%d ip:%s port:%d tm:%d", iter->second->m_uluSN.c_str(),
										iter->second->m_devid.c_str(), iter->second->m_dev_type, iter->second->m_dev_remote_ip.c_str(),
										iter->second->m_dev_remote_port, utc2strtime(iter->second->m_last_keep_time).c_str());

								printf("\nonline dev: sn:%s id:%s tp:%d ip:%s tm:%d\n\n", iter->second->m_uluSN.c_str(),
										iter->second->m_devid.c_str(), iter->second->m_dev_type, iter->second->m_dev_remote_ip.c_str(),
										iter->second->m_last_keep_time);

							}
						}
					}

					osip_free(pdata);
				}
			}
		}
		if (strcmp(contentType->subtype, "ULU+streamjson") == 0) {
			Response200(m_eCtx, je);
			osip_body_t *body = NULL;
			osip_message_get_body(je->request, 0, &body);
			if (body != NULL) {
				puts(body->body);
				Debug(ERROR, "close by streamserver:%s", body->body);
//				Json::Reader reader;
//				Json::Value jmessage;
//				if (reader.parse(body->body, jmessage)) {
//					string devid = jmessage["sn"].asString();
//					int sesType = jmessage["type"].asInt();
//					string option = jmessage["option"].asString();
//
//					sesType += 1;
//					closeSession(devid, sesType, 2);
//
//				}

			}

		}

	}
}
void GB28181Server::handleNotifyAck(eXosip_event_t *je) {
	puts(this->sipmessage2str(je->response).c_str());
}
void GB28181Server::handleNotify(eXosip_event_t *je) {
	Response200(m_eCtx, je);

	puts(this->sipmessage2str(je->request).c_str());

	osip_body_t *body = NULL;
	osip_message_get_body(je->request, 0, &body);
	if (body != NULL) {
		TiXmlDocument doc;
		doc.Parse(body->body);
		TiXmlHandle docHandle(&doc);
		TiXmlNode * notifyType = docHandle.FirstChild("Notify").FirstChild("CmdType").ToElement();
		if (NULL != notifyType) {
			char* cmdstr = (char*) notifyType->ToElement()->GetText();
			if (0 == strcmp(cmdstr, "Catalog")) {
				string notifystr = body->body;
				Debug(INFO, "channel info from notify:%s", theApp.replace_str(notifystr, string("\r\n"), string(" | ")).c_str());

				vector<channelStat> statlist;
				char* pNvrid;
				if (docHandle.FirstChild("Notify").FirstChild("DeviceID").Element() == NULL)
					return;
				TiXmlNode * devinfo = docHandle.FirstChild("Notify").FirstChild("DeviceID").ToElement();
				if (devinfo != NULL) {
					pNvrid = (char*) devinfo->ToElement()->GetText();
					if (pNvrid == NULL) {
						return;
					}
				}
				if (docHandle.FirstChild("Notify").FirstChild("DeviceList").Element() == NULL)
					return;
				TiXmlNode * list = docHandle.FirstChild("Notify").FirstChild("DeviceList").ToElement();
				if (list != NULL) {

					TiXmlNode *nodeEle = list->FirstChild("Item");

					if (nodeEle != NULL) {
						for (; nodeEle != NULL; nodeEle = nodeEle->NextSiblingElement()) {
							if (nodeEle == NULL)
								break;
							char * channelid;
							char * channelstat;
							TiXmlNode *channelidNode = nodeEle->FirstChild("DeviceID");
							if (channelidNode != NULL) {
								channelid = (char*) channelidNode->ToElement()->GetText();
								if (channelid == NULL) {
									continue;
								}
							}
							TiXmlNode *channelstatNode = nodeEle->FirstChild("Event");
							if (channelstatNode != NULL) {
								channelstat = (char*) channelstatNode->ToElement()->GetText();
							}

							int stat = 0;
							if (0 == strcmp(channelstat, "ON"))
								stat = 1;

							channelStat item;
							item.nvrid = pNvrid;
							item.channelid = channelid;
							item.stat = stat;

							statlist.push_back(item);

						}
					}
				}
				if (statlist.size() > 0) {
					onChannelStatChange(statlist);
				}

				return;
			} else {
				puts(this->sipmessage2str(je->request).c_str());
			}
		} else {

			if (docHandle.FirstChild("Response").Element() == NULL)
				return;
			if (docHandle.FirstChild("Response").FirstChild("DeviceID").Element() == NULL)
				return;
			TiXmlNode * devinfo = docHandle.FirstChild("Response").FirstChild("DeviceID").ToElement();
			char * nvrid;
			if (devinfo != NULL) {
				nvrid = (char*) devinfo->ToElement()->GetText();
			}
			if (docHandle.FirstChild("Response").FirstChild("DeviceList").Element() == NULL)
				return;
			TiXmlNode * list = docHandle.FirstChild("Response").FirstChild("DeviceList").ToElement();
			if (list != NULL) {

				TiXmlNode *nodeEle = list->FirstChild("Item");

				vector<channelStat> statlist;
				if (nodeEle != NULL) {
					for (; nodeEle != NULL; nodeEle = nodeEle->NextSiblingElement()) {
						if (nodeEle == NULL)
							break;

						if (nodeEle->FirstChild("DeviceID") != NULL && nodeEle->FirstChild("Status") != NULL) {

							char * channelid = (char*) nodeEle->FirstChild("DeviceID")->ToElement()->GetText();
							char * status = (char*) nodeEle->FirstChild("Status")->ToElement()->GetText();

							if (channelid != NULL && nvrid != NULL) {
								if (strlen(channelid) == 20 && strlen(nvrid) == 20) {
//									DevIt it = this->m_map_devs.find(nvrid);
//									if (it != m_map_devs.end()) {
//										it->second->setChannelStat(channelid, 1);
//									}

									channelStat item;
									item.nvrid = nvrid;
									item.channelid = channelid;
									item.stat = 0;
									if (strcmp(status, "ON") == 0) {
										item.stat = 1;
									}
									statlist.push_back(item);
								}

							}
						}
					}
				}

				if (statlist.size() > 0) {
					onChannelStatChange(statlist);
				}

			}

		}
	}
}
void GB28181Server::handleKeepAlive1(eXosip_event_t *je, TiXmlHandle& docHandle) {
	char *remoteIpAddr = je->remote_ipaddr; //[64] = { 0 };
	int remotePort = je->remote_port;

	if (strcmp(remoteIpAddr, "127.0.0.1") == 0) {
		return;
	}

	osip_via_t * s_via = NULL;
	osip_message_get_via(je->request, 0, &s_via);
	char * ip = osip_via_get_host(s_via);
	char* port = osip_via_get_port(s_via);

	bool isDeviceExist = false;
	bool isReset = false;

	TiXmlNode * DeviceIDNode = docHandle.FirstChild("Notify").FirstChild("DeviceID").ToElement();
	if (DeviceIDNode != NULL) {
		const char *DeviceID;
		DeviceID = (char*) DeviceIDNode->ToElement()->GetText();

		if (theApp.m_logmask & (1 << BIT_3_KEEP)) {
			Debug(VERBOSE, "keepalive:id=%s udp:%s:%d ,sip:%s:%s", DeviceID, remoteIpAddr, remotePort, ip, port);
		}

		DevIt it = m_map_devs.find(DeviceID);
		if (it != m_map_devs.end()) {
			if (it->second->m_dev_status == STAT_RESET) {
				if (it->second->m_dev_remote_ip == remoteIpAddr) {

					it->second->m_dev_remote_port = remotePort;
					it->second->m_dev_status = STAT_REG_OK;
					it->second->m_querySN = theApp.m_SerailNumber++;
					if (it->second->m_dev_type == 2) {
						isReset = true;
					}

				} else {
					return;
				}
			} else {

				if (it->second->m_dev_remote_ip == remoteIpAddr) {

					if (it->second->m_dev_remote_port != remotePort) {
						Debug(ERROR, "udp port update:id=%s ,old %s:%d new %s:%d \n sip ip:%s port:%s", DeviceID,
								it->second->m_dev_remote_ip.c_str(), it->second->m_dev_remote_port, remoteIpAddr, remotePort, ip, port);
						it->second->m_dev_remote_port = remotePort;

					}
				} else {
					Debug(ERROR, "udp ip change:id=%s ,old %s:%d new %s:%d \n sip ip:%s port%s", DeviceID,
							it->second->m_dev_remote_ip.c_str(), it->second->m_dev_remote_port, remoteIpAddr, remotePort, ip, port);
					return;
				}
			}

			it->second->m_last_keep_time = time(NULL);
			if (it->second->m_last_keep_time - it->second->m_last_update_time > ONLINE_EXP_TIME) {
				it->second->m_last_update_time = it->second->m_last_keep_time;
				m_dev_local_cache.updateDev(string(DeviceID));
			}
			isDeviceExist = true;
			Response200(m_eCtx, je);

			if (it->second->m_dev_type == 2 && it->second->m_last_keep_time - it->second->m_last_subscribe_time > 90) {
				it->second->m_last_subscribe_time = it->second->m_last_keep_time;

				//	int removeret = eXosip_subscription_remove(m_eCtx, it->second->m_sub_did);
				SendQueryCatalog(m_eCtx, it->second, it->second->m_querySN);
				Debug(DEBUG, "query catalog:%s ", it->second->m_devid.c_str());
			}

		} else {
//no response
		}
	}

	if (!isDeviceExist) {
		//Response403(m_eCtx, je);
	}
}

void GB28181Server::handleMessageSendFail(eXosip_event_t *je) {
	char *remoteIpAddr = je->remote_ipaddr; //[64] = { 0 };
	int remotePort = je->remote_port;
	if (strcmp(remoteIpAddr, "127.0.0.1") == 0 || theApp.isServerSipIp(remoteIpAddr)) {
		return;
	}

	osip_to_t *hto = osip_message_get_to(je->request);
	osip_cseq_t* hseq = osip_message_get_cseq(je->request);
	if (hseq != NULL) {
		if (strcmp(hseq->method, "BYE") == 0) {
			osip_call_id_t * callid = osip_message_get_call_id(je->request);
			if (callid != NULL) {
				Debug(ERROR, "BYE Response from fail:%s  %s", hto->url->username, callid->number);
				delSessionBycid(hto->url->username, atoi(callid->number));
			}
		} else {
			Debug(ERROR, "Request fail :%s", sipmessage2str(je->request).c_str());
		}
	}

}

void GB28181Server::handleResponse(eXosip_event_t *je) {

	Response200(m_eCtx, je);
	char *remoteIpAddr = je->remote_ipaddr; //[64] = { 0 };
	int remotePort = je->remote_port;
	if (strcmp(remoteIpAddr, "127.0.0.1") == 0 || theApp.isServerSipIp(remoteIpAddr)) {
		return;
	}

	osip_to_t *hto = osip_message_get_to(je->request);
	osip_cseq_t* hseq = osip_message_get_cseq(je->request);
	if (hseq != NULL) {
		if (strcmp(hseq->method, "BYE") == 0) {
			printf("ack----close---%s %s\n", hto->url->host, hseq->number);

			osip_call_id_t * callid = osip_message_get_call_id(je->request);
			if (callid != NULL) {
				Debug(ERROR, "BYE Response from ack:%s  %s", hto->url->username, callid->number);
				delSessionBycid(hto->url->username, atoi(callid->number));
			}
		}
	}

//	osip_body_t *body1 = NULL;
//	osip_message_get_body(je->request, 0, &body1);
//	if (body1 != NULL) {
//		Debug(ERROR, "sip Response : req = %s", body1->body);
//
//	}
//
//	osip_body_t *body = NULL;
//	osip_message_get_body(je->response, 0, &body);
//	if (body != NULL) {
//		Debug(ERROR,"response body is:\n%s\n", body->body);
//	}

}
void GB28181Server::sendInterJson(int agentid, string jsonstr) {
	Json::Value root;
	root["type"] = "json";
	root["agentid"] = agentid;
	root["data"] = jsonstr;

	Json::FastWriter writer;
	string message = writer.write(root);
	if (m_local_com_socket == 0) {
		m_local_com_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_local_com_socket < 0)
			return;
	}
	struct sockaddr_in peerAddr;
	peerAddr.sin_family = AF_INET;
	peerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	peerAddr.sin_port = htons(theApp.m_localUdpPort);
	int ret = sendto(m_local_com_socket, message.c_str(), message.length(), 0, (struct sockaddr*) &peerAddr, sizeof(sockaddr_in));
}

void GB28181Server::sendInterJsonMessage(Json::Value& json, int agentid) {
	char buf[10] = { 0 };
	sprintf(buf, "%d", agentid);
	sendInterJsonMessage(json, buf);
}

void GB28181Server::sendInterJsonMessage(Json::Value& json, char* agentid) {

	Json::Value root;
	root["type"] = "json";
	root["agentid"] = agentid;
	root["data"] = json;

	Json::FastWriter writer;
	string message = writer.write(root);
	if (m_local_com_socket == 0) {
		m_local_com_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_local_com_socket < 0)
			return;
	}
	struct sockaddr_in peerAddr;
	peerAddr.sin_family = AF_INET;
	peerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	peerAddr.sin_port = htons(theApp.m_localUdpPort);
	int ret = sendto(m_local_com_socket, message.c_str(), message.length(), 0, (struct sockaddr*) &peerAddr, sizeof(sockaddr_in));
}
void GB28181Server::sendInterUDPMessage(osip_message_t * msg) {

	Json::Value root;
	root["type"] = "sip";
	root["data"] = sipmessage2str(msg);

	Json::FastWriter writer;
	string message = writer.write(root);

	if (m_local_com_socket == 0) {
		m_local_com_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_local_com_socket < 0)
			return;
	}
	struct sockaddr_in peerAddr;
	peerAddr.sin_family = AF_INET;
	peerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	peerAddr.sin_port = htons(theApp.m_localUdpPort);
	int ret = sendto(m_local_com_socket, message.c_str(), message.length(), 0, (struct sockaddr*) &peerAddr, sizeof(sockaddr_in));
}

void GB28181Server::Response(struct eXosip_t * peCtx, eXosip_event_t *je, int value) {
	int iRet = 0;
	osip_message_t * pSRegister = NULL;
	iRet = eXosip_message_build_answer(peCtx, je->tid, value, &pSRegister);
	if (iRet == 0 && pSRegister != NULL) {
		eXosip_lock(peCtx);
		eXosip_message_send_answer(peCtx, je->tid, value, pSRegister);
		eXosip_unlock(peCtx);
	}
}

void GB28181Server::Response403(struct eXosip_t * peCtx, eXosip_event_t *je) {
	Response(peCtx, je, 403);
}

void GB28181Server::Response200(struct eXosip_t * peCtx, eXosip_event_t *je) {
	Response(peCtx, je, 200);
}
string GB28181Server::sipmessage2str(osip_message_t * msg) {
	char* pmsgstr;
	size_t len;
	osip_message_to_str(msg, &pmsgstr, &len);
	string resp = pmsgstr;
	osip_free(pmsgstr);

	return resp;
}
string GB28181Server::utc2strtime(int utctm) {
	char stbuf[32] = { 0 };

	time_t t = utctm;
	struct tm* l = localtime(&t);
	strftime(stbuf, 32, "%Y-%m-%dT%H:%M:%S", l);

	return string(stbuf);

}
CameraDevice* GB28181Server::findChannel(string channelid, int& devstat) {
	DevIt iter = m_map_devs.begin();
	while (iter != m_map_devs.end()) {
		int stat = iter->second->findChannelStat1(channelid);
		if (stat == -1) {
			iter++;
			continue;
		} else {
			devstat = stat;
			return iter->second;
		}
	}
	return NULL;
}

