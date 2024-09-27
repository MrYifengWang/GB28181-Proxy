/*
 * GB28181Server_reg.cpp
 *
 *  Created on: Mar 4, 2020
 *      Author: wyf
 */

#include "GB28181Server.h"

int GB28181Server::SendCatlogSubRefresh(CameraDevice* deviceNode) {
	osip_message_t *message = NULL;

	int ret = eXosip_subscription_build_refresh_request(m_eCtx, 1, &message);
	if (ret == 0 && message != NULL) {
		eXosip_lock(m_eCtx);
		eXosip_subscription_send_refresh_request(m_eCtx, 1, message);
		eXosip_unlock(m_eCtx);
	}
	return 0;
}

int GB28181Server::SendCatlogSubscribe(struct eXosip_t *peCtx, CameraDevice* deviceNode, int querysn) {

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

	const char *deviceId = deviceNode->m_devid.c_str();
	const char *platformIpAddr = deviceNode->m_dev_remote_ip.c_str();
	int platformSipPort = deviceNode->m_dev_remote_port;

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Query");
		if (query != NULL) {
			char buf[512] = { 0 };
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "Catalog");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", querysn);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, deviceId);
//			node = mxmlNewElement(query, "StartTime");
//			int curtime = time(NULL);
//			curtime = curtime - 7200;
//			mxmlNewText(node, 0, utc2strtime(curtime).c_str());
//			node = mxmlNewElement(query, "EndTime");
//			mxmlNewText(node, 0, utc2strtime(curtime + 7200).c_str());
			mxmlSaveString(tree, buf, 512, whitespace_cb);

			char dest_call[256], source_call[256];

			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", deviceId, platformIpAddr, platformSipPort);
			snprintf(source_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(),
					theApp.m_sipserver_port);
			//	printf("catalog:%s\n%s\n", dest_call, source_call);
//			{
//				ret = eXosip_message_build_request(peCtx, &message, "SUBSCRIBE", dest_call, source_call, NULL);
//				if (ret == 0 && message != NULL) {
//					osip_message_set_body(message, buf, strlen(buf));
//					osip_message_set_content_type(message, "Application/MANSCDP");
//					eXosip_lock(peCtx);
//					eXosip_message_send_request(peCtx, message);
//					eXosip_unlock(peCtx);
//					Debug(ERROR,"xml:%s, dest_call:%s, source_call:%s, ok", buf, dest_call, source_call);
//				} else {
//					Debug(ERROR,"eXosip_message_build_request failed!\n");
//				}
//			}

			ret = eXosip_subscription_build_initial_subscribe(peCtx, &message, dest_call, source_call, NULL, "Catalog", 600);
			if (ret == 0 && message != NULL) {
				osip_message_set_body(message, buf, strlen(buf));
				osip_message_set_content_type(message, "Application/MANSCDP+xml");
				eXosip_lock(peCtx);
				int sendret = eXosip_subscription_send_initial_request(peCtx, message);
				eXosip_unlock(peCtx);

				deviceNode->m_last_subscribe_time = time(NULL);
				deviceNode->m_sub_did = sendret;

				Debug(ERROR, "subscribe:%s %d", deviceId, sendret);
				printf("subscribe:%s %d\n", deviceId, sendret);

			} else {
				Debug(ERROR, "eXosip_subscription_send_initial_request failed!\n");
			}
		} else {
			Debug(ERROR, "mxmlNewElement Query failed!\n");
		}
		mxmlDelete(tree);
	} else {
		Debug(ERROR, "mxmlNewXML failed!\n");
	}

	return 0;
}
int GB28181Server::SendCatlogSubscribe1(struct eXosip_t *peCtx, CameraDevice* deviceNode, int querysn) {

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

	const char *deviceId = deviceNode->m_devid.c_str();
	const char *platformIpAddr = deviceNode->m_dev_remote_ip.c_str();
	int platformSipPort = deviceNode->m_dev_remote_port;

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Query");
		if (query != NULL) {
			char buf[512] = { 0 };
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "DeviceStatus");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", querysn);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, deviceId);
//			node = mxmlNewElement(query, "StartTime");
//			int curtime = time(NULL);
//			curtime = curtime - 7200;
//			mxmlNewText(node, 0, utc2strtime(curtime).c_str());
//			node = mxmlNewElement(query, "EndTime");
//			mxmlNewText(node, 0, utc2strtime(curtime + 7200).c_str());
			mxmlSaveString(tree, buf, 512, whitespace_cb);

			char dest_call[256], source_call[256];

			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", deviceId, platformIpAddr, platformSipPort);
			snprintf(source_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(),
					theApp.m_sipserver_port);
			//	printf("catalog:%s\n%s\n", dest_call, source_call);
//			{
//				ret = eXosip_message_build_request(peCtx, &message, "SUBSCRIBE", dest_call, source_call, NULL);
//				if (ret == 0 && message != NULL) {
//					osip_message_set_body(message, buf, strlen(buf));
//					osip_message_set_content_type(message, "Application/MANSCDP");
//					eXosip_lock(peCtx);
//					eXosip_message_send_request(peCtx, message);
//					eXosip_unlock(peCtx);
//					Debug(ERROR,"xml:%s, dest_call:%s, source_call:%s, ok", buf, dest_call, source_call);
//				} else {
//					Debug(ERROR,"eXosip_message_build_request failed!\n");
//				}
//			}

			ret = eXosip_subscription_build_initial_subscribe(peCtx, &message, dest_call, source_call, NULL, "DeviceStatus", 600);
			if (ret == 0 && message != NULL) {
				osip_message_set_body(message, buf, strlen(buf));
				osip_message_set_content_type(message, "Application/MANSCDP+xml");
				eXosip_lock(peCtx);
				int sendret = eXosip_subscription_send_initial_request(peCtx, message);
				eXosip_unlock(peCtx);

				deviceNode->m_last_subscribe_time = time(NULL);
				deviceNode->m_sub_did = sendret;

				Debug(ERROR, "subscribe:%s %d", deviceId, sendret);
				printf("subscribe:%s %d\n", deviceId, sendret);

			} else {
				Debug(ERROR, "eXosip_subscription_send_initial_request failed!\n");
			}
		} else {
			Debug(ERROR, "mxmlNewElement Query failed!\n");
		}
		mxmlDelete(tree);
	} else {
		Debug(ERROR, "mxmlNewXML failed!\n");
	}

	return 0;
}
void GB28181Server::handleCatalogResponse1(eXosip_event_t *je, TiXmlHandle& handle) {
	Response200(m_eCtx, je);

	osip_to_t *hto = osip_message_get_to(je->request);
	osip_body_t *body = NULL;
	osip_message_get_body(je->request, 0, &body);
	if (body != NULL) {
		TiXmlDocument doc;
		doc.Parse(body->body);
		string catalogstr(body->body);
		catalogstr = theApp.replace_str(catalogstr, string("\r"), string(""));
		catalogstr = theApp.replace_str(catalogstr, string("\n"), string(""));

		Debug(DEBUG, "channel info from catalog:%s", catalogstr.c_str());

		TiXmlHandle docHandle(&doc);
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

void GB28181Server::onChannelStatChange(vector<channelStat>& list) {
	for (int i = 0; i < list.size(); i++) {
		Json::Value root;
		root["nvrid"] = list[i].nvrid;
		root["channelid"] = list[i].channelid;
		root["stat"] = list[i].stat;

		DevIt it = this->m_map_devs.find(list[i].nvrid);
		//	bool isStatChanged = false;
		int channelStat = 0;
		if (it != m_map_devs.end()) {
			//Debug(INFO, "channel check: nvrid:%s size %d %s", list[i].nvrid.c_str(), m_map_devs.size(), it->second->m_devid.c_str());
			m_lock.lock();
			channelStat = it->second->setChannelStat(list[i].channelid, list[i].stat);
			m_lock.unlock();
		}

//		isStatChanged = true;

		if (theApp.isChannelID(list[i].channelid) && (channelStat > 0)) {

			Json::FastWriter writer;
			string message = writer.write(root);
			Debug(INFO, "channel stat change:%s", message.c_str());

			Message* msg = new Message(message, MSG_JSON, CMD_CHANNELSTAT);
			dispatchMessage(msg);
		} else {
			Debug(VERBOSE, "channel stat is devid:%s :%s :%d", list[i].nvrid.c_str(), list[i].channelid.c_str(), list[i].stat);
		}

		if (theApp.isChannelID(list[i].channelid) && (channelStat == 1)) {
			//onDevStatChange(list[i].channelid, 1, list[i].stat);
		}
	}

}
