char *execute_command(char *command, sqlite3 *database);
int get_command(char *command, char *command_text);
int get_update_command(char *command, char *command_text);
void printhelp(char *command);
int readfile(char *command, sqlite3 *database);
int readDNB(FILE *fp, sqlite3 *database);
int readSBS(FILE *fp, sqlite3 *database);
int update(char *command, sqlite3 *database);
