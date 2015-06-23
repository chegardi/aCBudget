#include "../sql/sqlite3.h"	//	I have stored my sql files in its own 'sql' folder one level back

int callback(void *NotUsed, int argc, char **argv, char **azColName);
int easyExecuteSQL(int argc, char **argv);
int insert(char *command, sqlite3 *database);
char *myselect(char *command, sqlite3 *database);
int numbered_callback(void *NotUsed, int argc, char **argv, char **azColName);
int revert_or_backup(sqlite3 *database, int isSave);