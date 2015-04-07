#include "acbudget.h"

/*
 *	Fetches/creates default configuration in 'config.ini' file
 */
void configurate(char *command, sqlite3 *db)
{
	#ifdef DEBUG
	fprintf(stderr, "Configurating..\n");
	#endif
    //	Initiating default values
	DATABASE = malloc(sizeof(char)*strlen("regnskap.db"));
	strcpy(DATABASE, "regnskap.db");
	MONTH = malloc(sizeof(char)*strlen("01"));
	strcpy(MONTH, "01");
	YEAR = malloc(sizeof(char)*strlen("2014"));
	strcpy(YEAR, "2014");
	TABLE = malloc(sizeof(char)*strlen("r2014"));
	strcpy(TABLE, "r2014");
	//	Checking for configuration file
	CONFIG_FILENAME = malloc(sizeof(char) * strlen("config.ini"));
	strcpy(CONFIG_FILENAME, "config.ini");
	FILE *config_file;
	config_file = fopen(CONFIG_FILENAME, "r");
	if (config_file == NULL)  // file does not exist
	{
		#ifdef DEBUG
		fprintf(stderr, "Creating '%s'\n", CONFIG_FILENAME);
		#endif
		config_file = fopen("config.ini", "w");
		fprintf(config_file, "#configuration file for aCBudget\n");
		fprintf(config_file, "database=%s\n", DATABASE);
		fprintf(config_file, "table=%s\n", TABLE);
		fprintf(config_file, "year=%s\n", YEAR);
		fprintf(config_file, "month=%s\n", MONTH);
		fclose(config_file);
		#ifdef DEBUG
		fprintf(stderr, "'%s' created\n", CONFIG_FILENAME);
		#endif
		snprintf(command, COMMAND_LEN, "'%s' created and default settings loaded\n", CONFIG_FILENAME);
	}
	else  // file exists
	{
		int counter = 1;
		char *token;
		do {
			if (fscanf(config_file, "%s\n", command) > COMMAND_LEN)
			{
				fprintf(stderr, "ERROR ON LINE #%d IN '%s'\nVALUE TOO LONG\n", counter, CONFIG_FILENAME);
				exit(EXIT_FAILURE);
			}
			counter++;
			if (command[0] != '#')	//	not a comment
			{
				token = strtok(command, "=");
				if (strncmp(token, "year", 4) == 0) {
					token = strtok(NULL, "");
					free(YEAR);
					YEAR = malloc(sizeof(char)*strlen(token));
					strcpy(YEAR, token);
					#ifdef DEBUG
					fprintf(stderr, "YEAR=%s\n", YEAR);
					#endif
				} else if (strncmp(token, "month", 5) == 0) {
					token = strtok(NULL, "");
					free(MONTH);
					MONTH = malloc(sizeof(char)*strlen(token));
					strcpy(MONTH, token);
				} else if (strncmp(token, "table", 5) == 0) {
					token = strtok(NULL, "");
					free(TABLE);
					TABLE = malloc(sizeof(char)*strlen(token));
					strcpy(TABLE, token);
					#ifdef DEBUG
					fprintf(stderr, "TABLE=%s\n", TABLE);
					#endif
				} else if (strncmp(token, "database", 8) == 0) {
					token = strtok(NULL, "");
					free(DATABASE);
					DATABASE = malloc(sizeof(char)*strlen(token));
					strcpy(DATABASE, token);
					#ifdef DEBUG
					fprintf(stderr, "DATABASE=%s\n", DATABASE);
					#endif
				}
			}
		} while (!feof(config_file));
		fclose(config_file);
		#ifdef DEBUG
		fprintf(stderr, "'%s' imported\n", CONFIG_FILENAME);
		#endif
		snprintf(command, COMMAND_LEN, "Settings loaded from '%s'\n", CONFIG_FILENAME);
	}	
}

/*
 *	Provides runtime configuration-options
 */
char *config(char *command, sqlite3 *database)
{
	int len;
	char *token;
	len = get_command(command, "config"); command[len-1] = '\0';
	while ((strncmp(command, "e\0", 2) != 0) && (strncmp(command, "end\0", 4) != 0)) {
		if ((strncmp(command, "sv\0", 3) == 0) || (strncmp(command, "save\0", 5) == 0))
		{
			save_config(command);
			printf("%s", command);
		}
		else if ((strncmp(command, "ld\0", 3) == 0) || (strncmp(command, "load\0", 5) == 0))
		{
			configurate(command, database);
			printf("%s", command);
		}
		else if ((strncmp(command, "sw\0", 3) == 0) || (strncmp(command, "show\0", 5) == 0))
		{
			printf("Database: %s\nTable: %s\nMonth: %s\nYear: %s\n", DATABASE, TABLE, MONTH, YEAR);
		}
		else if ((strncmp(command, "yr\0", 2) == 0) ||(strncmp(command, "year\0", 5) == 0))
		{
			printf("Year=%s\n", YEAR);
		}
		else if ((strncmp(command, "mn\0", 3) == 0) || (strncmp(command, "month\0", 6) == 0))
		{
			printf("Month=%s\n", MONTH);
		}
		else if ((strncmp(command, "tb\0", 3) == 0) || (strncmp(command, "table\0", 6) == 0))
		{
			printf("Table=%s\n", TABLE);
		}
		else if ((strncmp(command, "db\0", 3) == 0) || (strncmp(command, "database\0", 9) == 0))
		{
			printf("Database=%s\n", DATABASE);
		}
		else if ((strncmp(command, "h\0", 2) == 0) || (strncmp(command, "help\0", 5) == 0))
		{
			fprintf(stdout,
			"Commands withing config:\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%14s - %s\n",
			"h", "help", "Displays this help text",
			"sv", "save", "Saves the current configuration to file",
			"ld", "load", "Loads configuration from file",
			"sw", "show", "Shows the current configuration in program",
			"db", "database", "Displays current database",
			"tb", "table", "Displays current table in database",
			"yr", "year", "Displays default year",
			"mn", "month", "Displays default month",
			"variable=value", "sets variable to new value");
		}
		else
		{
			len = strlen(command);
			token = xstrtok(command, "=");
			if (len != strlen(token))
			{
				if (strncmp(token, "year\0", 5) == 0)
				{
					token = xstrtok(NULL, "");
					free(YEAR);
					YEAR = malloc(sizeof(char)*strlen(token));
					strcpy(YEAR, token);
				}
				else if (strncmp(token, "month\0", 6) == 0)
				{
					token = xstrtok(NULL, "");
					free(MONTH);
					MONTH = malloc(sizeof(char)*strlen(token));
					strcpy(MONTH, token);
				}
				else if (strncmp(token, "table\0", 6) == 0)
				{
					token = xstrtok(NULL, "");
					free(TABLE);
					TABLE = malloc(sizeof(char)*strlen(token));
					strcpy(TABLE, token);
				}
				else if (strncmp(token, "database\0", 9) == 0)
				{
					token = xstrtok(NULL, "");
					free(DATABASE);
					DATABASE = malloc(sizeof(char)*strlen(token));
					strcpy(DATABASE, token);
				}
				else
					printf("No such configurable variable: %s\n", token);
			}
			else
				printf("No such command: %s\n", command);
		}
		len = get_command(command, "config"); command[len-1] = '\0';
	}
	return command;
}

/*
 *	Saves current configuration to 'config.ini'
 */
void save_config(char *command)
{
	FILE *config_file;
	config_file = fopen(CONFIG_FILENAME, "r");
	if (config_file == NULL)  // File does not exist, creating new file
	{
		#ifdef DEBUG
		fprintf(stderr, "No old config file, creating new.\n");
		#endif
		config_file = fopen(CONFIG_FILENAME, "w");
		fprintf(config_file, "#configuration file for aCBudget\n");
		fprintf(config_file, "database=%s\n", DATABASE);
		fprintf(config_file, "month=%s\n", MONTH);
		fprintf(config_file, "year=%s\n", YEAR);
		fprintf(config_file, "table=%s\n", TABLE);
		fclose(config_file);
		snprintf(command, COMMAND_LEN, "Settings saved to new file '%s'\n", CONFIG_FILENAME);
		return;
	}
	else  // file exists, merging
	{
		#ifdef DEBUG
		fprintf(stderr, "Old config file found, merging...\n");
		#endif
		char *token, tmpname[strlen(CONFIG_FILENAME)];
		tmpnam(tmpname);
		//_mktemp_s(tmpname, strlen(CONFIG_FILENAME));
		FILE *new_file = fopen(tmpname, "w");
		#ifdef DEBUG
		fprintf(stderr, "Created file with temp-name: '%s'\n", tmpname);
		#endif
		int counter = 1, file_size = 0;
		do {
			if (fgets(command, COMMAND_LEN, config_file) != NULL)
			{
				counter++;
				if (command[0] != '#')  // config-variabel
				{
					token = strtok(command, "="); // '=' is config variabel delim
					if (strncmp(token, "year", 4) == 0) {  // year is stored
						fprintf(new_file, "year=%s\n", YEAR);
					} else if (strncmp(token, "month", 5) == 0) {	//	month is stored
						fprintf(new_file, "month=%s\n", MONTH);
					} else if (strncmp(token, "table", 5) == 0) { // database table is stored
						fprintf(new_file, "table=%s\n", TABLE);
					} else if (strncmp(token, "database", 8) == 0) {  // database name is stored
						fprintf(new_file, "database=%s\n", DATABASE);
					}
				}
				else  // not a config variabel, user has manually added lines
				{
					fprintf(new_file, "%s", command);
				}
			}
			else if (command == NULL)  // not supposed to happen
			{
				fprintf(stderr, "ERROR ON LINE #%d IN 'config.ini'\n", counter);
				exit(EXIT_FAILURE);
			}
		} while (!feof(config_file) && counter < 10);
		fclose(config_file);
		fclose(new_file);
		if (remove(CONFIG_FILENAME))  // failed to remove old config file
		{
			printf("Failed to remove old 'config.ini';\n%s\n", strerror(errno));
		}
		if (rename(tmpname, CONFIG_FILENAME))  // failed to rename new config file
		{
			printf("Failed to rename new 'config.ini';\n%s\n", strerror(errno));
		}
		snprintf(command, COMMAND_LEN, "Settings saved to '%s'\n", CONFIG_FILENAME);
	}
	return;
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
	 *	Manual database and SQL statement given with program execution, ie.
	 *	$ aCBudget database "statement;"
	 */
	if (argc == 3)
	{
		//return easyExecuteSQL(argc, argv);
		//sqlite3 *database;
		//char *zErrMsg = 0;
		rc = sqlite3_open(argv[1], &database);
		if ( rc ) {
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(database));
			return(1);
		}
		rc = sqlite3_exec(database, argv[2], callback, 0, &zErrMsg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		sqlite3_close(database);
	}
	else {
		/*
		 *	General program purpose
		 *	First configurates database according to a config-file (if not present, creates default)
		 *	Then accepts user input and acts accordingly.
		 */
		configurate(command, database);
		rc = sqlite3_open(DATABASE, &database);
		if ( rc ) {
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(database));
			return(1);
		}
		//	Startup userprompt for commands
		len = get_command(command, "main"); command[len-1] = '\0';
		//	Loop user for input while user don't want to quit
		while ((strncmp(command, "q\0", 2) != 0) && (strncmp(command, "quit\0", 5) != 0)) {
			#ifdef DEBUG
			fprintf(stderr, "%s (%d)\n", command, len);
			#endif
			printout = execute_command(command, database);
			//	Printout if there is anything to print
			if (strlen(printout) > 1) {
				printf("%s", printout);
			}
			//	Fetch a new command
			len = get_command(command, "main"); command[len-1] = '\0';
		}
		sqlite3_close(database);
	}
	return 0;
}
