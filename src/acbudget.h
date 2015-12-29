//  standard libraries
#include<stdio.h>  // fprintf, printf, snprintf, FILE, stdout, stderr, scanf, 
#include<stdlib.h> // free, calloc, EXIT_FAILURE, 
#include<string.h> // strncpy, strlen, strtok, strstr, 
#include<errno.h>  // errno

//  aCBudget libraries
#include "acbudget_sql.h"
#include "acbudget_utilities.h"
#include "acbudget_execution.h"

//  pre-defined constants
#define AMOUNT_LEN 16       //  amounts bigger than 100000000000000 would be great - as income
#define COMMAND_LEN 256		//	regular length
#define COMMENT_LEN 101		//	max length of comment is 100 + \0
#define DATE_LEN 11			//	length of date: YYYY-MM-DD\0 = 11 chars
#define DAY_LEN 3           //  length of day in month DD\0
#define ID_LEN 6			//	unique id is 5 chars + \0
#define INPUT_LEN 512		//	regular length x2
#define INSERT_LEN 256		//	for use during insert-actions on SQL
#define PRINT_LEN 1024		//	regular length x4
#define SELECT_LEN 1024		//	for use during select-actions on SQL
#define TYPE_LEN 16			//	max length of type is 15 + \0

//	structs
typedef struct insert {
	char	date[DATE_LEN];
	char	comment[COMMENT_LEN];
	char	type[TYPE_LEN];
	char	amount[AMOUNT_LEN];
	char	id[ID_LEN];
}	insert_t;

//	database variables
sqlite3 *database;
char	*DATABASE, 
		*TABLE , 
		*MONTH,
		*YEAR,
		*CONFIG_FILENAME,
		*BACKUP_FILENAME;

//	utility variables
char	*UNIQUE_ID,
	    *zErrMsg;
int 	*P_COUNTER,
		*READ_COUNTER;

//	main functions
int argParser( int argc, char *argv[] );
int configurate( char *command );
int main( int argc, char **argv );
void save_config( char *command, sqlite3 *db );
void usage( void );
