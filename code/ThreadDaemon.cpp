/*
 * ThreadDaemon.cpp
 *
 *  Created on: May 20, 2020
 *      Author: wyf
 */

#include "ThreadDaemon.h"
#include "GB28181Server.h"

ThreadDaemon::ThreadDaemon() {
	// TODO Auto-generated constructor stub

}

ThreadDaemon::~ThreadDaemon() {
	// TODO Auto-generated destructor stub
}
void ThreadDaemon::startThread() {
	ThreadDaemon* timer = new ThreadDaemon();

	timer->setAutoDelete(true);
	timer->start();
}

void ThreadDaemon::run() {
	int count = 0;
	while (true) {
		ulu_time::wait(15 * 1000);

		if (!checkSipThread()) {
			Debug(ERROR, "sip thread check fail:%d %d", theApp.m_sip_lastkeeptime, theApp.m_sip_lastchecktime);
			logAndExit();
		}
//		if (!checkEpollThread()) {
//			logAndExit();
//		}
//		if (!checkWorkerThread()) {
//			logAndExit();
//		}
	}
}
void ThreadDaemon::logAndExit() {
	Debug(ERROR, "write pstask...");

	string ofn = "gbserver_stack.txt";

	time_t now = time(NULL);
	struct tm ntm;
	localtime_r(&now, &ntm);
	char time_buf[256] = { 0 };
	sprintf(time_buf, "%d-%02d-%02d %02d:%02d:%02d", ntm.tm_year + 1900, ntm.tm_mon + 1, ntm.tm_mday, ntm.tm_hour, ntm.tm_min, ntm.tm_sec);

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "echo -e \"----------------%s deadlock stack:\r\n\" >> %s", time_buf, ofn.c_str());
	system(cmd);

	snprintf(cmd, sizeof(cmd), "pstack %d >> %s", getpid(), ofn.c_str());
	system(cmd);

	snprintf(cmd, sizeof(cmd), "echo -e \"\r\n\r\n\r\n\r\n\r\n\r\n\" >> %s", ofn.c_str());
	system(cmd);

	snprintf(cmd, sizeof(cmd), "kill -s SIGQUIT %d", getpid());
	system(cmd);

	exit(0);

}
bool ThreadDaemon::checkWorkerThread() {

	vector<int> list;
	theApp.m_pserver->getQueueSize(list);

	for (int i = 0; i < list.size(); i++) {
		if (list[i] > 200) {
			return false;
		}
	}
	return true;

}
bool ThreadDaemon::checkSipThread() {
	int curtime = time(NULL);
	return (curtime - theApp.m_sip_lastkeeptime < 90);
}

bool ThreadDaemon::checkEpollThread() {
	ulu_socket_msg_old m_tSocket;

	if (!m_tSocket.create(SOCK_STREAM)) {
		assert(0);
		return false;
	}

	if (!m_tSocket.bind(0)) {
		assert(0);
		return false;
	}
	if (!m_tSocket.connect1("127.0.0.1", theApp.m_devmgr_port)) {
		return false;
	}
	{
		Json::Value root;
		root["cmd"] = "daemon_login_req";
		root["serial"] = "123";

		Json::FastWriter writer;
		string message = writer.write(root);

		m_tSocket.asynwrite(message);
	}

	string response;
	int ret = m_tSocket.readpacket(response, 5);
	m_tSocket.close();
	return (ret == 1);

}

