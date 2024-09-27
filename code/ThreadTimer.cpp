#include "ThreadTimer.h"

CThreadTimer::CThreadTimer() {
}

CThreadTimer::~CThreadTimer() {
}

void CThreadTimer::startThread() {
	CThreadTimer* timer = new CThreadTimer();

	timer->setAutoDelete(true);
	timer->start();
}

void CThreadTimer::run() {
	int count = 0;
	while (true) {
		ulu_time::wait(5 * 1000);
		xundianTimer();
	}
}
void CThreadTimer::xundianTimer() {
	theApp.m_xdmgr.checkOffsite();
}

