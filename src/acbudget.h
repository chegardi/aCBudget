#include "sql/sqlite3.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define COMMAND_LEN 256
#define PRINT_LEN 1024
#define INSERT_LEN 256
#define SELECT_LEN 256
#define ID_LEN 6
#define INPUT_LEN 350
#define DATABASE "regnskap.db"

unsigned long INDEX = 0L;

static int callback(void *NotUsed, int argc, char **argv, char **azColName);
static int indexcallback(void *NotUsed, int argc, char **argv, char **azColName);
int get_command(char *command, char *command_text);
void generate_id(char *id);
int insert(char *command, sqlite3 *database);
char *select(sqlite3 *database);
void copydate(char* date, char* token);
void copynumber(int offset, char* to, char* from);
int readfile(char *command, sqlite3 *database);
int readDNB(FILE *fp, sqlite3 *database);
int readSBS(FILE *fp, sqlite3 *database);
char *xstrtok(char *line, char *delims);
void printhelp(char *command);
char *execute_command(char *command, sqlite3 *database);
int main(int argc, char **argv);