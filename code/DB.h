/*
 * DB.h
 *
 *  Created on: Mar 3, 2020
 *      Author: wyf
 */

#ifndef DB_H_
#define DB_H_

#include <string>
#include <vector>
using namespace std;

typedef struct db_dev_item {

	string devid;
	string jsonStr;
	int updateTime;
} db_dev_item_;

class DB {
public:
	DB();
	virtual ~DB();

	int initSqlDB();
	int releaseDB();
	int insertDev(std::string& devid, std::string& jsonStr);
	int updateDev(std::string devid);
	int deleteDevByDid(string devid);
	int deleteAllDev();
	int queryDevBydid(string devid, db_dev_item_& item);
	int queryAllDev(std::vector<db_dev_item_>& list);
	int queryCount();

	int insertSes(int type, std::string& devid, std::string& jsonStr);
	int deleteSesByDid(int type, string devid);
	int querySesBydid(int type, string devid, db_dev_item_& item);
	int queryAllSes(int type, std::vector<db_dev_item_>& list);

	struct sqlite3 * db;
};

#endif /* DB_H_ */
