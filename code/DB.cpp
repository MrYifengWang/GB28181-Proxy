/*
 * DB.cpp
 *
 *  Created on: Mar 3, 2020
 *      Author: wyf
 */

#include "DB.h"
#include "common/sqlite3.h"
#include "debugLog.h"

DB::DB() {
	// TODO Auto-generated constructor stub

}

DB::~DB() {
	// TODO Auto-generated destructor stub
}

#define sqlDBName	"devdb.db"
int DB::initSqlDB() {
	int rc = sqlite3_open( sqlDBName, &db);
	{
		char *errMsg;

		if (rc == SQLITE_OK) {
			printf("open sql ok!");
			char sql[1024] = "CREATE TABLE devdb("
					"devid            TEXT     primary key,"
					"jsonStr           TEXT    NOT NULL,"
					"updatetime         DATETIME NOT NULL	 DEFAULT current_timestamp);";

			rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

			if (rc != SQLITE_OK) {
				Debug(ERROR, "create table :%d,ret:%s\n", rc, errMsg);
			}
			sqlite3_free(errMsg);
		}
	}
	{
		char *errMsg;

		if (rc == SQLITE_OK) {
			printf("open sql ok!");
			char sql[1024] = "CREATE TABLE t_session_1("
					"devid            TEXT     primary key,"
					"jsonStr           TEXT    NOT NULL,"
					"updatetime         DATETIME NOT NULL	 DEFAULT current_timestamp);";

			rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

			if (rc == SQLITE_OK) {
				Debug(ERROR, "create table :%d,ret:%s\n", rc, errMsg);
			}
			sqlite3_free(errMsg);

		}
	}
	{
		char *errMsg;

		if (rc == SQLITE_OK) {
			printf("open sql ok!");
			char sql[1024] = "CREATE TABLE t_session_2("
					"devid            TEXT     primary key,"
					"jsonStr           TEXT    NOT NULL,"
					"updatetime         DATETIME NOT NULL	 DEFAULT current_timestamp);";

			rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

			if (rc != SQLITE_OK) {
				Debug(ERROR, "create table :%d,ret:%s\n", rc, errMsg);
			}
			sqlite3_free(errMsg);

		}
	}
	{
		char *errMsg;

		if (rc == SQLITE_OK) {
			printf("open sql ok!");
			char sql[1024] = "CREATE TABLE t_session_3("
					"devid            TEXT     primary key,"
					"jsonStr           TEXT    NOT NULL,"
					"updatetime         DATETIME NOT NULL	 DEFAULT current_timestamp);";

			rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

			if (rc != SQLITE_OK) {
				Debug(ERROR, "create table :%d,ret:%s\n", rc, errMsg);
			}
			sqlite3_free(errMsg);

		}
	}

}
int DB::releaseDB() {
	sqlite3_close(db);
}

int queryCallback(void *para, int n_column, char **column_value, char **column_name) {

	db_dev_item_* pItem = (db_dev_item_*) para;
	if (n_column >= 6) {
		pItem->devid = column_value[0];
		pItem->jsonStr = column_value[1];
		pItem->updateTime = atoi(column_value[2]);
	}
	return 0;
}

int DB::updateDev(std::string devid) {
//	puts("update dev db");
	char* errMsg;
	char sqlStr[2048] = { 0 };
	sprintf(sqlStr, "update devdb set updatetime = '%d' where devid='%s'", time(NULL), devid.c_str());
	int rc = sqlite3_exec(db, sqlStr, NULL, NULL, &errMsg);

	if (rc != SQLITE_OK) {
		Debug(ERROR, "update data error!:%s", errMsg);
	}
	if (errMsg != NULL) {
		sqlite3_free(errMsg);
	}
	return 0;
}

int DB::insertSes(int type, std::string& devid, std::string& jsonStr) {
	//lock_db.lock();
	return 0;
	printf("insert %s\n", jsonStr.c_str());
	char* errMsg;
	char sqlStr[2048] = { 0 };
	sprintf(sqlStr, "replace into t_session_%d (devid,jsonStr,updatetime) values('%s','%s','%d')", type, devid.c_str(), jsonStr.c_str(),
			time(NULL));
	int rc = sqlite3_exec(db, sqlStr, NULL, NULL, &errMsg);

	if (rc != SQLITE_OK) {
		Debug(ERROR, "insert data error!:%s", errMsg);
	}
	if (errMsg != NULL) {
		sqlite3_free(errMsg);
	}
	//lock_db.unlock();
	return 0;
}
int DB::deleteSesByDid(int type, string devid) {
	return 0;
	char* errMsg;
	char sqlStr[2048] = { 0 };
	sprintf(sqlStr, "delete from t_session_%d where devid=%s", type, devid.c_str());
	int rc = sqlite3_exec(db, sqlStr, NULL, NULL, &errMsg);

	if (rc != SQLITE_OK) {
		Debug(ERROR, "insert data error!:%s", errMsg);
	}
	if (errMsg != NULL) {
		sqlite3_free(errMsg);
	}
}
int DB::querySesBydid(int type, string devid, db_dev_item_& item) {
}
int DB::queryAllSes(int type, std::vector<db_dev_item_>& list) {
	char** pResult;
	int nRow, nCol;
	char* errmsg;
	char sqlstr[256] = { 0 };
	sprintf(sqlstr, "select * from t_session_%d", type);
	int nResult = sqlite3_get_table(db, sqlstr, &pResult, &nRow, &nCol, &errmsg);

	if (nResult != SQLITE_OK) {
		sqlite3_free(errmsg);
		return 0;
	}

	int nIndex = nCol;
	for (int i = 0; i < nRow; i++) {
		db_dev_item_ pItem;
		pItem.devid = pResult[nIndex];
		pItem.jsonStr = pResult[nIndex + 1];
		pItem.updateTime = atoi(pResult[nIndex + 2]);
		list.push_back(pItem);
		nIndex += nCol;

	}

	sqlite3_free_table(pResult);
	return 0;
}

int DB::insertDev(std::string& devid, std::string& jsonStr) {
	//lock_db.lock();
//	printf("insert %s\n", jsonStr.c_str());
	char* errMsg;
	char sqlStr[2048] = { 0 };
	sprintf(sqlStr, "replace into devdb(devid,jsonStr,updatetime) values('%s','%s','%d')", devid.c_str(), jsonStr.c_str(), time(NULL));
	int rc = sqlite3_exec(db, sqlStr, NULL, NULL, &errMsg);

	if (rc != SQLITE_OK) {
		Debug(ERROR, "local DB fail:%s:%s", sqlStr, errMsg);
	}
	if (errMsg != NULL) {
		sqlite3_free(errMsg);
	}
	//lock_db.unlock();
	return 0;

}
int DB::deleteDevByDid(string did) {
	//lock_db.lock();
	char* errMsg;
	char sqlStr[2048] = { 0 };
	sprintf(sqlStr, "delete from devdb where devid=%s", did.c_str());
	int rc = sqlite3_exec(db, sqlStr, NULL, NULL, &errMsg);
	Debug(DEBUG, sqlStr);

	if (rc != SQLITE_OK) {
		Debug(ERROR, "local DB fail:%s :%s", sqlStr, errMsg);
	}
	if (errMsg != NULL) {
		sqlite3_free(errMsg);
	}
	//lock_db.unlock();
}

int DB::deleteAllDev() {
	//lock_db.lock();
	char* errMsg;
	char sqlStr[2048] = { 0 };
	sprintf(sqlStr, "delete from devdb  where 1=1");
	int rc = sqlite3_exec(db, sqlStr, NULL, NULL, &errMsg);
	Debug(DEBUG, sqlStr);

	if (rc != SQLITE_OK) {
		Debug(ERROR, "local DB fail:%s :%s", sqlStr, errMsg);
	}
	if (errMsg != NULL) {
		sqlite3_free(errMsg);
	}
	//lock_db.unlock();
}

int DB::queryDevBydid(string devid, db_dev_item_& item) {
	//lock_db.lock();

	char* errMsg;
	int rc = sqlite3_exec(db, "select * from devdb where devid=%s", queryCallback, &item, &errMsg);

	if (rc != SQLITE_OK) {
		Debug(ERROR, "query failed!");
	}
	if (errMsg != NULL) {
		sqlite3_free(errMsg);
	}
	//lock_db.unlock();

	return 0;
}

int DB::queryAllDev(std::vector<db_dev_item_>& list) {
	char** pResult;
	int nRow, nCol;
	char* errmsg;
	int nResult = sqlite3_get_table(db, "select * from devdb", &pResult, &nRow, &nCol, &errmsg);
	if (nResult != SQLITE_OK) {
		sqlite3_free(errmsg);
		return 0;
	}

	int nIndex = nCol;
	for (int i = 0; i < nRow; i++) {
//		string tmpstr = pResult[nIndex];
//		if (tmpstr.length() < 20) {
//			continue;
//		}
		db_dev_item_ pItem;
		pItem.devid = pResult[nIndex];
		pItem.jsonStr = pResult[nIndex + 1];
		pItem.updateTime = atoi(pResult[nIndex + 2]);
		list.push_back(pItem);
		nIndex += nCol;

	}

	sqlite3_free_table(pResult);
	return 0;
}

int querycountCallback(void *para, int n_column, char **column_value, char **column_name) {
	int* pItem = (int*) para;
	if (n_column >= 1) {
		(*pItem) = atoi(column_value[0]);
	}
	return 0;
}
int DB::queryCount() {
	int count = 0;
	//lock_db.lock();

	char* errMsg;
	int rc = sqlite3_exec(db, "select count(*) from devdb", querycountCallback, &count, &errMsg);

	if (rc != SQLITE_OK) {
		count = 60001;
		Debug(ERROR, "query failed!");
	}
	if (errMsg != NULL) {
		sqlite3_free(errMsg);
	}
	//lock_db.unlock();

	return count;
}
