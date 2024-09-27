/*
 * GB28181Server_alarm.cpp
 *
 *  Created on: Mar 9, 2020
 *      Author: wyf
 */

#include "GB28181Server.h"

void GB28181Server::handleDevAlarm1(eXosip_event_t *je, TiXmlHandle& docHandle) {
	const char *DeviceID;
	const char *SN;

	int alarmMethed;
	int alarmType;
	string alarmtime;

	osip_from_t *hfrom = osip_message_get_from(je->request);
	char * fromid = hfrom->url->username;

	//osip_to_param_get_byname
	osip_uri_param_t * ptag;
	osip_uri_param_get_byname(&(hfrom->gen_params), "tag", &ptag);
	if (ptag != NULL) {
		//printf("from header:%s %s\n", ptag->gname, ptag->gvalue);
	}

	TiXmlNode * DeviceIDNode = docHandle.FirstChild("Notify").FirstChild("DeviceID").ToElement();
	if (DeviceIDNode != NULL) {
		DeviceID = (char*) DeviceIDNode->ToElement()->GetText();

	}

	{
		TiXmlNode * node = docHandle.FirstChild("Notify").FirstChild("SN").ToElement();
		if (node != NULL) {
			SN = (char*) node->ToElement()->GetText();
		}
	}
	{
		TiXmlNode * node = docHandle.FirstChild("Notify").FirstChild("AlarmPriority").ToElement();
		if (node != NULL) {
			//SN = (char*) node->ToElement()->GetText();
		}
	}
	{
		TiXmlNode * node = docHandle.FirstChild("Notify").FirstChild("AlarmTime").ToElement();
		if (node != NULL) {
			alarmtime = (char*) node->ToElement()->GetText();
		}
	}
	{
		TiXmlNode * node = docHandle.FirstChild("Notify").FirstChild("AlarmMethod").ToElement();
		if (node != NULL) {
			char *AlarmMethod = (char*) node->ToElement()->GetText();
			alarmMethed = atoi(AlarmMethod);

		}
	}
	{
		TiXmlNode * node = docHandle.FirstChild("Notify").FirstChild("Info").ToElement();
		if (node != NULL) {

			TiXmlNode * node1 = docHandle.FirstChild("Notify").FirstChild("Info").FirstChild("AlarmType").ToElement();
			if (node1 != NULL) {
				const char *AlarmType = (char*) node1->ToElement()->GetText();
				alarmType = atoi(AlarmType);
			}

		}
	}

	DevIt it = m_map_devs.find(DeviceID);
	if (it != m_map_devs.end()) {
		SendAlarmResponse(m_eCtx, je, DeviceID, SN, (char*) it->second->m_dev_remote_ip.c_str(), it->second->m_dev_remote_port);
	}

	printf("++++++++++++++++++++= alarm++++++++++++++ %d  %d ++", alarmMethed, alarmType);

	{

		DevIt it = m_map_devs.find(fromid);
		if (it != m_map_devs.end()) {
			if (it->second->m_uluSN != "-1") {

				Json::Value root;
				root["method"] = alarmMethed;
				root["type"] = alarmType;
				root["time"] = alarmtime;
				root["sn"] = it->second->m_uluSN;
				root["did"] = it->second->m_devid;
				Json::FastWriter writer;
				string message = writer.write(root);
				if (theApp.m_logmask & (1 << BIT_7_MOTION)) {
					Debug(VERBOSE, "dev alarm: %s", message.c_str());
				}
				Message* msg = new Message(message, MSG_JSON, CMD_ALARM);
				dispatchMessage(msg);

			}
		}

	}
}


int GB28181Server::SendAlarmResponse(struct eXosip_t * peCtx, eXosip_event_t *je, const char * deviceId, const char * sn,
		char * platformIpAddr, int platformSipPort) {
	int ret;
	mxml_node_t *tree, *query, *node;

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Response");
		if (query != NULL) {
			char buf[256] = { 0 };
			char dest_call[256], source_call[256];
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "Alarm");
			node = mxmlNewElement(query, "SN");
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, deviceId);
			node = mxmlNewElement(query, "Result");
			mxmlNewText(node, 0, "OK");
			mxmlSaveString(tree, buf, 256, whitespace_cb);

			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", deviceId, platformIpAddr, platformSipPort);
			snprintf(source_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(),
					theApp.m_sipserver_port);
			ret = eXosip_message_build_request(peCtx, &message, "MESSAGE", dest_call, source_call, NULL);
			if (ret == 0 && message != NULL) {
				osip_message_set_body(message, buf, strlen(buf));
				osip_message_set_content_type(message, "Application/MANSCDP+xml");
				eXosip_lock(peCtx);
				eXosip_message_send_request(peCtx, message);
				eXosip_unlock(peCtx);
			} else {
				Debug(ERROR, "eXosip_message_build_request failed!\n");
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

