#ifndef ThreadTimer_h__
#define ThreadTimer_h__

#include "App.h"
class CThreadTimer: public ulu_thread {
public:

	CThreadTimer();
	virtual ~CThreadTimer();

public:
	static void startThread();

protected:
	virtual void run();

private:
	void xundianTimer();
	void epolltimer();

private:

};

#endif

