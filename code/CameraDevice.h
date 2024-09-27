#ifndef CAMERADEVICE_H
#define CAMERADEVICE_H

#include <stdint.h>
#include <stdio.h>
#include <list>
#include <string>
#include <eXosip2/eXosip.h>
#include <vector>
#include "App.h"

using namespace std;
class CameraDevice {
public:
	CameraDevice();
	CameraDevice(struct eXosip_t *);
	virtual ~CameraDevice();
	bool operator ==(CameraDevice node);
	int setChannelStat(string channelid, int stat);
	int findChannelStat(string channelid);
	int findChannelStat1(string channelid);

public:
	string m_uluSN;
	string m_devid;
	string m_dev_remote_ip;
	int m_dev_remote_port;
	int m_dev_status;
	int m_last_keep_time;
	int m_last_update_time;
	int m_reg_tid;
	string m_reg_nonce;
	int m_dev_type; //1 ipc, 2 nvr 3 channel
	int m_querySN;
	int m_cseq;
	int m_last_subscribe_time;
	int m_sub_did;

	vector<channelStat> m_channels;

};

#endif // CAMERADEVICE_H
