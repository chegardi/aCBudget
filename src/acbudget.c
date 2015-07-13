#include "acbudget.h"

/*
 *	Fetches/creates default configuration in 'config.ini' file
 */
int configurate(char *command)
{
	#if DEBUG
	fprintf(stderr, "Configurating..\n");
	#endif
    //	Initiating default values
	DATABASE = malloc(sizeof(char)*strlen("database.db"));
	strcpy(DATABASE, "database.db");
	MONTH = malloc(sizeof(char)*strlen("01"));
	strcpy(MONTH, "01");
	YEAR = malloc(sizeof(char)*strlen("2015"));
	strcpy(YEAR, "2015");
	TABLE = malloc(sizeof(char)*strlen("r2015"));
	strcpy(TABLE, "r2015");
	READ_COUNTER = malloc(sizeof(int));
	(*READ_COUNTER) = 0;
	//	Checking for configuration file
	CONFIG_FILENAME = malloc(sizeof(char) * strlen("config.ini"));
	strcpy(CONFIG_FILENAME, "config.ini");
	FILE *config_file;
	config_file = fopen(CONFIG_FILENAME, "r");
	if (config_file == NULL) {	// file does not exist 
		#if DEBUG
		fprintf(stderr, "Creating '%s'\n", CONFIG_FILENAME);
		#endif
		config_file = fopen("config.ini", "w");
		fprintf(config_file, "#configuration file for aCBudget\n");
		fprintf(config_file, "database=%s\n", DATABASE);
		fprintf(config_file, "table=%s\n", TABLE);
		fprintf(config_file, "year=%s\n", YEAR);
		fprintf(config_file, "month=%s\n", MONTH);
		fprintf(config_file, "read=%d\n", (*READ_COUNTER));
		fclose(config_file);
		#if DEBUG
		fprintf(stderr, "'%s' created\n", CONFIG_FILENAME);
		#endif
	}	//	file did not exist
	else  {	// file exists
		int	counter = 1,
			len=0;
		char *variable, *value;
		do {
			if (fscanf(config_file, "%s\n", command) > COMMAND_LEN) {
				fprintf(stderr, "ERROR ON LINE #%d IN '%s'\nVALUE TOO LONG\n", counter, CONFIG_FILENAME);
				return 1;
			}	//	too long line read
			counter++;
			if (command[0] != '#')	{	//	not a comment
				variable = strtok(command, "=");
				value = strtok(NULL, "");
				len = strlen(variable)+1;
				if (strncmp(variable, "year", 4) == 0) {
					free(YEAR);
					YEAR = malloc(sizeof(char)*len);
					strncpy(YEAR, value, len);
					#if DEBUG
					fprintf(stderr, "YEAR=%s\n", YEAR);
					#endif
				} else if (strncmp(variable, "month", 5) == 0) {
					free(MONTH);
					MONTH = malloc(sizeof(char)*len);
					strncpy(MONTH, value, len);
				} else if (strncmp(variable, "table", 5) == 0) {
					free(TABLE);
					TABLE = malloc(sizeof(char)*len);
					strncpy(TABLE, value, len);
					#if DEBUG
					fprintf(stderr, "TABLE=%s\n", TABLE);
					#endif
				} else if (strncmp(variable, "database", 8) == 0) {
					free(DATABASE);
					DATABASE = malloc(sizeof(char)*len);
					strncpy(DATABASE, value, len);
					#if DEBUG
					fprintf(stderr, "DATABASE=%s\n", DATABASE);
					#endif
				} else if (strncmp(variable, "read", 4) == 0) {
					(*READ_COUNTER) = atoi(value);
				}
			}	//	done with line
		} while (!feof(config_file));	//	lines left to check
		fclose(config_file);
		#if DEBUG
		fprintf(stderr, "'%s' imported\n", CONFIG_FILENAME);
		#endif
		snprintf(command, COMMAND_LEN, "Settings loaded from '%s'\n", CONFIG_FILENAME);
	}	//	file existed
	return 0;
}

/*
 *	Main method to initialize program
 */
int main(int argc, char **argv)
{
	sqlite3 *database;
	char *zErrMsg = 0, command[COMMAND_LEN], *printout;
	int rc, len;
	/*
	 *	First configurates database according to a config-file (if not present, creates default)
	 *	Then accepts user input and acts accordingly.
	 */
	configurate(command);
	rc = sqlite3_open(DATABASE, &database);
	if ( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(database));
		return(1);
	}
	len = get_command(command, "main");
	//	Loop user for input while user don't want to quit
	while ((strncmp(command, "q\0", 2) != 0) && (strncmp(command, "quit\0", 5) != 0)) {
		#if DEBUG
		fprintf(stderr, "%s (%d)\n", command, len);
		#endif
		printout = execute_command(command, database);
		//	Printout if there is anything to print
		if (strlen(printout) > 1) {
			printf("%s", printout);
		}
		//	Fetch a new command
		len = get_command(command, "main");
	}
	sqlite3_close(database);
	if (free_all()) {	
		#if DEBUG
		fprintf(stderr, "free_all() did not return correctly\n.");
		#endif
		return -1;
	}
	#if DEBUG
	fprintf(stderr, "free_all() returned correctly\n.");
	#endif
	return 0;
}

/*
 *	Saves current configuration to 'config.ini'
 */
void save_config(char *command, sqlite3 *database)
{
	FILE *config_file;
	config_file = fopen(CONFIG_FILENAME, "r");
	if (config_file == NULL)  // File does not exist, creating new file
	{
		#if DEBUG
		fprintf(stderr, "No old config file, creating new.\n");
		#endif
		config_file = fopen(CONFIG_FILENAME, "w");
		fprintf(config_file, "#configuration file for aCBudget\n");
		fprintf(config_file, "database=%s\n", DATABASE);
		fprintf(config_file, "month=%s\n", MONTH);
		fprintf(config_file, "year=%s\n", YEAR);
		fprintf(config_file, "table=%s\n", TABLE);
		fprintf(config_file, "read=%d\n", (*READ_COUNTER));
		fclose(config_file);
		snprintf(command, COMMAND_LEN, "Settings saved to new file '%s'\n", CONFIG_FILENAME);
	}
	else  {	// file exists, merging
		#if DEBUG
		fprintf(stderr, "Old config file found, merging...\n");
		#endif
		char *token, tmpname[strlen(CONFIG_FILENAME)];
		tmpnam(tmpname);
		//_mktemp_s(tmpname, strlen(CONFIG_FILENAME));
		FILE *new_file = fopen(tmpname, "w");
		#if DEBUG
		fprintf(stderr, "Created file with temp-name: '%s'\n", tmpname);
		#endif
		int counter = 1, file_size = 0;
		do {
			if (fgets(command, COMMAND_LEN, config_file) != NULL) {
				counter++;
				if (command[0] != '#')  {	// config-variabel 
					token = strtok(command, "="); // '=' is config variabel delim
					if (strncmp(token, "year", 4) == 0) {  // year is stored
						fprintf(new_file, "year=%s\n", YEAR);
					} else if (strncmp(token, "month", 5) == 0) {	//	month is stored
						fprintf(new_file, "month=%s\n", MONTH);
					} else if (strncmp(token, "table", 5) == 0) { // database table is stored
						fprintf(new_file, "table=%s\n", TABLE);
					} else if (strncmp(token, "database", 8) == 0) {  // database name is stored
						fprintf(new_file, "database=%s\n", DATABASE);
					} else if (strncmp(token, "read", 4) == 0) {
						fprintf(new_file, "read=%d\n", (*READ_COUNTER));
					}
				}
				else  {	// not a config variabel, user has manually added lines
					fprintf(new_file, "%s", command);
				}
			}
			else if (command == NULL)  {	// not supposed to happen
				fprintf(stderr, "ERROR ON LINE #%d IN 'config.ini'\n", counter);
				exit(EXIT_FAILURE);
			}
		} while (!feof(config_file) && counter < 10);
		fclose(config_file);
		fclose(new_file);
		if (remove(CONFIG_FILENAME))  {	// failed to remove old config file
			printf("Failed to remove old 'config.ini';\n%s\n", strerror(errno));
		}
		if (rename(tmpname, CONFIG_FILENAME))  {	// failed to rename new config file
			printf("Failed to rename new 'config.ini';\n%s\n", strerror(errno));
		}
		snprintf(command, COMMAND_LEN, "Settings saved to '%s'\n", CONFIG_FILENAME);
	}
	return;
}