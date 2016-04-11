/*
 * 将文件系统权限属🐷保存到sqlite数据库中。
 * cc perba.c -lsqlite3编译程序，如果系统中没有sqlite库则在其他系统中使用：
 * cc perba.c -lsqlite3 -static编译拷贝即可
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ftw.h>
#include <unistd.h>
#include <sqlite3.h>
#include <assert.h>

sqlite3 *db;
char const *zpathname;
char *zdb;


int backtodb(void);
int restorfromdb(void);
int writesql(const char *path, const uid_t uid, const gid_t git,
	     const mode_t mode);
int writesql_1(const char *filename, const struct stat *stat, int flag,
	       struct FTW *ftw);
int reback(void *arg, int cols, char **data, char **colname);

int main(int argc, char *argv[])
{
	if (argc == 1) {
		printf("usage:prog dbfile -{b|r} directory!\n");
		return 1;
	}
	if (argc != 4) {
		printf("usage:prog dbfile -{b|r} directory!\n");
		return 1;
	}

	if (argv[3][0] != '/') {
		printf("the directory must be begin '/'\n");
		return -1;
	}
/*	if (argv[2][0] != '-' || (argv[2][1] == 'b' || argv[2][1] == 'r') ){
		printf("usage:prog dbfile -{b|r} directory!\n");
		return 1;
	}*/
	zdb = argv[1];
	zpathname = argv[3];

	if (argv[2][1] == 'b') {
		if (backtodb() == -1) {
			return -1;
		}
	} else if (argv[2][1] == 'r') {
		if (restorfromdb() == -1) {
			return -1;
		}
	}


	return 0;
}

int writesql(const char *path, const uid_t uid, const gid_t gid,
	     const mode_t mode)
{
	int rc;
	sqlite3_stmt *pstmt;
	char sql[512] = { 0x0 };
	sprintf(sql,
		"insert into pathper(pathname,uid,gid,pathper) values (\"%s\",%d,%d,?)",
		path, uid, gid);
	printf("%s\n", sql);
	rc = sqlite3_prepare(db, sql, -1, &pstmt, 0);
	if (rc != SQLITE_OK) {
		return -1;
	}
	sqlite3_bind_blob(pstmt, 1, &mode, sizeof(mode_t), SQLITE_STATIC);
	rc = sqlite3_step(pstmt);
	assert(rc != SQLITE_ROW);
	rc = sqlite3_finalize(pstmt);
	return 0;
}

int writesql_1(const char *path, const struct stat *ostat, int flag,
	       struct FTW *ftw)
{
	int rc;
	sqlite3_stmt *pstmt;
	char sql[512] = { 0x0 };
	sprintf(sql,
		"insert into pathper (pathname,uid,gid,pathper) values(\"%s\",%d,%d,?)",
		path, ostat->st_uid, ostat->st_gid);
	/*
	   printf("%s\n",sql);
	   don't show sql.
	 */
	rc = sqlite3_prepare(db, sql, -1, &pstmt, 0);
	if (rc != SQLITE_OK) {
		return -1;
	}
	sqlite3_bind_blob(pstmt, 1, &(ostat->st_mode), sizeof(mode_t),
			  SQLITE_STATIC);
	rc = sqlite3_step(pstmt);
	assert(rc != SQLITE_ROW);
	rc = sqlite3_finalize(pstmt);

	return 0;
}

int backtodb(void)
{
	if (sqlite3_open_v2(zdb, &db,
			    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			    NULL) != SQLITE_OK) {
		return -1;
	}
	sqlite3_exec(db, "create table if not exists pathper (\
			pathname varchar(200),\
			uid int,\
			gid int,\
			pathper blob\
			)", NULL, NULL, NULL);
	if (nftw(zpathname, writesql_1, 20, FTW_PHYS | FTW_MOUNT) == -1) {
		return -1;
	}
	return 0;
}

int restorfromdb(void)
{
	if (sqlite3_open_v2(zdb, &db,
			    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			    NULL) != SQLITE_OK) {
		return -1;
	}
	char sql[512] = { 0x0 };
	sprintf(sql, "select pathname,uid,gid,pathper from pathper");
	if (sqlite3_exec(db, sql, reback, NULL, NULL) != SQLITE_OK) {
		return -1;
	}
	return 0;
}

int reback(void *arg, int cols, char **data, char **colname)
{
	struct per {
		char pathname[1024];
		uid_t uid;
		gid_t gid;
		mode_t mode;
	};
	struct per oper;
	memset(&oper, 0, sizeof(struct per));
	int i = 0;
	if (arg) {
	}

	memcpy(oper.pathname, data[0], strlen(data[0]));
	oper.uid = atoi(data[1]);
	oper.gid = atoi(data[2]);
	memcpy(&oper.mode, data[3], sizeof(mode_t));
	/*
	   printf("%s:%d:%d:MMMM",oper.pathname,oper.uid,oper.gid);

	   if (oper.mode & S_IFDIR) {
	   printf("DIR");
	   }else if (oper.mode & S_IFREG) {
	   printf("FILE");
	   }
	   printf("\n");
	   we don't need this display!
	 */

	if (chown(oper.pathname, oper.uid, oper.gid) == -1) {
		return 100;
	}
	if (chmod(oper.pathname, oper.mode) == -1) {
		return 200;
	}
	return 0;
}

