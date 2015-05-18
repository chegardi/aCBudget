#include "../sql/sqlite3.h"	//	I have stored my sql files in its own 'sql' folder one level back

int easyExecuteSQL(int argc, char **argv);
int callback(void *NotUsed, int argc, char **argv, char **azColName);
int numbered_callback(void *NotUsed, int argc, char **argv, char **azColName);
int decreasing_callback(void *NotUsed, int argc, char **argv, char **azColName);
int insert(char *command, sqlite3 *database);
int revertOrBackup(sqlite3 *database, int isSave);
char *myselect(char *command, sqlite3 *database);