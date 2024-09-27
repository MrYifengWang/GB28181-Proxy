/*
 * ThreadCenter.h
 *
 *  Created on: Jun 9, 2020
 *      Author: wyf
 */

#ifndef THREADCENTER_H_
#define THREADCENTER_H_
#include "App.h"
#define KEEPALIVE_TIME 30

class ThreadCenter: public ulu_thread {
public:
	ThreadCenter();
	virtual ~ThreadCenter();
public:
	static void startThread();
protected:
	virtual bool onStart();
	virtual void run();

private:
	bool loginCenterServer();
	void handleCtrlCommand(Json::Value& command);
	void handleStreamList(Json::Value& command);
	bool readCommand();
	void handleCommand(string& command);
	bool sendKeepAlive(int curtime);
	bool checkKeepAlive(int curtime);
	void handleSipResponse(char* buf, int len);
private:
	ulu_socket_msg_old m_tSocket;
	int m_uSocket;

	string m_token;
	int m_lastkeepalivetm;
	int m_lastkeepaliveAcktm;

};

#endif /* THREADCENTER_H_ */
