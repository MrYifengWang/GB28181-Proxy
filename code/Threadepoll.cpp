#include "App.h"
#include "Threadepoll.h"
#include <signal.h>

CThreadepoll::CThreadepoll(int identity) {
	m_epoll = epoll_create(5000);
	m_identity = identity;
	m_totalsock = 0;
	m_lastchecktime = 0;
}

CThreadepoll::~CThreadepoll(void) {
	close(m_epoll);
//	eXosip_quit(m_eCtx);
//	osip_free(m_eCtx);
}

void CThreadepoll::startThread(int identity) {
	CThreadepoll *thread = new CThreadepoll(identity);
	assert(thread);

	thread->setAutoDelete(true);
	thread->start();
}

bool CThreadepoll::onStart() {

	{
		struct sigaction act;
		memset(&act, 0, sizeof(act));
		sigemptyset(&act.sa_mask);
		act.sa_handler = SIG_IGN;
		if (sigaction(SIGPIPE, &act, NULL) < 0) {

		}
	}

	{
		if (!m_tListener.create(SOCK_STREAM)) {
			assert(0);
			return false;
		}

		if (!m_tListener.reuseAddress()) {
			assert(0);
			return false;
		}
		m_tListener.enableNonblock(true);

		if (!m_tListener.bind(theApp.m_devmgr_port)) {
			exit(0);
		}

		if (!m_tListener.listen(5)) {
			assert(0);
			return false;
		}

		puts("listening......");
		struct epoll_event ev;

		ev.events = EPOLLIN;
		ev.data.fd = (int) (SOCKET) m_tListener;
		if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, (int) (SOCKET) m_tListener, &ev) < 0) {
			assert(0);
		}
		m_totalsock = 1;
	}
	{
		int port = theApp.m_localUdpPort;

		m_tSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_tSocket < 0) {
			return false;
		}
		int p = 1;
		::setsockopt(m_tSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &p, sizeof(p));

		while (port < 65535) {

			struct sockaddr_in peerAddr;
			peerAddr.sin_family = AF_INET;
			peerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			peerAddr.sin_port = htons(port);

			int ret = ::bind(m_tSocket, (struct sockaddr*) &peerAddr, sizeof(peerAddr));

			if (ret < 0) {
				int err = errno;
				if (err == 98) {
					port += 1;
					continue;
				} else {
					break;
				}
			}
			theApp.m_localUdpPort = port;

		}

		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = (int) (SOCKET) m_tSocket;
		if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, (int) (SOCKET) m_tSocket, &ev) < 0) {
		}
		m_totalsock++;
	}

	return true;
}
//void CThreadepoll::cleanResponse() {
//	while (1) {
//		eXosip_event_t *je = NULL;
//
//		je = eXosip_event_wait(m_eCtx, 0, 50);
//		if (je == NULL) {
//			return;
//		}
//		eXosip_event_free(je);
//		break;
//	}
//
//}
void CThreadepoll::checkTimeout() {
	int curtime = time(NULL);
	if (curtime - m_lastchecktime > 5) {
		m_lastchecktime = curtime;
		for (int i = 0; i < m_agents.size(); i++) {
			if (m_agents[i]->checkKeepalive(curtime) == false) {
				m_agents[i]->release();
				break;
			}
		}
	}
	//check sip service thread
	if (curtime - theApp.m_sip_lastchecktime > 10) {
		theApp.m_sip_lastchecktime = curtime;
		Json::Value root;
		root["cmd"] = "sip_service_check";

		Json::FastWriter writer;
		string message = writer.write(root);
		theApp.sendInterMessage(message, 0);

	}
}

void CThreadepoll::run() {

	int retnum;
	while (true) {
		checkTimeout();
		retnum = epoll_wait(m_epoll, m_events, m_totalsock, 5 * 1000);

		if (retnum < 0) {
			if (errno == EINTR) {
				puts("bug while intr");
				continue;
			}
			Debug(ERROR, "epoll error %d\n", errno);
			assert(0);
		}
		for (int i = 0; i < retnum; i++) {
			if (m_events[i].data.fd == (int) m_tListener) {

				SOCKET handle;

				sockaddr_in remoteaddr;
				handle = m_tListener.accept(remoteaddr);
//				printf("accept %d %d\n", handle, remoteaddr.sin_port);

				char ipbuf[64] = { 0 };
				int pport = ntohs(remoteaddr.sin_port);
				strcpy(ipbuf, inet_ntoa(remoteaddr.sin_addr));

				int agentid = time(NULL) + remoteaddr.sin_port + handle;
				Agent * member = new Agent(handle, m_epoll, this, agentid, ipbuf, pport);
				struct epoll_event ev;

				ev.events = EPOLLIN;
				ev.data.ptr = member;

				if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, (int) handle, &ev) < 0) {
					delete member;
					continue;
				} else {
					m_totalsock++;
					m_agents.push_back(member);
					member->start();
				}

			}

			else if (m_events[i].data.fd == (int) m_tSocket) {
				struct sockaddr_in peerAddr;
				socklen_t len = sizeof(peerAddr);

				char udpBuffer[1024 * 8] = { 0 };
				int cLen = recvfrom(m_tSocket, udpBuffer, 1024 * 8, 0, (struct sockaddr*) &peerAddr, &len);
				if (cLen < 0) {
					int err = errno;
					Debug(ERROR, "inter udp recv fail:%d", errno);
					return;
				} else {
					handleSipResponse(udpBuffer, strlen(udpBuffer));
				}
			} else {

				for (int j = 0; j < m_agents.size(); j++) {
					bool found = false;
					if (m_agents[j] == m_events[i].data.ptr) {
						found = true;
						m_agents[j]->handleEvent(m_events[i].events);
					}
					if (!found) {

					}
				}
			}
		}
	}

	Debug(ERROR, "epoll thread close %d", this->m_identity);
}
void CThreadepoll::handleSipResponse(char* buf, int len) {

	Json::Reader reader;
	Json::Value jmessage;
	string msg(buf, len);
	if (reader.parse(msg, jmessage)) {

		if (jmessage["type"].asString() == "sip") {
			puts("epoll recv inter response");
			osip_message_t * message;
			osip_message_init(&message);
			string datastr = jmessage["data"].asString();
			osip_message_parse(message, datastr.c_str(), datastr.length());

			if (message != NULL) {
				osip_to_t *hto = osip_message_get_to(message);
//				printf("epoll agentid == %s\n", hto->url->username);
				Agent* pMember = findUser(atoi(hto->url->username));
				if (pMember != NULL) {
					pMember->handleSipResponse(message);
				} else {
					Debug(ERROR, "httpAgent time out 1 %s\n%s", hto->url->username, datastr.c_str());
				}
			}
			osip_message_free(message);
		} else if (jmessage["type"].asString() == "json") {
			string aid = jmessage["agentid"].asString();
			if (aid == "0") {
				theApp.m_sip_lastkeeptime = time(NULL);
				return;
			}
			Agent* pMember = findUser(atoi(aid.c_str()));
			if (pMember != NULL) {
				pMember->handleJsonResponse(jmessage["data"]);
			} else {
				Debug(ERROR, "httpAgent time out %s\n%s", aid.c_str(), buf);
			}
		}
	}
}
void CThreadepoll::handleJsonResponse(char* buf, int len) {

}
void CThreadepoll::checkOldStreamServer(string ip, int tm) {
	for (int i = 0; i < m_agents.size(); i++) {
		if (m_agents[i]->m_agent_type == 1) {
			if (m_agents[i]->m_peerip == ip) {
				if (m_agents[i]->m_stream_login_tm != tm) {
					m_agents[i]->m_isdead = true;
					break;
				}
			}

		}
	}
}

Agent* CThreadepoll::findUser(int aid) {
	for (int i = 0; i < m_agents.size(); i++) {
		if (m_agents[i]->m_agentid == aid) {
			return m_agents[i];
		}
	}
	return NULL;
}
bool CThreadepoll::delUser(int aid) {
	for (int i = 0; i < m_agents.size(); i++) {
		if (m_agents[i]->m_agentid == aid) {
			m_agents.erase(m_agents.begin() + i);
			break;
		}
	}
	return true;
}

bool CThreadepoll::delStreamUser(string sipid, int tm, string token) {
	for (int i = 0; i < m_agents.size(); i++) {
		if (m_agents[i]->m_agent_type == 1 && m_agents[i]->m_sip_server_id == sipid && m_agents[i]->m_stream_login_tm != tm) {
			Agent* pagent = m_agents[i];
			m_agents.erase(m_agents.begin() + i);
			delete pagent;
			return true;
		}
	}
	return false;
}

