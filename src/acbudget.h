//	standard libraries
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

//	aCBudget libraries
#include "acbudget_sql.h"
#include "acbudget_utilities.h"
#include "acbudget_execution.h"

//	pre-defined constants
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
//	database variables
char *DATABASE, *TABLE , *MONTH, *YEAR, *CONFIG_FILENAME, *BACKUP_FILENAME;
//	utility variables
char *UNIQUE_ID;
int *P_COUNTER, *READ_COUNTER;

//	main functions
void configurate(char *command, sqlite3 *db);
char *config(char *command, sqlite3 *database);
void save_config(char *command);
int main(int argc, char **argv);
