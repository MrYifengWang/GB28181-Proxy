/*
 * ThreadCenter.cpp
 *
 *  Created on: Jun 9, 2020
 *      Author: wyf
 */

#include "ThreadCenter.h"
#include "common/base64.h"

ThreadCenter::ThreadCenter() {
	// TODO Auto-generated constructor stub
	m_token = theApp.makeSirealstr();
}

ThreadCenter::~ThreadCenter() {
	// TODO Auto-generated destructor stub
}

void ThreadCenter::startThread() {
	ThreadCenter* timer = new ThreadCenter();

	timer->setAutoDelete(true);
	timer->start();
}
bool ThreadCenter::onStart() {

	int port = theApp.m_localCenterUdpPort;

	m_uSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_uSocket < 0) {
		return false;
	}
	int p = 1;
	::setsockopt(m_uSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &p, sizeof(p));

	while (port < 65535) {

		struct sockaddr_in peerAddr;
		peerAddr.sin_family = AF_INET;
		peerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		peerAddr.sin_port = htons(port);

		int ret = ::bind(m_uSocket, (struct sockaddr*) &peerAddr, sizeof(peerAddr));

		if (ret < 0) {
			int err = errno;
			if (err == 98) {
				port += 1;
				continue;
			} else {
				break;
			}
		}
		theApp.m_localCenterUdpPort = port;
		return true;

	}

	return false;
}

void ThreadCenter::run() {
//	char * psrc = "aGhoaGhoaGhkYWlsXiYoKSkxOTIuMTY4LjExMC4xMjM=";
//	char dstbuf[256] = { 0 };
//	int ret = b64_decode((unsigned char*) psrc, strlen(psrc), (unsigned char*) dstbuf);
//	printf("--------%d--- %s----- %s\n", ret, dstbuf, psrc);
//	return;

	while (true) {

		while (true) {
			if (true == loginCenterServer()) {
				m_tSocket.enableNonblock(true);
				m_lastkeepaliveAcktm = time(NULL);
				theApp.m_CenterEvent.signal();
				break;
			}
			sleep(10);
		}

		while (true) {
			int mfd = (int) m_tSocket > m_uSocket ? (int) m_tSocket : m_uSocket;
			struct timeval tv;
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			fd_set set;
			FD_ZERO(&set);
			FD_SET(m_tSocket, &set);
			FD_SET(m_uSocket, &set);

			int ret = select((int) mfd + 1, &set, NULL, NULL, &tv);

			if (ret < 0) {
				Debug(ERROR, "close by center server %d", ret);
				break;
			} else if (ret == 0) {
				int curtime = time(NULL);
				if (!sendKeepAlive(curtime) || !checkKeepAlive(curtime))
					break;
			} else {
				if (FD_ISSET(m_tSocket, &set)) {
					if (!readCommand()) {
						Debug(ERROR, "handle command fail %d", errno);
						break;
					}
				} else if (FD_ISSET(m_uSocket, &set)) {
					struct sockaddr_in peerAddr;
					socklen_t len = sizeof(peerAddr);

					char udpBuffer[1024 * 10] = { 0 };
					int cLen = recvfrom(m_uSocket, udpBuffer, 1024 * 10, 0, (struct sockaddr*) &peerAddr, &len);

					if (cLen < 0) {
						int err = errno;
						Debug(ERROR, "handle udp fail %d", errno);
						sleep(5);
						break;

					}
					printf("recv udp:%s", udpBuffer);
					handleSipResponse(udpBuffer, strlen(udpBuffer));

				}
			}
		}

		m_tSocket.close();
		sleep(3);
	}
}

bool ThreadCenter::readCommand() {
	if (m_tSocket.readall() < 0) {
		Debug(ERROR, "tcp close by server");
		return false;
	}
	while (1) {
		string command;
		int iret = m_tSocket.readpacket(command);

		if (iret < 0) {
			if (iret == -1) {
				if (errno == 11)
					return true;
			}

			Debug(ERROR, "tcp close :read %d", errno);
			return false;
		} else if (iret == 0) {
			break;
		} else {
			handleCommand(command);
		}
	}

	return true;
}

void ThreadCenter::handleCommand(string& command) {

	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(command, jmessage)) {
		return;
	}

	string serialstr = theApp.makeSirealstr();

	if (jmessage["serial"].isString()) {
		serialstr = jmessage["serial"].asString();
	}
	string cmdstr = jmessage["cmd"].asString();

	if (cmdstr != "gbserver_keepalive_mgr_ack") {
		Debug(INFO, "center cmd :%s\n", command.c_str());
		printf("center cmd :%s\n", command.c_str());
	}

	if (cmdstr == "gbserver_keepalive_mgr_ack") {
		m_lastkeepaliveAcktm = time(NULL);
	} else if (cmdstr == "remote_restart_req") {
		Debug(ERROR, "restart by center server");
		theApp.shutdown();
		exit(0);
	} else if (cmdstr == "server_stat_notify") {
		Json::Value data = jmessage["data"];
		handleCtrlCommand(data);
	} else if (cmdstr == "config_update_notify") {
		Json::Value data = jmessage["data"];
		theApp.parseConfig(data);
	} else if (cmdstr == "query_from_gbserver_req") {
		theApp.sendInterMessage(command, 100);
	} else if (cmdstr == "stream_list_notify") {
		Json::Value data = jmessage["data"];
		handleStreamList(data);
	} else if (cmdstr == "set_log_mask_notity") {

		Debug(INFO, "old log mask = %d\n", theApp.m_logmask);

		if (jmessage["data"]["mask"].isInt()) {
			int mask = jmessage["data"]["mask"].asInt();
			theApp.m_logmask = mask;
		} else {
			int bit = jmessage["data"]["bit"].asInt();
			int val = jmessage["data"]["value"].asInt();
			if (val == 0) {
				theApp.m_logmask &= (~(1 << (bit - 1)));
			} else {
				theApp.m_logmask |= 1 << (bit - 1);
			}
		}
		Debug(INFO, "new log mask = %d\n", theApp.m_logmask);
	} else if (cmdstr == "set_log_level_notity") {

		Debug(INFO, "old log level = %d\n", theApp.m_loglevel);

		if (jmessage["data"]["level"].isInt()) {
			int level = jmessage["data"]["level"].asInt();
			theApp.m_loglevel = level;
		}
		Debug(INFO, "new log level = %d\n", theApp.m_loglevel);
	}

}

bool ThreadCenter::loginCenterServer() {

	if (!m_tSocket.create(SOCK_STREAM)) {
		assert(0);
		return false;
	}
	if (!m_tSocket.bind(0)) {
		assert(0);
		return false;
	}

	if (!m_tSocket.connect1(theApp.m_center_server_ip.c_str(), theApp.m_center_server_port)) {
		return false;
	}
	{
		Json::Value root;
		root["cmd"] = "gbserver_login_mgr_req";
		root["serial"] = theApp.makeSirealstr();
		Json::Value data;
		data["token"] = m_token;
		data["version"] = "1.0.3";
		data["tm"] = (int) time(NULL);
		string tmpip = theApp.getLocalIP();
		if (tmpip.empty()) {
			return false;
		}
		data["local_ip"] = tmpip;
		root["data"] = data;

		Json::FastWriter writer;
		string message = writer.write(root);

		m_tSocket.asynwrite(message);
	}

	string response;
	int ret = m_tSocket.readpacket(response, 30);

	Debug(ERROR, "config from center:%s", response.c_str());
	printf("%d %s\n", ret, response.c_str());
	if (ret == 1) {
		Json::Reader reader;
		Json::Value jmessage;
		if (reader.parse(response, jmessage)) {
			if (jmessage["cmd"].asString() != "gbserver_login_mgr_ack") {
				m_tSocket.close();
				return false;
			}
			int error = jmessage["error"].asInt();
			if (error != 0) {
				Debug(ERROR, "login center error:%s", response.c_str());
				return false;
			}
			if (jmessage["data"].isObject()) {
				Json::Value data = jmessage["data"];
				theApp.parseConfig(data);
			} else {
				Debug(ERROR, "get config fail, self reset");
				//exit(0);
				m_tSocket.close();
				return false;
			}
		}
		return true;
	}
	return false;

}
void ThreadCenter::handleCtrlCommand(Json::Value& data) {

	int servertype = data["server_type"].asInt();
	int serverstat = data["server_stat"].asInt();
	string serverip = data["server_ip"].asString();
	string serverid = data["server_id"].asString();

	if (servertype == 2 && serverstat == 0) {
		Json::Value root;
		root["cmd"] = "reset_all_session";
		root["serverip"] = serverip;

		Json::FastWriter writer;
		string message = writer.write(root);
		theApp.sendInterMessage(message, 0);
	} else if (servertype == 2 && serverstat == 1) {
		mediaserver server;
		server.m_streamserver_id = serverid;
		server.m_streamserver_ip = serverip;
		server.m_streamserver_sip_ip = serverip;
		server.m_streamserver_rtp_tcp_port = data["rtp_port"].asInt();
		server.m_streamserver_rtp_udp_port = server.m_streamserver_rtp_tcp_port;
		//	server.m_streamserver_sip_ip = data["server_sip_ip"].asString();
		server.m_streamserver_sip_port = data["sip_port"].asInt();

		Debug(INFO, "add server from center %d %d %^s\n", server.m_streamserver_sip_port, server.m_streamserver_rtp_tcp_port,
				serverip.c_str());
		theApp.addServer(server);
	}

}
void ThreadCenter::handleStreamList(Json::Value& data) {

	int arrsize = data.size();
	for (int i = 0; i < arrsize; i++) {
		Json::Value item = data[i];
		string serverip = item["server_ip"].asString();
		string serverid = item["server_id"].asString();
		mediaserver server;
		server.m_streamserver_id = serverid;
		server.m_streamserver_ip = serverip;
		server.m_streamserver_sip_ip = serverip;
		server.m_streamserver_rtp_tcp_port = item["rtp_port"].asInt();
		server.m_streamserver_rtp_udp_port = server.m_streamserver_rtp_tcp_port;
		//	server.m_streamserver_sip_ip = item["server_sip_ip"].asString();
		server.m_streamserver_sip_port = item["sip_port"].asInt();
		Debug(ERROR, "add server from notifylist %d %d %s %s\n", server.m_streamserver_sip_port, server.m_streamserver_rtp_tcp_port,
				serverip.c_str(), server.m_streamserver_sip_ip.c_str());
		theApp.addServer(server);
	}
}

bool ThreadCenter::sendKeepAlive(int curtime) {
	if (curtime - m_lastkeepalivetm > KEEPALIVE_TIME) {
		m_lastkeepalivetm = curtime;
		Json::Value root;
		Json::Value data;
		root["cmd"] = "gbserver_keepalive_mgr_req";
		root["serial"] = theApp.makeSirealstr();
		root["data"] = data;

		Json::FastWriter writer;
		string message = writer.write(root);
		puts(message.c_str());

		return this->m_tSocket.asynwrite(message);
	}
	return true;
}
bool ThreadCenter::checkKeepAlive(int curtime) {
	return (curtime - m_lastkeepaliveAcktm < KEEPALIVE_TIME * 3);
}
void ThreadCenter::handleSipResponse(char* buf, int len) {

	Json::Reader reader;
	Json::Value jmessage;
	string msg(buf, len);
	if (reader.parse(msg, jmessage)) {

		Json::FastWriter writer;
		string message = writer.write(jmessage);
		if (jmessage["cmd"].asString() == "query_from_gbserver_ack") {
			printf("handle udp:%s\n", message.c_str());
			this->m_tSocket.asynwrite(msg);
		} else if (jmessage["cmd"].asString() == "request_for_serverlist_notify") {
			printf("handle udp:%s\n", message.c_str());
			this->m_tSocket.asynwrite(msg);
		}
	}
}
