/*
 * MessageQueue.cpp
 *
 *  Created on: Mar 5, 2020
 *      Author: wyf
 */

#include "MessageQueue.h"
#include "App.h"

Message::Message() {
	// TODO Auto-generated constructor stub

}

Message::~Message() {
	// TODO Auto-generated destructor stub
}
void Message::handleMsg() {
	osip_message_t * message;
	int mlen;
	osip_message_init(&message);
	osip_message_parse(message, m_data.c_str(), m_data.length());

	if (message != NULL) {
		osip_call_id * hdfrom = osip_message_get_call_id(message);
		if (hdfrom != NULL) {
			printf("---from---%s\n", hdfrom->number);
		}
	}
	osip_message_free(message);
}

//----------------------------
MessageQueue::MessageQueue() {
	// TODO Auto-generated constructor stub

}

MessageQueue::~MessageQueue() {
	// TODO Auto-generated destructor stub
}

void MessageQueue::push(Message* transmit) {

	m_lock.lock();

	m_queue.push(transmit);

	m_lock.unlock();

	m_sem.release();

}
Message* MessageQueue::trypop(int seconds) {
	m_sem.wait_sec(seconds);

	m_lock.lock();

	Message* transmit = NULL;
	if (!m_queue.empty()) {
		transmit = m_queue.front();
		m_queue.pop();
	}

	m_lock.unlock();

	return transmit;
}

Message* MessageQueue::pop() {
	m_sem.wait();

	m_lock.lock();

	Message* transmit = m_queue.front();
	m_queue.pop();

	m_lock.unlock();

	return transmit;
}
int MessageQueue::size() {
	int size = 0;
	m_lock.lock();

	size = m_queue.size();

	m_lock.unlock();

	return size;

}

//-------------------------------------
