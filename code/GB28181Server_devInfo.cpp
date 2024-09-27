/*
 * GB28181Server_devInfo.cpp
 *
 *  Created on: Mar 4, 2020
 *      Author: wyf
 */

#include "GB28181Server.h"
const char * GB28181Server::whitespace_cb(mxml_node_t *node, int where) {
	return NULL;
}

int GB28181Server::SendQueryDeviceStatus(struct eXosip_t *peCtx, CameraDevice deviceNode) {
	fprintf(stderr, "SendQueryDeviceStatus\n");

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
			mxmlNewText(node, 0, "DeviceStatus");
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

int GB28181Server::SendQueryDeviceInfo(struct eXosip_t *peCtx, CameraDevice* deviceNode, char* agentid) {
	fprintf(stderr, "SendQueryDeviceInfo\n");

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

	const char *deviceId = deviceNode->m_devid.c_str();

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Query");
		if (query != NULL) {
			char buf[256] = { 0 };
			char dest_call[256], source_call[256];
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "DeviceInfo");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", theApp.m_SerailNumber++);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, deviceId);
			mxmlSaveString(tree, buf, 256, whitespace_cb);
			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", deviceId, deviceNode->m_dev_remote_ip.c_str(), deviceNode->m_dev_remote_port);
			snprintf(source_call, 256, "sip:%s@%s:%d", agentid, theApp.m_sipserver_ip.c_str(), theApp.m_sipserver_port);
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

int GB28181Server::SendQueryDeviceAlarm(struct eXosip_t *peCtx, CameraDevice deviceNode) {
	fprintf(stderr, "SendQueryDeviceAlarm\n");

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
			mxmlNewText(node, 0, "Alarm");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", theApp.m_SerailNumber++);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, deviceId);
			node = mxmlNewElement(query, "StartAlarmPriorit");
			mxmlNewText(node, 0, "0");
			node = mxmlNewElement(query, "EndAlarmPriority");
			mxmlNewText(node, 0, "0");

			char stbuf[32] = { 0 };
			char edbuf[32] = { 0 };
			{
				time_t t = time(NULL) - 600;
				struct tm* l = localtime(&t);
				strftime(stbuf, 32, "%Y-%m-%dT%H:%M:%S", l);

			}
			{
				time_t t = time(NULL);
				struct tm* l = localtime(&t);
				strftime(edbuf, 32, "%Y-%m-%dT%H:%M:%S", l);
			}

			node = mxmlNewElement(query, "StartAlarmTime");
			mxmlNewText(node, 0, stbuf);
			node = mxmlNewElement(query, "EndAlarmTime");
			mxmlNewText(node, 0, edbuf);
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

int GB28181Server::SendQueryDeviceConfig(struct eXosip_t *peCtx, CameraDevice deviceNode) {
	fprintf(stderr, "SendQueryDeviceConfig\n");

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
			mxmlNewText(node, 0, "ConfigDownload");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", theApp.m_SerailNumber++);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, deviceId);
			node = mxmlNewElement(query, "ConfigType");
			mxmlNewText(node, 0, "VideoParamOpt");
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

void GB28181Server::handleConfigDownload(eXosip_event_t *je, mxml_node_t* xml) {
	Response200(m_eCtx, je);

	if (theApp.m_logmask & (1 << BIT_5_QUERY)) {
		Debug(INFO, "dev config:%s", sipmessage2str(je->request).c_str());
	}
	return;

	mxml_node_t * DeviceIDNode = mxmlFindElement(xml, xml, "DeviceID", NULL, NULL, MXML_DESCEND); //must

	if (DeviceIDNode != NULL) {
		const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);
	}
}

void GB28181Server::handleDevStatus(eXosip_event_t *je, mxml_node_t* xml) {
	Response200(m_eCtx, je);
	if (theApp.m_logmask & (1 << BIT_5_QUERY)) {

		Debug(INFO, "dev status:%s", sipmessage2str(je->request).c_str());
	}
	return;

	mxml_node_t * DeviceIDNode = mxmlFindElement(xml, xml, "DeviceID", NULL, NULL, MXML_DESCEND); //must

	if (DeviceIDNode != NULL) {
		const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);
	}

	mxml_node_t * ResultNode = mxmlFindElement(xml, xml, "Result", NULL, NULL, MXML_DESCEND); //must

	if (ResultNode != NULL) {
		const char *Result = mxmlGetText(ResultNode, NULL);

		printf("Result is %s", Result);
	}

	mxml_node_t * OnlineNode = mxmlFindElement(xml, xml, "Online", NULL, NULL, MXML_DESCEND); //must

	if (OnlineNode != NULL) {
		const char *Online = mxmlGetText(OnlineNode, NULL);

		printf("Online is %s", Online);
	}

	mxml_node_t * StatusNode = mxmlFindElement(xml, xml, "Status", NULL, NULL, MXML_DESCEND); //must

	if (StatusNode != NULL) {
		const char *Status = mxmlGetText(StatusNode, NULL);

		printf("Status is %s", Status);
	}

	mxml_node_t * DeviceTimeNode = mxmlFindElement(xml, xml, "DeviceTime", NULL, NULL, MXML_DESCEND); //option

	if (DeviceTimeNode != NULL) {
		const char *DeviceTime = mxmlGetText(DeviceTimeNode, NULL);

		printf("DeviceTime is %s", DeviceTime);
	}

	mxml_node_t * AlarmstatusNode = mxmlFindElement(xml, xml, "Alarmstatus", NULL, NULL, MXML_DESCEND); //option

	if (AlarmstatusNode != NULL) {
		const char *Num = mxmlElementGetAttr(AlarmstatusNode, "Num");
		int count = atoi(Num);
		mxml_node_t * ItemNode = mxmlFindElement(AlarmstatusNode, AlarmstatusNode, "Item", NULL, NULL, MXML_DESCEND);

		for (int i = 0; i < count; i++) {
			if (i != 0)
				ItemNode = mxmlFindElement(ItemNode, AlarmstatusNode, "Item", NULL, NULL, MXML_DESCEND);

			if (ItemNode != NULL) {
				mxml_node_t * DeviceIDNode = mxmlFindElement(ItemNode, ItemNode, "DeviceID", NULL, NULL, MXML_DESCEND);
				mxml_node_t * DutyStatusNode = mxmlFindElement(ItemNode, ItemNode, "DutyStatus", NULL, NULL, MXML_DESCEND);

				const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);
				const char *DutyStatus = mxmlGetText(DutyStatusNode, NULL);

			}
		}
	}

	mxml_node_t * EncodeNode = mxmlFindElement(xml, xml, "Encode", NULL, NULL, MXML_DESCEND); //option

	if (EncodeNode != NULL) {
		const char *Encode = mxmlGetText(EncodeNode, NULL);

		printf("Encode is %s", Encode);
	}

	mxml_node_t * RecordNode = mxmlFindElement(xml, xml, "Record", NULL, NULL, MXML_DESCEND); //option

	if (RecordNode != NULL) {
		const char *Record = mxmlGetText(RecordNode, NULL);

		printf("Record is %s", Record);
	}
}
void GB28181Server::handleDevInfoQuery1(eXosip_event_t *je, TiXmlHandle& handle) {
	Response200(m_eCtx, je);
	if (theApp.m_logmask & (1 << BIT_5_QUERY)) {
		Debug(INFO, "dev info:%s", sipmessage2str(je->request).c_str());
	}

	osip_to_t *hto = osip_message_get_to(je->request);
	printf("agentid == %s\n", hto->url->username);
	if (theApp.m_sipserver_id != hto->url->username) {
		sendInterUDPMessage(je->request);
		return;
	}
}

void GB28181Server::handleDevInfoQuery(eXosip_event_t *je, mxml_node_t* xml) {

	Response200(m_eCtx, je);

	osip_to_t *hto = osip_message_get_to(je->request);
	printf("agentid == %s\n", hto->url->username);
	if (theApp.m_sipserver_id != hto->url->username) {
		sendInterUDPMessage(je->request);
		return;
	}

	mxml_node_t * DeviceIDNode = mxmlFindElement(xml, xml, "DeviceID", NULL, NULL, MXML_DESCEND); //must

	if (DeviceIDNode != NULL) {
		const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);

		//receiveMessage(DeviceID, MessageType_DeviceInfo, body->body);
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
	}
	const char *Result = mxmlGetText(ResultNode, NULL);
	if (ManufacturerNode != NULL) {
		const char *Manufacturer = mxmlGetText(ManufacturerNode, NULL);
		printf("Manufacturer is %s", Manufacturer);
	}
	if (ModelNode != NULL) {
		const char *Model = mxmlGetText(ModelNode, NULL);
		printf("Model is %s", Model);
	}
	if (FirmwareNode != NULL) {
		const char *Firmware = mxmlGetText(FirmwareNode, NULL);
		printf("Firmware is %s", Firmware);
	}
	if (ChannelNode != NULL) {
		const char *Channel = mxmlGetText(ChannelNode, NULL);
		printf("Channel is %s", Channel);
	}
	if (InfoNode != NULL) {
		const char *Info = mxmlGetText(InfoNode, NULL);
		printf("Info is %s", Info);
	}

}

int GB28181Server::SendQueryCatalog(struct eXosip_t *peCtx, CameraDevice* deviceNode, int catlogSN) {
	fprintf(stderr, "sendQueryCatalog\n");

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

	const char *deviceId = deviceNode->m_devid.c_str();
	const char *platformSipId = theApp.m_sipserver_id.c_str();

	const char *platformIpAddr = deviceNode->m_dev_remote_ip.c_str();
	int platformSipPort = deviceNode->m_dev_remote_port;

	const char *localSipId = theApp.m_sipserver_id.c_str();
	const char *localIpAddr = theApp.m_sipserver_ip.c_str();

	tree = mxmlNewXML("1.0");
	if (tree != NULL) {
		query = mxmlNewElement(tree, "Query");
		if (query != NULL) {
			char buf[256] = { 0 };
			char dest_call[256], source_call[256];
			node = mxmlNewElement(query, "CmdType");
			mxmlNewText(node, 0, "Catalog");
			node = mxmlNewElement(query, "SN");
			snprintf(sn, 32, "%d", catlogSN);
			mxmlNewText(node, 0, sn);
			node = mxmlNewElement(query, "DeviceID");
			mxmlNewText(node, 0, deviceId);
			mxmlSaveString(tree, buf, 256, whitespace_cb);

			osip_message_t *message = NULL;
			snprintf(dest_call, 256, "sip:%s@%s:%d", platformSipId, platformIpAddr, platformSipPort);
			snprintf(source_call, 256, "sip:%s@%s:%d", localSipId, localIpAddr, theApp.m_sipserver_port);
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


int GB28181Server::SendQueryCatalog1(struct eXosip_t *peCtx, CameraDevice* deviceNode, int catlogSN) {
	fprintf(stderr, "sendQueryCatalog1\n");

	char sn[32] = { 0 };
	int ret;
	mxml_node_t *tree, *query, *node;

	const char *deviceId = deviceNode->m_devid.c_str();
	const char *platformSipId = theApp.m_sipserver_id.c_str();

	const char *platformIpAddr = deviceNode->m_dev_remote_ip.c_str();
	int platformSipPort = deviceNode->m_dev_remote_port;

	const char *localSipId = theApp.m_sipserver_id.c_str();
	const char *localIpAddr = theApp.m_sipserver_ip.c_str();

	TiXmlDocument xml_doc;
	/*建立*/
	//<A>a</A>
	TiXmlElement* xmlElemA = new TiXmlElement("Query");

	{
		TiXmlElement* pNode1 = new TiXmlElement("CmdType");
		TiXmlText * pText1 = new TiXmlText("Catalog");
		pNode1->LinkEndChild(pText1);
		xmlElemA->LinkEndChild(pNode1);
	}
	{
		TiXmlElement* pNode1 = new TiXmlElement("SN");
		snprintf(sn, 32, "%d", catlogSN);
		TiXmlText * pText1 = new TiXmlText(sn);
		pNode1->LinkEndChild(pText1);
		xmlElemA->LinkEndChild(pNode1);
	}
	{
		TiXmlElement* pNode1 = new TiXmlElement("DeviceID");
		TiXmlText * pText1 = new TiXmlText(deviceId);
		pNode1->LinkEndChild(pText1);
		xmlElemA->LinkEndChild(pNode1);
	}

	xml_doc.LinkEndChild(xmlElemA);
	TiXmlPrinter printer;
	xml_doc.Accept(&printer);
	std::string stringBuffer = printer.CStr();

	char dest_call[256], source_call[256];
	osip_message_t *message = NULL;
	snprintf(dest_call, 256, "sip:%s@%s:%d", platformSipId, platformIpAddr, platformSipPort);
	snprintf(source_call, 256, "sip:%s@%s:%d", localSipId, localIpAddr, theApp.m_sipserver_port);
	ret = eXosip_message_build_request(peCtx, &message, "MESSAGE", dest_call, source_call, NULL);
	if (ret == 0 && message != NULL) {
		osip_message_set_body(message, stringBuffer.c_str(), stringBuffer.length());
		osip_message_set_content_type(message, "Application/MANSCDP");

		eXosip_lock(peCtx);
		eXosip_message_send_request(peCtx, message);
		eXosip_unlock(peCtx);
	} else {
		Debug(ERROR, "eXosip_message_build_request failed!\n");
	}

	return 0;
}

