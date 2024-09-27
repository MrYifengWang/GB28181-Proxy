#include "CameraDevice.h"

CameraDevice::CameraDevice() {
	m_dev_remote_port = 0;
	m_dev_status = 0;
	m_last_keep_time = time(NULL);
	m_last_update_time = time(NULL);
	m_reg_tid = 0;
	m_dev_type = 0;
	m_uluSN = "-1";
	m_cseq = 30;
	m_last_subscribe_time = 0;
	m_sub_did = 0;
}

CameraDevice::~CameraDevice() {

}
bool CameraDevice::operator ==(CameraDevice node) {
	bool isSame = false;

	if ((node.m_devid.compare(this->m_devid) == 0) && (node.m_dev_remote_ip.compare(this->m_dev_remote_ip) == 0)) {
		isSame = true;
	}

	return isSame;
}
int CameraDevice::setChannelStat(string channelid, int stat) {
	int changed = 0;
	bool found = false;
	//Debug(INFO, "channel check: dev:%s %s %d %s %d", this->m_devid.c_str(), this->m_uluSN.c_str(), this->m_channels.size(),
	//		this->m_dev_remote_ip.c_str(), this->m_dev_status);;
	for (int i = 0; i < m_channels.size(); i++) {
		//	Debug(ERROR, "channel check:size %d i %d id %s stat %d %d", m_channels.size(), i, m_channels[i].channelid.c_str(), m_channels[i].stat, stat);

		if (m_channels[i].channelid == channelid) {
			int curtime = time(NULL);

			changed = m_channels[i].stat != stat ? 1 : 0;
			if (changed == 1) {
				Debug(INFO, "channel stat changed:%s %d", channelid.c_str(), stat);
			} else {
				if (curtime - m_channels[i].m_last_report_time > 30 * 60) {
					changed = 2;
					Debug(INFO, "channel stat report after 1800s timeout:%s %d", channelid.c_str(), stat);
				}
			}

			if (changed > 0) {
				m_channels[i].m_last_report_time = curtime;
			}
			m_channels[i].stat = stat;
			found = true;
			break;
		}
	}
	if (!found) {
		Debug(INFO, "channel check:push channel %d", m_channels.size());
		channelStat item;
		item.channelid = channelid;
		item.stat = stat;
		m_channels.push_back(item);
		changed = 1;
	}
	return changed;
}
int CameraDevice::findChannelStat(string channelid) {
	bool found = false;
	int stat = 0;
	for (int i = 0; i < m_channels.size(); i++) {
		if (m_channels[i].channelid == channelid) {
			stat = m_channels[i].stat;
			found = true;
			break;
		}
	}

	if (!found) {
		return -1;
	} else {
		return 1;
	}
}

int CameraDevice::findChannelStat1(string channelid) {
	bool found = false;
	int stat = 0;
	for (int i = 0; i < m_channels.size(); i++) {
		if (m_channels[i].channelid == channelid) {
			stat = m_channels[i].stat;
			found = true;
			break;
		}
	}

	if (!found) {
		return -1;
	} else {
		return stat;
	}

}

