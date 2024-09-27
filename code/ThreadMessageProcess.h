/*
 * ThreadMessageProcess1.h
 *
 *  Created on: Mar 13, 2020
 *      Author: wyf
 */

#ifndef THREADMESSAGEPROCESS1_H_
#define THREADMESSAGEPROCESS1_H_

#include "MessageQueue.h"
class ThreadMessageProcess: public ulu_thread {
public:
	ThreadMessageProcess(MessageQueue* queue);
	virtual ~ThreadMessageProcess();
public:
	static void startThread(MessageQueue* queue);

protected:
	virtual bool onStart();
	virtual void run();
private:
	void handelMessage(Message& msg);
	void handleRegisterAuth(Message& msg);
	void handleAlarmReport(Message& msg);
	void handleDevstatReport(Message& msg);
	void handleChannelstatReport(Message& msg);

	void cleanResponse();
	int httpGetAuthDigest(char* did, char* realm, string& response);
	string httpGetSN(char* devid);
	void httpReportAlarm(string jsonstr);
	void httpReportDevstat(string jsonstr);
	void httpReportChannelstat(string id, int status);

private:
	MessageQueue* m_pqueue;
//	ulu_socket_msg_old m_tSocket;
	struct eXosip_t *m_eCtx;

	string m_logstring;

};

#endif /* THREADMESSAGEPROCESS1_H_ */
