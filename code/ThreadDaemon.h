/*
 * ThreadDaemon.h
 *
 *  Created on: May 20, 2020
 *      Author: wyf
 */

#ifndef THREADDAEMON_H_
#define THREADDAEMON_H_
#include "App.h"

class ThreadDaemon: public ulu_thread {
public:
	ThreadDaemon();
	virtual ~ThreadDaemon();
public:
	static void startThread();

protected:
	virtual void run();
	void logAndExit();
	bool checkSipThread();
	bool checkEpollThread();
	bool checkWorkerThread();
};

#endif /* THREADDAEMON_H_ */
