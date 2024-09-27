/*
 * ThreadMessageProcess1.cpp
 *
 *  Created on: Mar 13, 2020
 *      Author: wyf
 */

#include "ThreadMessageProcess.h"
#include <eXosip2/eXosip.h>
#include <curl/curl.h>
#include "App.h"
#include "common/urlCodec.h"

ThreadMessageProcess::ThreadMessageProcess(MessageQueue* queue) {
	// TODO Auto-generated constructor stub
	m_pqueue = queue;
}

ThreadMessageProcess::~ThreadMessageProcess() {
	// TODO Auto-generated destructor stub
}

void ThreadMessageProcess::startThread(MessageQueue* queue) {
	ThreadMessageProcess* timer = new ThreadMessageProcess(queue);

	timer->setAutoDelete(true);
	timer->start();
}
bool ThreadMessageProcess::onStart() {

	{
		struct sigaction act;
		memset(&act, 0, sizeof(act));
		sigemptyset(&act.sa_mask);
		act.sa_handler = SIG_IGN;
		if (sigaction(SIGPIPE, &act, NULL) < 0) {

		}
	}

//	printf("---ip %s  %d\n", theApp.m_alarm_srv_ip.c_str(), theApp.m_alarm_srv_port);

	/*	if (theApp.m_alarm_srv_ip != "-1" && theApp.m_alarm_srv_port != -1) {
	 if (!m_tSocket.create(SOCK_STREAM)) {
	 assert(0);
	 return false;
	 }

	 if (!m_tSocket.bind(0)) {
	 assert(0);
	 return false;
	 }

	 if (!m_tSocket.connect1(theApp.m_alarm_srv_ip.c_str(), theApp.m_alarm_srv_port)) {
	 assert(0);
	 return false;
	 }

	 {
	 Json::Value root;
	 root["cmd"] = "client_login_mgr_req";
	 root["serial"] = "123";
	 Json::Value data;
	 data["token"] = "0000000000";
	 root["data"] = data;

	 Json::FastWriter writer;
	 string message = writer.write(root);

	 m_tSocket.asynwrite(message);
	 }

	 m_tSocket.enableNonblock(true);
	 }*/

	return true;
}

void ThreadMessageProcess::run() {

	puts("ThreadMessageProcess run ...");
	int count = 0;
	int curtime = time(NULL);
	while (true) {
		Message *msg = m_pqueue->trypop(10);
		if (msg != NULL) {
			handelMessage(*msg);
			delete msg;
		} else {

		}

	}
}

void ThreadMessageProcess::handelMessage(Message& msg) {

	if (msg.m_cmd == CMD_REG || msg.m_cmd == CMD_REG_REFRESH) {
		handleRegisterAuth(msg);
	} else if (msg.m_cmd == CMD_ALARM) {
		handleAlarmReport(msg);
	} else if (msg.m_cmd == CMD_DEVSTAT) {
		handleDevstatReport(msg);
	} else if (msg.m_cmd == CMD_CHANNELSTAT) {
		handleChannelstatReport(msg);
	} else if (msg.m_cmd == CMD_LOG) {
		debugFace::GetInstancePtr()->writefile(msg.m_data);
//		m_logstring.append(msg.m_data);
//		if (m_logstring.length() > 1024 * 20) {
//			debugFace::GetInstancePtr()->writefile(m_logstring);
//		}
	} else {
		puts("out -------");
	}
}

void ThreadMessageProcess::handleAlarmReport(Message& msg) {

	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(msg.m_data.c_str(), jmessage)) {
		return;
	}

	int alarmMethed = jmessage["method"].asInt();
	int alarmType = jmessage["type"].asInt();
	string alarmtime = jmessage["time"].asString();
	string sn = jmessage["sn"].asString();
	string did = jmessage["did"].asString();

	stringstream http_alarmdata("");
	if (alarmMethed == 2) {
		puts("+++++++++++ video lost");
		if (alarmType == 1) {
			//data["type"] = "video_lost_alarm";
		} else if (alarmType == 3) {
			//data["type"] = "storage_low_alarm";
		}
	} else if (alarmMethed == 5) {
		if (alarmType == 2) {
			//data["type"] = "motion_detect_alarm";

			if (theApp.m_logmask & (1 << BIT_7_MOTION)) {
				Debug(DEBUG, "motion alarm:%s", did.c_str());
			}
			theApp.m_xdmgr.checkConfig(did);
//			if (theApp.m_xdmgr.checkConfig(did)) {
//
//
//			}

		}
	} else if (alarmMethed == 6) {
		if (alarmType == 1) {
			//data["type"] = "disk_error_alarm";
		}
	}

}

void ThreadMessageProcess::handleChannelstatReport(Message& msg) {

	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(msg.m_data.c_str(), jmessage)) {
		Debug(ERROR, "channel check: error msg %s", msg.m_data.c_str());
		return;
	}

	string did = jmessage["channelid"].asString();
	int stat = jmessage["stat"].asInt();
	httpReportChannelstat(did, stat);
}
void ThreadMessageProcess::handleDevstatReport(Message& msg) {

	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(msg.m_data.c_str(), jmessage)) {
		Debug(ERROR, "dev stat err:%s", msg.m_data.c_str());
		return;
	}

	int type = jmessage["devtype"].asInt();
	int stat = jmessage["stat"].asInt();
	string sn = jmessage["sn"].asString();

	Json::Value arr;
	Json::Value root;

	root["device_type"] = type;
	root["devmgr_addr"] = theApp.m_sipserver_ip;
	root["off_tm"] = (int) time(NULL);
	root["off_tml"] = 0;
	root["sn"] = sn;
	root["state"] = stat;
	root["version"] = "";
	arr.append(root);

	Json::FastWriter writer;
	string message = writer.write(arr);
	httpReportDevstat(message);

}

void ThreadMessageProcess::handleRegisterAuth(Message& msg) {

	Json::Value root;
	root["cmd"] = "reg_auth_ack";
	root["error"] = E_DEV_ERROR_OK;
	Json::Value data;

	osip_message_t * message;
	int mlen;
	osip_message_init(&message);
	osip_message_parse(message, msg.m_data.c_str(), msg.m_data.length());

	osip_from_t *hfrom = osip_message_get_from(message);

	data["did"] = hfrom->url->username;

	if (message != NULL) {

		osip_authorization_t * Sdest = NULL;
		osip_message_get_authorization(message, 0, &Sdest);

		char *pcMethod = message->sip_method;
		char *pAlgorithm = NULL;

		if (Sdest->algorithm != NULL) {
			pAlgorithm = osip_strdup_without_quote(Sdest->algorithm);
		}
		char *pUsername = NULL;
		if (Sdest->username != NULL) {
			pUsername = osip_strdup_without_quote(Sdest->username);
		}
		char *pRealm = NULL;
		if (Sdest->realm != NULL) {
			pRealm = osip_strdup_without_quote(Sdest->realm);
		}
		char *pNonce = NULL;
		if (Sdest->nonce != NULL) {
			pNonce = osip_strdup_without_quote(Sdest->nonce);
		}
		char *pNonce_count = NULL;
		if (Sdest->nonce_count != NULL) {
			pNonce_count = osip_strdup_without_quote(Sdest->nonce_count);
		}
		char *pUri = NULL;
		if (Sdest->uri != NULL) {
			pUri = osip_strdup_without_quote(Sdest->uri);
		}

		char *request_response = NULL;
		if (Sdest->response != NULL) {
			request_response = osip_strdup_without_quote(Sdest->response);
		}
		if (pAlgorithm == NULL || pUsername == NULL || pRealm == NULL || pNonce == NULL) {
			data["result"] = 0;
		} else {
			//HA1
			stringstream ha1str("");
			ha1str << pUsername << ":" << pRealm;
//			string ha1;
//			ulu_md5::make(ha1str.str(), ha1);

			string ha1;
			string auth;
			if (-1 != httpGetAuthDigest(hfrom->url->username, (char*) ha1str.str().c_str(), auth)) {
				Json::Reader reader;
				Json::Value jmessage;
				if (reader.parse(auth, jmessage)) {
					if (jmessage["data"].isString()) {
						ha1 = jmessage["data"].asString();

						stringstream ha2str("");
						ha2str << pcMethod << ":" << pUri;
						string ha2;
						ulu_md5::make(ha2str.str(), ha2);

						//resp
						stringstream respstr("");
						respstr << ha1 << ":" << pNonce << ":" << ha2;
						//	puts(respstr.str().c_str());
						string out;
						ulu_md5::make(respstr.str(), out);
						//	puts(out.c_str());

						if (msg.m_cmd == CMD_REG) {
							string uluSN = httpGetSN(hfrom->url->username);
							printf("api: %s src:%s %s %s\n", out.c_str(), request_response, ha1.c_str(), ha1str.str().c_str());
							if (out == request_response && !uluSN.empty()) {
								data["result"] = 1;
								data["SN"] = uluSN;

								for (int i = 0; i < 3; i++) {
									if (true == theApp.m_xdmgr.httpGetConfig(hfrom->url->username)) {
										break;
									} else {
										sleep(1);
									}
								}

								Debug(INFO, "reg auth OK:%s", pUsername);
							} else {
								data["result"] = 0;
								if (theApp.m_logmask & (1 << BIT_1_REG)) {
									Debug(ERROR, "reg auth Fail:%s %s", pUsername, hfrom->url->host);
								}

							}
						} else {
							if (out == request_response) {
								data["result"] = 1;
								Debug(INFO, "reg auth refresh OK:%s", pUsername);
							} else {
								data["result"] = 0;
								if (theApp.m_logmask & (1 << BIT_1_REG)) {
									Debug(ERROR, "reg auth refresh Fail:%s %s", pUsername, hfrom->url->host);
								}
							}
						}
						root["data"] = data;
						Json::FastWriter writer;
						string message = writer.write(root);

						puts(message.c_str());
						theApp.sendInterMessage(message);

					}
				}

			}
		}

		if (pAlgorithm != NULL)
			osip_free(pAlgorithm);
		if (pUsername != NULL)
			osip_free(pUsername);
		if (pRealm != NULL)
			osip_free(pRealm);
		if (pNonce != NULL)
			osip_free(pNonce);
		if (pNonce_count != NULL)
			osip_free(pNonce_count);
		if (pUri != NULL)
			osip_free(pUri);
		if (request_response != NULL)
			osip_free(request_response);
	}
	osip_message_free(message);

}

void ThreadMessageProcess::cleanResponse() {
	while (1) {
		eXosip_event_t *je = NULL;

		je = eXosip_event_wait(m_eCtx, 0, 50);
		if (je == NULL) {
			return;
		}
		eXosip_event_free(je);
		break;
	}

}

void ThreadMessageProcess::httpReportAlarm(string alarmdata) {
	CURL *curl;

	curl = curl_easy_init();
	if (curl) {

//		string urlstr = urlCodec::UrlEncode(alarmdata);
//		printf("---%s\n%s\n", alarmdata.c_str(), urlstr.c_str());
		string respBuf;
		char strURL[512] = { 0 };
		sprintf(strURL, "https://domain-service.ulucu.com/gb28181/offsite/report?%s", alarmdata.c_str());
		//	curl_easy_setopt(curl, CURLOPT_POST, 1);
		//curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, urlstr.length());
		//	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, urlstr.c_str());
		//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, strURL);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CApp::responseCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respBuf);
		int res = curl_easy_perform(curl);
		if (res != 0) {
			curl_easy_cleanup(curl);
			return;
		} else {
			puts(respBuf.c_str());
			Json::Reader reader;
			Json::Value jmessage;
			if (reader.parse(respBuf, jmessage)) {
				if (jmessage["code"].asInt() != 0) {
					Debug(ERROR, "alarm report fail:%s", alarmdata.c_str());
				}
			}
		}
		curl_easy_cleanup(curl);
	}

	return;
}
void ThreadMessageProcess::httpReportChannelstat(string id, int status) {
	CURL *curl;

	curl = curl_easy_init();
	if (curl) {
		stringstream querystr("");
		querystr << "did=" << id << "&";
		querystr << "status=" << status;
		string urlstr = urlCodec::UrlEncode(querystr.str());

		string respBuf;
		char strURL[1024] = { 0 };
		sprintf(strURL, "https://domain-service.ulucu.com/gb28181/status/report?%s", querystr.str().c_str());
		Debug(INFO, "channel stat report:%s", strURL);

		//	puts(strURL);
//		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//		curl_easy_setopt(curl, CURLOPT_POST, 1);
//		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, urlstr.length());
//		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, urlstr.c_str());
		curl_easy_setopt(curl, CURLOPT_URL, strURL);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CApp::responseCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respBuf);
		int res = curl_easy_perform(curl);
		if (res != 0) {
			curl_easy_cleanup(curl);
			return;
		} else {
			puts(respBuf.c_str());
			Json::Reader reader;
			Json::Value jmessage;
			if (reader.parse(respBuf, jmessage)) {
				if (jmessage["code"].asInt() != 0) {
					Debug(ERROR, "channel stat report fail:%s", querystr.str().c_str());
				}
			}
		}
		curl_easy_cleanup(curl);
	}

	return;

}
void ThreadMessageProcess::httpReportDevstat(string statdata) {
	CURL *curl;

	curl = curl_easy_init();
	if (curl) {

		Debug(INFO, "dev stat report :%s", statdata.c_str());
		string urlstr = urlCodec::UrlEncode(statdata);
		//	printf("---%s\n%s\n", statdata.c_str(), urlstr.c_str());

		string respBuf;
		char strURL[1024] = { 0 };
		sprintf(strURL, "https://middleware-service.ulucu.com/middleware/alarm/device_offline?v=1&msg=%s", urlstr.c_str());

		//	puts(strURL);
		//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, strURL);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CApp::responseCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respBuf);
		int res = 0;
		res = curl_easy_perform(curl);
		if (res != 0) {
			curl_easy_cleanup(curl);
			return;
		} else {
			puts(respBuf.c_str());
			Json::Reader reader;
			Json::Value jmessage;
			if (reader.parse(respBuf, jmessage)) {
				if (jmessage["code"].asInt() != 0) {
					Debug(ERROR, "dev stat report fail:%s", urlstr.c_str());
				}
			}
		}
		curl_easy_cleanup(curl);
	}

	return;
}
string ThreadMessageProcess::httpGetSN(char* devid) {
	CURL *curl;

	string devsn;
	int ret = 0;

	curl = curl_easy_init();
	if (curl) {
		char strURL[512] = { 0 };
		sprintf(strURL, "https://domain-service.ulucu.com/gb28181/device/translate?did=%s", devid);
		Debug(INFO, "getsn req:%s", strURL);

		string respBuf;
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long) CURLAUTH_DIGEST);
		//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, strURL);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CApp::responseCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respBuf);
		int res = curl_easy_perform(curl);
		if (res != 0) {
			Debug(ERROR, "http getSN fail!");

			curl_easy_cleanup(curl);
			return devsn;
		} else {
			puts(respBuf.c_str());
			Debug(INFO, "http getSN:%s  response:%s", strURL, respBuf.c_str());
			Json::Reader reader;
			Json::Value jmessage;
			if (reader.parse(respBuf, jmessage)) {
				if (jmessage["code"].asInt() == 0) {
					devsn = jmessage["data"].asString();
				}
			}
		}
		curl_easy_cleanup(curl);
	}

	return devsn;
}

int ThreadMessageProcess::httpGetAuthDigest(char* did, char* realm, string& response) {
	CURL *curl;
	CURLcode res;

	int ret = 0;

	curl = curl_easy_init();
	if (curl) {
		char strURL[512] = { 0 };
		sprintf(strURL, "https://domain-service.ulucu.com/gb28181/device/auth?device_id=%s&s=%s", did, realm);
		//	puts(strURL);
//		sprintf(strURL, "https://192.168.2.169/gb28181/device/auth?device_id=%s&s=%s", did, realm);
		Debug(DEBUG, strURL);
		string respBuf;
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long) CURLAUTH_DIGEST);
		//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, strURL);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CApp::responseCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respBuf);
		res = curl_easy_perform(curl);
		if (res != 0) {
			Debug(ERROR, "http auth fail!");

			curl_easy_cleanup(curl);
			return -1;
		} else {
			Debug(INFO, "auth resp:%s", respBuf.c_str());
			response = respBuf;
		}
		curl_easy_cleanup(curl);
	}

	return ret;
}

