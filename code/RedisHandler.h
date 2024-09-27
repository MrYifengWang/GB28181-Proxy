/*
 * RedisHandler.h
 *
 *  Created on: Mar 10, 2020
 *      Author: wyf
 */
#ifndef __REDIS_HANDLER_H__
#define __REDIS_HANDLER_H__

//#include "hiredis\hiredis.h"
#include "hiredis/hiredis.h"
#include <string>
#include "CameraDevice.h"

using namespace std;

enum {
	M_REDIS_OK = 0, //执行成功
	M_CONNECT_FAIL = -1, //连接redis失败
	M_CONTEXT_ERROR = -2, //RedisContext返回错误
	M_REPLY_ERROR = -3, //redisReply错误
	M_EXE_COMMAND_ERROR = -4 //redis命令执行错误
};

class RedisHandler {
public:
	RedisHandler();
	~RedisHandler();
	int connect(const string & ip, int port, const string & pswd); //连接redis数据库：addr：IP地址，port：端口号，pwd：密码(默认为空)
	int disConnect(); //断开连接
	int reconnect();
	int setDevItem(CameraDevice*);
	int delDevItem(char* did);

private:

	redisContext* pm_rct; //redis结构体
	//redisReply* pm_rr; //返回结构体
	string error_msg; //错误信息
	bool m_is_connect;

	int connectAuth(const string &pwd); //使用密码登录
	int handleReply(redisReply* pm_rr, void* value = NULL, redisReply ***array = NULL); //处理返回的结果
	int handleResult(redisReply* pm_rr);
};

#endif
