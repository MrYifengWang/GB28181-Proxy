/*
 * GB28181Server_ctrl.cpp
 *
 *  Created on: Mar 9, 2020
 *      Author: wyf
 */

#include "GB28181Server.h"

void GB28181Server::onInterPtzCtrl(osip_message_t * msg, Json::Value& data) {
	osip_from_t *from = osip_message_get_from(msg);
	char* strid = from->url->username;

	string did = data["data"]["did"].asString();
	string channelid = did;
	if (data["data"]["channel"].isString()) {
		string tmpstr = data["data"]["channel"].asString();
		if (tmpstr.length() == 20) {
			channelid = data["data"]["channel"].asString();
		}
	}
	DevIt it = this->m_map_devs.find(did);
	if (it == m_map_devs.end()) {
		return;
	}

	unsigned char ptzCmd[10] = { 0 };

	string cmd = data["cmd"].asString();
	if (cmd == "direction_ctrl_req") {
		int speed = data["data"]["speed"].asInt();
		string dir = data["data"]["direction"].asString();
		char dirCmd;
		if (dir == "top") {
			dirCmd = 0x08;
		} else if (dir == "bottom") {
			dirCmd = 0x04;
		} else if (dir == "left") {
			dirCmd = 0x02;
		} else if (dir == "right") {
			dirCmd = 0x01;
		} else if (dir == "left_top") {
			dirCmd = 0x0A;
		} else if (dir == "right_top") {
			dirCmd = 0x09;
		} else if (dir == "left_bottom") {
			dirCmd = 0x06;
		} else if (dir == "right_bottom") {
			dirCmd = 0x05;
		}
		speed = (255 / 6) * (speed + 1);
		//A50F4D1000001021
		ptzCmd[0] = 0xA5;
		ptzCmd[1] = (((ptzCmd[0] & 0xF0) >> 4) + (ptzCmd[0] & 0x0F)) % 16;
		ptzCmd[2] = 0x0;
		ptzCmd[3] = dirCmd;
		ptzCmd[4] = speed;
		ptzCmd[5] = speed;
		ptzCmd[6] = 0x0;
		ptzCmd[7] = (ptzCmd[0] + ptzCmd[1] + ptzCmd[2] + ptzCmd[3] + ptzCmd[4] + ptzCmd[5] + ptzCmd[6]) % 256;
		SendPTZCmd(m_eCtx, it->second, channelid, strid, ptzCmd);

	} else if (cmd == "stop_ctrl_req") {
		ptzCmd[0] = 0xA5;
		ptzCmd[1] = (((ptzCmd[0] & 0xF0) >> 4) + (ptzCmd[0] & 0x0F)) % 16;
		ptzCmd[2] = 0x0;
		ptzCmd[3] = 0x0;
		ptzCmd[4] = 0x0;
		ptzCmd[5] = 0x0;
		ptzCmd[6] = 0x0;
		ptzCmd[7] = (ptzCmd[0] + ptzCmd[1] + ptzCmd[2] + ptzCmd[3] + ptzCmd[4] + ptzCmd[5] + ptzCmd[6]) % 256;
		SendPTZCmd(m_eCtx, it->second, channelid, strid, ptzCmd);
		/*
		 {
		 ptzCmd[0] = 0xA5;
		 ptzCmd[1] = (((ptzCmd[0] & 0xF0) >> 4) + (ptzCmd[0] & 0x0F)) % 16;
		 ptzCmd[2] = 0x0;
		 ptzCmd[3] = 0x40;
		 ptzCmd[4] = 0x0;
		 ptzCmd[5] = 0x0;
		 ptzCmd[6] = 0x0;
		 ptzCmd[7] = (ptzCmd[0] + ptzCmd[1] + ptzCmd[2] + ptzCmd[3] + ptzCmd[4] + ptzCmd[5] + ptzCmd[6]) % 256;
		 SendFcousCmd(m_eCtx, it->second, strid, ptzCmd);
		 }
		 */
	} else if (cmd == "focus_ctrl_req") {
		int scale = data["data"]["focus"].asInt();
		int speed = data["data"]["speed"].asInt();

		ptzCmd[0] = 0xA5;
		ptzCmd[1] = (((ptzCmd[0] & 0xF0) >> 4) + (ptzCmd[0] & 0x0F)) % 16;
		ptzCmd[2] = 0x0;
		ptzCmd[3] = scale == 1 ? 0x41 : 0x42;
		ptzCmd[4] = (255 / 6) * speed;
		ptzCmd[5] = 0x0;
		ptzCmd[6] = 0x0;
		ptzCmd[7] = (ptzCmd[0] + ptzCmd[1] + ptzCmd[2] + ptzCmd[3] + ptzCmd[4] + ptzCmd[5] + ptzCmd[6]) % 256;
		SendFcousCmd(m_eCtx, it->second, channelid, strid, ptzCmd);

	} else if (cmd == "zoom_ctrl_req") {
		int zoom = data["data"]["zoom"].asInt();
		int speed = data["data"]["speed"].asInt();
		speed = (15 / 6) * (speed + 1);

		ptzCmd[0] = 0xA5;
		ptzCmd[1] = (((ptzCmd[0] & 0xF0) >> 4) + (ptzCmd[0] & 0x0F)) % 16;
		ptzCmd[2] = 0x0;
		ptzCmd[3] = zoom == 1 ? 0x20 : 0x10;
		ptzCmd[4] = 0;
		ptzCmd[5] = 0;
		ptzCmd[6] = speed;
		ptzCmd[7] = (ptzCmd[0] + ptzCmd[1] + ptzCmd[2] + ptzCmd[3] + ptzCmd[4] + ptzCmd[5] + ptzCmd[6]) % 256;
		SendPTZCmd(m_eCtx, it->second, channelid, strid, ptzCmd);

	}

}

//
int GB28181Server::SendPTZCmd(struct eXosip_t *peCtx, CameraDevice *deviceNode, string channelid, string agentid, unsigned char* ptzCmd) {

	char ptz[20] = { 0 };
	sprintf(ptz, "%02X%02X%02X%02X%02X%02X%02X%02X", ptzCmd[0], ptzCmd[1], ptzCmd[2], ptzCmd[3], ptzCmd[4], ptzCmd[5], ptzCmd[6],
			ptzCmd[7]);

	fprintf(stderr, "sendPTZCmd %s\n", ptz);

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

//	char *deviceId = channelid; //deviceNode->m_devid.c_str();
	const char *platformIpAddr = deviceNode->m_dev_remote_ip.c_str();
	int platformSipPort = deviceNode->m_dev_remote_port;

	Debug(INFO, "sendPTZCmd to %s: %s\n", channelid.c_str(), ptz);

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Control");
		if (query != NULL) {
			char buf[256] = { 0 };
			char dest_call[256], source_call[256];
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "DeviceControl");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", theApp.m_SerailNumber++);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, channelid.c_str());
			node = mxmlNewElement(query, "PTZCmd");
			mxmlNewText(node, 0, ptz);
			mxmlSaveString(tree, buf, 256, whitespace_cb);

			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", channelid.c_str(), platformIpAddr, platformSipPort);
			snprintf(source_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(),
					theApp.m_sipserver_port);
			ret = eXosip_message_build_request(peCtx, &message, "MESSAGE", dest_call, source_call, NULL);
			if (ret == 0 && message != NULL) {
				osip_message_set_body(message, buf, strlen(buf));
				osip_message_set_content_type(message, "Application/MANSCDP");
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
int GB28181Server::SendFcousCmd(struct eXosip_t *peCtx, CameraDevice *deviceNode, string channelid, string agentid, unsigned char* ptzCmd) {

	char ptz[20] = { 0 };
	sprintf(ptz, "%02X%02X%02X%02X%02X%02X%02X%02X", ptzCmd[0], ptzCmd[1], ptzCmd[2], ptzCmd[3], ptzCmd[4], ptzCmd[5], ptzCmd[6],
			ptzCmd[7]);

	fprintf(stderr, "sendPTZCmd %s\n", ptz);

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

	//const char *deviceId = channelid; //deviceNode->m_devid.c_str();
	const char *platformIpAddr = deviceNode->m_dev_remote_ip.c_str();
	int platformSipPort = deviceNode->m_dev_remote_port;

	Debug(ERROR, "sendPTZCmd to %s: %s\n", channelid.c_str(), ptz);

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Control");
		if (query != NULL) {
			char buf[256] = { 0 };
			char dest_call[256], source_call[256];
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "DeviceControl");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", theApp.m_SerailNumber++);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, channelid.c_str());
			node = mxmlNewElement(query, "PTZCmd");
			mxmlNewText(node, 0, ptz);
			mxmlSaveString(tree, buf, 256, whitespace_cb);

			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", channelid.c_str(), platformIpAddr, platformSipPort);
			snprintf(source_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(),
					theApp.m_sipserver_port);
			ret = eXosip_message_build_request(peCtx, &message, "MESSAGE", dest_call, source_call, NULL);
			if (ret == 0 && message != NULL) {
				osip_message_set_body(message, buf, strlen(buf));
				osip_message_set_content_type(message, "Application/MANSCDP");
				eXosip_lock(peCtx);
				eXosip_message_send_request(peCtx, message);
				eXosip_unlock(peCtx);
			} else {
				Debug(ERROR,"eXosip_message_build_request failed!\n");
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
int GB28181Server::SendPresetQuery(struct eXosip_t *peCtx, CameraDevice deviceNode) {
	fprintf(stderr, "SendPresetQuery\n");

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

	const char *deviceId = deviceNode.m_devid.c_str();

	const char *platformIpAddr = deviceNode.m_dev_remote_ip.c_str();
	int platformSipPort = deviceNode.m_dev_remote_port;

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Query");
		if (query != NULL) {
			char buf[256] = { 0 };
			char dest_call[256], source_call[256];
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "PresetQuery");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", theApp.m_SerailNumber++);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, deviceId);
			mxmlSaveString(tree, buf, 256, whitespace_cb);

			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), platformIpAddr, platformSipPort);
			snprintf(source_call, 256, "sip:%s@%s:%d", theApp.m_sipserver_id.c_str(), theApp.m_sipserver_ip.c_str(),
					theApp.m_sipserver_port);
			ret = eXosip_message_build_request(peCtx, &message, "MESSAGE", dest_call, source_call, NULL);
			if (ret == 0 && message != NULL) {
				osip_message_set_body(message, buf, strlen(buf));
				osip_message_set_content_type(message, "Application/MANSCDP");
				eXosip_lock(peCtx);
				eXosip_message_send_request(peCtx, message);
				eXosip_unlock(peCtx);
			} else {
				Debug(ERROR,"eXosip_message_build_request failed!\n");
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

void GB28181Server::handlePresetQuery(eXosip_event_t *je, mxml_node_t* xml) {

	Response200(m_eCtx, je);
	if (theApp.m_logmask & (1 << BIT_5_QUERY)) {
		Debug(ERROR, "dev preset:%s", sipmessage2str(je->request).c_str());

	}
	return;
	mxml_node_t * DeviceIDNode = mxmlFindElement(xml, xml, "DeviceID", NULL, NULL, MXML_DESCEND); //must

	if (DeviceIDNode != NULL) {
		const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);

	}

	mxml_node_t * PresetListNode = mxmlFindElement(xml, xml, "PresetList", NULL, NULL, MXML_DESCEND); //option

	if (PresetListNode != NULL) {
		const char *Num = mxmlElementGetAttr(PresetListNode, "Num");
		int count = atoi(Num);

		mxml_node_t * ItemNode = mxmlFindElement(PresetListNode, PresetListNode, "Item", NULL, NULL, MXML_DESCEND);
		;

		for (int i = 0; i < count; i++) {
			if (i != 0)
				ItemNode = mxmlFindElement(ItemNode, PresetListNode, "Item", NULL, NULL, MXML_DESCEND);

			if (ItemNode != NULL) {
				mxml_node_t * PresetIDNode = mxmlFindElement(ItemNode, ItemNode, "PresetID", NULL, NULL, MXML_DESCEND);
				mxml_node_t * PresetNameNode = mxmlFindElement(ItemNode, ItemNode, "PresetName", NULL, NULL, MXML_DESCEND);

				const char *PresetID = mxmlGetText(PresetIDNode, NULL);
				const char *PresetName = mxmlGetText(PresetNameNode, NULL);

			}
		}

	}

}
