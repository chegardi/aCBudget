int readfile(char *command, sqlite3 *database);
int readDNB(FILE *fp, sqlite3 *database);
int readSBS(FILE *fp, sqlite3 *database);
int update(char *command, sqlite3 *database);
void printhelp(char *command);
int get_command(char *command, char *command_text);
char *execute_command(char *command, sqlite3 *database);
