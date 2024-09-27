/*
 * GB28181Server_record.cpp
 *
 *  Created on: Mar 31, 2020
 *      Author: wyf
 */

#include "GB28181Server.h"

int GB28181Server::SendRecordInfoCmd(struct eXosip_t *peCtx, CameraDevice* deviceNode, string channelID, string clientid, string starttime,
		string endtime) {
	Debug(INFO, "SendRecordInfoCmd %s,%s\n", starttime.c_str(), endtime.c_str());

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Query");
		if (query != NULL) {
			char buf[256] = { 0 };
			char dest_call[256], source_call[256];
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "RecordInfo");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", theApp.m_SerailNumber++);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, channelID.c_str());
			node = mxmlNewElement(query, "StartTime");
			mxmlNewText(node, 0, starttime.c_str());
			//mxmlNewCDATA(node,starttime);
			node = mxmlNewElement(query, "EndTime");
			mxmlNewText(node, 0, endtime.c_str());
			//mxmlNewCDATA(node,endtime);
			node = mxmlNewElement(query, "Type");
			mxmlNewText(node, 0, "time");
			mxmlSaveString(tree, buf, 256, whitespace_cb);

			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", channelID.c_str(), deviceNode->m_dev_remote_ip.c_str(), deviceNode->m_dev_remote_port);
			snprintf(source_call, 256, "sip:%s@%s:%d", clientid.c_str(), theApp.m_sipserver_ip.c_str(), theApp.m_sipserver_port);
			ret = eXosip_message_build_request(peCtx, &message, "MESSAGE", dest_call, source_call, NULL);
			if (ret == 0 && message != NULL) {

				char tmp[200] = { 0 };
				snprintf(tmp, 200, "SIP/2.0/UDP %s:%d;rport;branch=z9hG4bK%u", theApp.m_sipserver_ip.c_str(), theApp.m_sipserver_port,
						osip_build_random_number());
				osip_message_set_via(message, tmp);
				osip_message_set_body(message, buf, strlen(buf));
				osip_message_set_content_type(message, "Application/MANSCDP");
				eXosip_lock(peCtx);
				eXosip_message_send_request(peCtx, message);
				eXosip_unlock(peCtx);

			} else {
				Debug(ERROR, "eXosip_message_build_request failed:query record id=%s\n", channelID.c_str());
			}
		} else {
			Debug(ERROR,"mxmlNewElement Query failed!\n");
		}
		mxmlDelete(tree);
	} else {
		Debug(ERROR,"mxmlNewXML failed!\n");
	}

	return 0;
}
void GB28181Server::onInterGetRecordInfo(osip_message_t * msg, Json::Value& data) {
	osip_from_t *from = osip_message_get_from(msg);
	char* strid = from->url->username;
	string did = data["did"].asString();
	string nvrid = data["nvrid"].asString();
	if (nvrid.empty() || nvrid.length() < 20) {
		nvrid = did;
	}
	string clientid = data["clientid"].asString();
	string starttime = data["starttime"].asString();
	string endtime = data["endtime"].asString();

	char stbuf[32] = { 0 };
	char edbuf[32] = { 0 };
	{
		time_t t = atoi(starttime.c_str());
		struct tm* l = localtime(&t);
		strftime(stbuf, 32, "%Y-%m-%dT%H:%M:%S", l);

	}
	{
		time_t t = atoi(endtime.c_str());
		struct tm* l = localtime(&t);
		strftime(edbuf, 32, "%Y-%m-%dT%H:%M:%S", l);
	}

	puts(stbuf);

	DevIt it = this->m_map_devs.find(nvrid);
	if (it != m_map_devs.end()) {
		SendRecordInfoCmd(m_eCtx, it->second, did, strid, stbuf, edbuf);

//		int size = it->second->m_channels.size();
//		bool found = false;
//		for (int i = 0; i < size; i++) {
//			if (it->second->m_channels[i] == did) {
//				SendRecordInfoCmd(m_eCtx, it->second, did, strid, stbuf, edbuf);
//				found = true;
//			}
//		}
//		if (!found) {
//			Json::Value root;
//			root["cmd"] = "query_playback_ack";
//			root["type"] = "playback";
//			root["did"] = did;
//			root["nvrid"] = nvrid;
//			root["result"] = "nochannel";
//
//			sendInterJsonMessage(root, strid);
//		}

	} else {
		Json::Value root;
		root["cmd"] = "query_playback_ack";
		root["type"] = "playback";
		root["did"] = did;
		root["nvrid"] = nvrid;
		root["result"] = "offline";

		sendInterJsonMessage(root, strid);
	}
}
void GB28181Server::handleRecordResponse1(eXosip_event_t *je, TiXmlHandle& handle) {
	Response200(m_eCtx, je);
	osip_to_t *hto = osip_message_get_to(je->request);
	if (theApp.m_logmask & (1 << BIT_4_RECORDINFO)) {
		Debug(INFO, "record query ok:%s", sipmessage2str(je->request).c_str());
	} else {
		Debug(ERROR, "record query ok");
	}

	if (theApp.m_sipserver_id != hto->url->username) {
		sendInterUDPMessage(je->request);
		return;
	}
}
void GB28181Server::handleRecordResponse(eXosip_event_t *je, mxml_node_t* xml) {

	Response200(m_eCtx, je);
	osip_to_t *hto = osip_message_get_to(je->request);
	Debug(ERROR, "record query ok:");
	if (theApp.m_sipserver_id != hto->url->username) {
		sendInterUDPMessage(je->request);
		return;
	}

	osip_body_t *body = NULL;
	osip_message_get_body(je->request, 0, &body);
	if (body != NULL) {
		mxml_node_t *xml = mxmlLoadString(NULL, body->body, MXML_NO_CALLBACK);
		mxml_node_t * DeviceIDNode = mxmlFindElement(xml, xml, "DeviceID", NULL, NULL, MXML_DESCEND); //must

		if (DeviceIDNode != NULL) {
			const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);
		}

		mxml_node_t * NameNode = mxmlFindElement(xml, xml, "Name", NULL, NULL, MXML_DESCEND); //must
		if (NameNode != NULL) {
			const char *Name = mxmlGetText(NameNode, NULL);
			printf("Name is %s", Name);
		}

		mxml_node_t * SumNumNode = mxmlFindElement(xml, xml, "SumNum", NULL, NULL, MXML_DESCEND); //must
		int SumNum = 0;

		if (SumNumNode != NULL) {
			SumNum = mxmlGetInteger(SumNumNode);
			printf("SumNum is %d", SumNum);
		}

		mxml_node_t * RecordListNode = mxmlFindElement(xml, xml, "RecordList", NULL, NULL, MXML_DESCEND);

		if (RecordListNode != NULL) {
			const char *Num = mxmlElementGetAttr(RecordListNode, "Num");
			int count = atoi(Num);

			mxml_node_t * ItemNode = mxmlFindElement(RecordListNode, RecordListNode, "Item", NULL, NULL, MXML_DESCEND);

			for (int i = 0; i < count; i++) {
				if (i != 0)
					ItemNode = mxmlFindElement(ItemNode, RecordListNode, "Item", NULL, NULL, MXML_DESCEND);

				if (ItemNode != NULL) {
					mxml_node_t * NameNode = mxmlFindElement(ItemNode, ItemNode, "Name", NULL, NULL, MXML_DESCEND);
					mxml_node_t * StarttimeNode = mxmlFindElement(ItemNode, ItemNode, "StartTime", NULL, NULL, MXML_DESCEND);
					mxml_node_t * EndtimeNode = mxmlFindElement(ItemNode, ItemNode, "EndTime", NULL, NULL, MXML_DESCEND);

					const char *Name = mxmlGetText(NameNode, NULL);
					const char *starttime = mxmlGetCDATA(StarttimeNode);
					const char *endtime = mxmlGetCDATA(EndtimeNode);

				}
			}
		}

	}

}

