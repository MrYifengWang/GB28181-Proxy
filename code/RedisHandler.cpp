/*
 * RedisHandler.cpp
 *
 *  Created on: Mar 10, 2020
 *      Author: wyf
 */

#include "RedisHandler.h"
#include <string>
#include <cstring>
#include <iostream>
#include "App.h"
using namespace std;

RedisHandler::RedisHandler() {

	pm_rct = NULL;
	error_msg = "";

	if (!theApp.m_redis_ip.empty() && !theApp.m_redis_pswd.empty()) {
		Debug(ERROR, "redis server:%s  %d  %s\n", theApp.m_redis_ip.c_str(), theApp.m_redis_port, theApp.m_redis_pswd.c_str());
	}

}

RedisHandler::~RedisHandler() {
	disConnect();
	pm_rct = NULL;

}

int RedisHandler::reconnect() {

	for (int i = 5; i > 0; i--) {
		disConnect();
		m_is_connect = false;
		pm_rct = NULL;
		error_msg = "";
		Debug(ERROR, "connect redis begin");
		m_is_connect = (0 == connect(theApp.m_redis_ip, theApp.m_redis_port, theApp.m_redis_pswd));

		if (m_is_connect)
			break;
	}
	Debug(ERROR, "connect redis OK");
	return 0;
}

int RedisHandler::connect(const string &addr, int port, const string &pwd) {

	pm_rct = redisConnect(addr.c_str(), port);

	if (pm_rct->err) {
		error_msg = pm_rct->errstr;
		return M_CONNECT_FAIL;
	}

	if (!pwd.empty()) {
		return connectAuth(pwd);
	}

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	redisSetTimeout(pm_rct, tv);

	return M_REDIS_OK;
}

int RedisHandler::disConnect() {
	redisFree(pm_rct);
}

int RedisHandler::setDevItem(CameraDevice* dev) {
	if (!m_is_connect) {
		reconnect();
	}

	stringstream command("");
	command << "HMSET " << "hd_" << dev->m_uluSN << " ";
	if (dev->m_dev_type == 1) {
		command << "mask " << 1 << " ";
	} else {
		command << "mask " << 3 << " ";
	}
	command << "did " << dev->m_uluSN << " ";
	command << "status " << 1 << " ";

	char stbuf[32] = { 0 };
	time_t t = time(NULL);
	struct tm* l = localtime(&t);
	strftime(stbuf, 32, "%Y-%m-%dT%H:%M:%S", l);

	command << "report_time " << stbuf << " ";
	command << "devmgr_serv_addr " << theApp.m_sipserver_ip << ":" << theApp.m_sipserver_port << " ";
	command << "platform_flag " << 1 << " ";
	command << "login_cnt " << 1 << " ";

	redisReply* pm_rr = (redisReply*) redisCommand(pm_rct, command.str().c_str());

	int ret = handleResult(pm_rr);
	freeReplyObject(pm_rr);

	//if (ret < 0) {
	Debug(ERROR, "set status err:%d %s", ret, command.str().c_str());
	if (ret == -2)
		m_is_connect = false;
	//}

	return ret;
}
int RedisHandler::delDevItem(char* did) {
	if (!m_is_connect) {
		reconnect();
	}

	if (did == NULL)
		return 0;

	int ret = 0;
	{
		stringstream command("");
		command << "HSET " << "hd_" << did << " ";
		command << "status " << 2 << " ";

		redisReply* pm_rr = (redisReply*) redisCommand(pm_rct, command.str().c_str());

		ret = handleResult(pm_rr);
		freeReplyObject(pm_rr);
		Debug(ERROR, "del status err:%d %s", ret, command.str().c_str());

	}
	{
		stringstream command("");
		command << "HDEL " << "hd_" << did << " ";
		command << "channel_status";

		redisReply* pm_rr = (redisReply*) redisCommand(pm_rct, command.str().c_str());

		ret = handleResult(pm_rr);
		freeReplyObject(pm_rr);

		Debug(ERROR, "del status err:%d %s", ret, command.str().c_str());

	}

	//if (ret < 0) {
	if (ret == -2)
		m_is_connect = false;
	//}

	return 0;
}

int RedisHandler::connectAuth(const string &psw) {
	string cmd = "auth " + psw;
	redisReply* pm_rr = (redisReply*) redisCommand(pm_rct, cmd.c_str());

	int ret = handleReply(pm_rr);
	freeReplyObject(pm_rr);

	return ret;

}
int RedisHandler::handleResult(redisReply* pm_rr) {
	if (pm_rct->err) {
		error_msg = pm_rct->errstr;
		return M_CONTEXT_ERROR;
	}

	if (pm_rr == NULL) {
		error_msg = "auth redisReply is NULL";
		return M_REPLY_ERROR;
	}

	switch (pm_rr->type) {
	case REDIS_REPLY_ERROR:
		error_msg = pm_rr->str;
		return M_EXE_COMMAND_ERROR;
	case REDIS_REPLY_STATUS:
		if (!strcmp(pm_rr->str, "OK"))
			return M_REDIS_OK;
		else {
			error_msg = pm_rr->str;
			return M_EXE_COMMAND_ERROR;
		}
	default:
		break;
	}

	return pm_rr->type;
}
int RedisHandler::handleReply(redisReply* pm_rr, void* value, redisReply ***array) {
	if (pm_rct->err) {
		error_msg = pm_rct->errstr;
		return M_CONTEXT_ERROR;
	}

	if (pm_rr == NULL) {
		error_msg = "auth redisReply is NULL";
		return M_REPLY_ERROR;
	}

	switch (pm_rr->type) {
	case REDIS_REPLY_ERROR:
		error_msg = pm_rr->str;
		return M_EXE_COMMAND_ERROR;
	case REDIS_REPLY_STATUS:
		if (!strcmp(pm_rr->str, "OK"))
			return M_REDIS_OK;
		else {
			error_msg = pm_rr->str;
			return M_EXE_COMMAND_ERROR;
		}
	case REDIS_REPLY_INTEGER:
		*(int*) value = pm_rr->integer;
		return M_REDIS_OK;
	case REDIS_REPLY_STRING:
		*(string*) value = pm_rr->str;
		return M_REDIS_OK;
	case REDIS_REPLY_NIL:
		*(string*) value = "";
		return M_REDIS_OK;
	case REDIS_REPLY_ARRAY:
		*(int*) value = pm_rr->elements;
		*array = pm_rr->element;
		return M_REDIS_OK;
	default:
		error_msg = "unknow reply type";
		return M_EXE_COMMAND_ERROR;
	}
}
