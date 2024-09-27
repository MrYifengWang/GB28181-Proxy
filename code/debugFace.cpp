#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include "debugLog.h"
#include <pthread.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sstream>
#include <iostream>
using std::string;
using std::stringstream;

#include "MessageQueue.h"

debugFace* debugFace::m_pStSelf = NULL;

debugFace::debugFace() {
	m_debugBuf = new char[100];
	memset(m_debugBuf, 0, 100);

	{
		char path[512];
		getcwd(path, 511);

		m_tRoot = path;
		m_tRoot += "/logs";

		struct stat buffer;

		if (stat(m_tRoot.c_str(), &buffer) != 0) {
			mkdir(m_tRoot.c_str(), S_IRWXO | S_IRWXG | S_IRWXU);
			printf("---------%d %d %d------%d---------\n", S_IRWXO, S_IRWXG, S_IRWXU, S_IRWXO | S_IRWXG | S_IRWXU);
		}
	}
}

debugFace::~debugFace() {
	if (m_debugBuf) {
		printf("free m_debugBuf\n");
		delete[] m_debugBuf;
		m_debugBuf = NULL;
	}
}

debugFace* debugFace::GetInstancePtr() {
	if (m_pStSelf == NULL) //there may be a bug;
	{
		m_pStSelf = new debugFace();

	}
	return m_pStSelf;
}
void debugFace::debug1(E_DEBUG_LEVER level, char *pszFormat, ...) {

	if (level < theApp.m_loglevel)
		return;
	va_list arg_ptr;
	va_start(arg_ptr, pszFormat);
	{
		char buf[4096] = { 0 };
		vsnprintf(buf, 4095, pszFormat, arg_ptr);
		buf[strlen(buf)] = '\n';
		if (theApp.m_log_queue != NULL) {
			Message* msg = new Message(string(buf), MSG_STR, CMD_LOG);
			theApp.m_log_queue->push(msg);
		} else {
			writefile(buf, strlen(buf));
		}
	}
	va_end(arg_ptr);

}
void debugFace::debug(E_DEBUG_LEVER level, const char *pszFormat, ...) {

	if (level < theApp.m_loglevel)
		return;
	va_list arg_ptr;

	char* m_time = NULL;
	char* m_file = NULL;
	char* m_func = NULL;
	int m_line = 0;
	va_start(arg_ptr, pszFormat);
	m_time = va_arg(arg_ptr, char*);
	m_file = va_arg(arg_ptr, char*);
	m_func = va_arg(arg_ptr, char*);
	m_line = va_arg(arg_ptr, int);
	va_end(arg_ptr);
	if (m_time != NULL && m_file != NULL) {
		char buf[4096] = { 0 };
		sprintf(buf, pszFormat, m_time, m_file, m_func, m_line);
		if (theApp.m_log_queue != NULL) {
			Message* msg = new Message(string(buf), MSG_STR, CMD_LOG);
			theApp.m_log_queue->push(msg);
		} else {
			writefile(buf, strlen(buf));
		}
	} else {
		writefile((char*) pszFormat, strlen(pszFormat));
	}
}

char* debugFace::getHumanTime() {
	time_t curr_time;
	time(&curr_time);
	tm* p_local_time = localtime(&curr_time);
	memset(m_debugBuf, 0, 100);
	sprintf(m_debugBuf, "%04d/%02d/%02d %02d:%02d:%02d", p_local_time->tm_year + 1900, p_local_time->tm_mon + 1, p_local_time->tm_mday,
			p_local_time->tm_hour, p_local_time->tm_min, p_local_time->tm_sec);
	return m_debugBuf;
}

void debugFace::writefile(string txt) {
	writefile((char*) txt.c_str(), txt.length());
}

void debugFace::writefile(char* logbuf, int loglen) {
	struct stat buffer;

	lock_log.lock();

	int year, month, day, hour, minute, second;

	time_t current;
	time(&current);

	struct tm* l = localtime(&current);

	year = l->tm_year + 1900;
	month = l->tm_mon + 1;
	day = l->tm_mday;
	hour = l->tm_hour;
	minute = l->tm_min;
	second = l->tm_sec;

	stringstream path("");
	path << m_tRoot << "/" << year << "-" << month << "-" << day << ".txt";

	FILE* file = fopen(path.str().c_str(), "a");
	int flen = ftell(file);

	if (file) {
		//fwrite(logbuf, loglen, 1, file);
		fprintf(file, "[%04d/%02d/%02d %02d:%02d:%02d] %s", year, month, day, hour, minute, second, logbuf);
		fclose(file);
	}
	if (flen > 1024 * 1024 * 80) {
		char buf[128] = { 0 };
		sprintf(buf, "rm %s", path.str().c_str());
		system(buf);
	}

	lock_log.unlock();
}

