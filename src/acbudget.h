#include "../sql/sqlite3.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

#define COMMAND_LEN 256
#define PRINT_LEN 1024
#define INSERT_LEN 256
#define SELECT_LEN 256
#define DATE_LEN 11
#define COMMENT_LEN 100
#define TYPE_LEN 16
#define AMOUNT_LEN 16
#define ID_LEN 6
#define INPUT_LEN 350
char *DATABASE, *TABLE , *YEAR;

static int callback(void *NotUsed, int argc, char **argv, char **azColName);
static int indexcallback(void *NotUsed, int argc, char **argv, char **azColName);
int get_command(char *command, char *command_text);
void generate_id(char *id);
int insert(char *command, sqlite3 *database);
char *select(char* command, sqlite3 *database);
void copydate(char* date, char* token);
void copynumber(int offset, char* to, char* from);
int readfile(char *command, sqlite3 *database);
int readDNB(FILE *fp, sqlite3 *database);
int readSBS(FILE *fp, sqlite3 *database);
char *xstrtok(char *line, char *delims);
void printhelp(char *command);
char *print(char *command, sqlite3 *database);
char *config(char *command, sqlite3 *database);
void save_config(char *command);
char *execute_command(char *command, sqlite3 *database);
void configurate(char *command, sqlite3 *db);
int main(int argc, char **argv);