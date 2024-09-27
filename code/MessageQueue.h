/*
 * MessageQueue.h
 *
 *  Created on: Mar 5, 2020
 *      Author: wyf
 */

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include "common/ulu_lib.h"
#include <queue>
#include <eXosip2/eXosip.h>
#include "App.h"


class Message {
public:
	Message();
	Message(std::string pmsg, int itype = MSG_SIP, int icmd = CMD_NULL) {
		m_data = pmsg;
		m_type = itype;
		m_cmd = icmd;
	}

	virtual ~Message();
	void handleMsg();
public:
	int m_type; //1,json 2,sip 3,xml
	std::string m_data;
	int m_cmd;
};
class MessageQueue {
public:
	MessageQueue();
	virtual ~MessageQueue();
public:
	void push(Message* msg);
	Message* pop();
	Message* trypop(int seconds);
	int size();

private:
	mutable ulu_mutex m_lock;
	ulu_semaphore m_sem;
	std::queue<Message*> m_queue;
};

#endif /* MESSAGEQUEUE_H_ */
