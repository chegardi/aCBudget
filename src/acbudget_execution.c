#include "acbudget.h"


/*
 *	Provides runtime configuration-options
 */
char *config_command(char *command, sqlite3 *database)
{
	int len;
	char *variable, *value;
	len = get_command(command, "config");
	while ((strncmp(command, "e\0", 2) != 0) && (strncmp(command, "end\0", 4) != 0)) {
		if ((strncmp(command, "sv\0", 3) == 0) || (strncmp(command, "save\0", 5) == 0)) {
			save_config(command, database);
			printf("%s", command);
		}
		else if ((strncmp(command, "ld\0", 3) == 0) || (strncmp(command, "load\0", 5) == 0)) {
			if (configurate(command))	printf("Loading configuration from '%s' failed.\n", CONFIG_FILENAME);
			else	printf("Configuration loaded from '%s'.\n", CONFIG_FILENAME);
		}
		else if ((strncmp(command, "sw\0", 3) == 0) || (strncmp(command, "show\0", 5) == 0)) {
			printf("Database: %s\nTable: %s\nMonth: %s\nYear: %s\nRead: %d\n", DATABASE, TABLE, MONTH, YEAR, (*READ_COUNTER));
		}
		else if ((strncmp(command, "yr\0", 3) == 0) ||(strncmp(command, "year\0", 5) == 0)) {
			printf("Year=%s\n", YEAR);
		}
		else if ((strncmp(command, "mn\0", 3) == 0) || (strncmp(command, "month\0", 6) == 0)) {
			printf("Month=%s\n", MONTH);
		}
		else if ((strncmp(command, "tb\0", 3) == 0) || (strncmp(command, "table\0", 6) == 0)) {
			printf("Table=%s\n", TABLE);
		}
		else if ((strncmp(command, "rd\0", 3) == 0) || (strncmp(command, "read\0", 5) == 0)) {
			printf("Read=%d\n", (*READ_COUNTER));
		}
		else if ((strncmp(command, "db\0", 3) == 0) || (strncmp(command, "database\0", 9) == 0)) {
			printf("Database=%s\n", DATABASE);
		}
		else if ((strncmp(command, "bu\0", 3) == 0) || (strncmp(command, "backup\0", 7) == 0)) {
			if (revert_or_backup(database, 1)) {
				snprintf(command, COMMAND_LEN, "Failed to backup, exit program and contact developer!\n");
				return command;
			}
		}
		else if ((strncmp(command, "rv\0", 3) == 0) || (strncmp(command, "revert\0", 7) == 0)) {
			if (revert_or_backup(database, 0)) {
				snprintf(command, COMMAND_LEN, "Failed to revert: exit program and contact developer!\n");
				return command;
			}
		}
		else if ((strncmp(command, "h\0", 2) == 0) || (strncmp(command, "help\0", 5) == 0)) {
			fprintf(stdout,
			"Commands withing config:\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%-2s or %8s - %s\n%14s - %s\n",
			"h", "help", "Displays this help text",
			"sv", "save", "Saves the current configuration to file",
			"ld", "load", "Loads configuration from file",
			"sw", "show", "Shows the current configuration in program",
			"db", "database", "Displays current database(v)",
			"tb", "table", "Displays current table(v) in database",
			"yr", "year", "Displays default year(v)",
			"mn", "month", "Displays default month(v)",
			"rd", "read", "How many lines to skip if continuing reading from file",
			"bu", "backup", "Backups current database",
			"rv", "revert", "Reverts the database to backup",
			"variable=value", "sets variable(v) to new value");
		}
		else {
			len = strlen(command);
			variable = xstrtok(command, "=");
			if (len != strlen(variable)) {	//	to be sure there actually was a '=' sign
				value = xstrtok(NULL, "");
				if (strncmp(variable, "year\0", 5) == 0) {
					free(YEAR);
					YEAR = calloc(1, sizeof(char)*strlen(value));
					strcpy(YEAR, value);
				}
				else if (strncmp(variable, "month\0", 6) == 0) {
					free(MONTH);
					MONTH = calloc(1, sizeof(char)*strlen(value));
					strcpy(MONTH, value);
				}
				else if (strncmp(variable, "table\0", 6) == 0) {
					free(TABLE);
					TABLE = calloc(1, sizeof(char)*strlen(value));
					strcpy(TABLE, value);
				}
				else if (strncmp(variable, "database\0", 9) == 0) {
					int retval = sqlite3_close(database);
					if (retval == SQLITE_OK) {
//						printf("retval (%d) ?= SQLITE_OK (%d) = true\n", retval, SQLITE_OK);
						if (sqlite3_open(value, &database) != SQLITE_OK) {	//	database could not be opened
							fprintf(stderr, "Failed to open database (%s). SQLError-message: %s\nTrying to open old database...", value, sqlite3_errmsg(database));
							if (sqlite3_open(DATABASE, &database) != SQLITE_OK) {		//	old database could not be opened
								fprintf(stderr, "Failed to open old database (%s). SQLError-message: %s\nProgram shutdown to prevent damage to files.", DATABASE, sqlite3_errmsg(database));
								exit(EXIT_FAILURE);
							}
						}
						else {	//	database opened successfully
							len = strlen(value)+1;
							if (strlen(DATABASE) != strlen(value)) {	//	unequal lengths, new mallocation neccesary
								free(DATABASE);
								DATABASE = calloc(1, sizeof(char)*len);
							}
							strncpy(DATABASE, value, len);
						}
					}
					else printf("Could not change database to '%s', try again\n", value);
//					printf("retval (%d) ?= SQLITE_OK (%d) = false?\n", retval, SQLITE_OK);
				}
				else if (strncmp(variable, "read\0", 5) == 0) {
					(*READ_COUNTER) = atoi(value);
				}
				else	printf("No such configurable variable: %s\n", variable);
			}
			else	printf("No such command: %s\n", command);
		}
		len = get_command(command, "config");
	}
	return command;
}

/*
 *	Executes or give feedback to user about given command
 */
char *execute_command(char *command, sqlite3 *database)
{
	if (strncmp(command, "insert\0", 7) == 0) {
		snprintf(command, COMMAND_LEN, "%d insertions made.\n", insert(command, database));
	} else if (strncmp(command, "select\0", 6) == 0) {
		return myselect(command, database);
	} else if (strncmp(command, "read\0", 5) == 0) {
		snprintf(command, COMMAND_LEN, "%d insertions made.\n", read_file(command, database));
	} else if (strncmp(command, "update\0",  7) == 0) {
		int updated = update(command, database);
		if (updated < 0)	return 0;
		snprintf(command, COMMAND_LEN, "%d entries updated.\n", updated);
	} else if (strncmp(command, "stats\0", 6) == 0) {
		snprintf(command, COMMAND_LEN, "%d stats printed.\n", print_stats(command, database));
	} else if (strncmp(command, "config\0", 7) == 0) {
		return config_command(command, database);
	} else if ((strncmp(command, "help\0", 5) == 0) || (strncmp(command, "h\0", 2) == 0)) {
		print_help(command);
	} else {
		printf("aCBudget.%s > no such command\n", command);
		(*command) = 0;
	}
	return command;
}

/*
 *	Prompts for and stores next command
 */
int get_command(char *command, char *command_text)
{
	printf("aCBudget.%s > ", command_text);
	int command_len = strlen(fgets(command, COMMAND_LEN, stdin));
	command[command_len-1] = '\0';
	return command_len;
}

/*
 *	Need a slightly different command_argument for the update command
 */
int get_update_command(char *command, char *command_text)
{
	printf("aCBudget.update > %s: ", command_text);
	int command_len = strlen(fgets(command, COMMAND_LEN, stdin));
	command[command_len-1] = '\0';
	return command_len;
}

/*
 *	Select command
 *	Executes commands from user as long as e/end is not typed
 */
char *myselect(char *command, sqlite3 *database)
{
	int len, counter = 0;
	char *select = calloc(1, sizeof(char) * SELECT_LEN), *zErrMsg, execute;
	if (select == NULL) {
		snprintf(command, COMMAND_LEN, "Error allocating memory for operation");
	}
	else {
		printf("===WARNING===\nAny statements written WILL be executed! Be careful NOT to execute unintended statements on database.\n===WARNING===\n");
		len = get_command(select, "select");
		while ((strncmp(select, "e\0", 2) != 0) && (strncmp(select, "end\0", 4) != 0)) {
			if (strncmp(select, "select ", 7) != 0) {
				printf("Really execute '%s', ?: ", select);
				scanf("%c", &execute);
				clean_stdin();
			}	else execute = 'y';
			if (execute == 'y') {
				#ifdef DEBUG
				fprintf(stderr, "sql statement: '%s'\n", select);
				#endif
				if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				} else {
					counter++;
				}
			}
			len = get_command(select, "select");
		}
		snprintf(command, COMMAND_LEN, "%d commands excuted\n", counter);
		free(select);
	}
	return command;
}

/*
 *
 */
int print_stats(char *command, sqlite3 *database)
{
	int	stats_cnt = 0,
			len = -1,
			execution = -1,
			max_commands = print_stats_help();
	char	select[SELECT_LEN],
			*zErrMsg;
	do {
		/*	prompt user for command	*/
		len = get_command(command, "stats");
		/*	print stat selected or exit	*/
		execution = atoi(command);
		if (strncmp(command, "h\0", 2) == 0)	print_stats_help();
		else if (strncmp(command, "e\0", 2) == 0)	break;
		else if (execution > 0 && execution <= max_commands) {
			if (execution == 1) {
				snprintf(select, SELECT_LEN, "select type, sum(amount) as Balance from %s group by type order by type", TABLE);
				if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
					fprintf(stderr, "SQL error %d - %s\n", sqlite3_errcode(database), sqlite3_errmsg(database));
					sqlite3_free(zErrMsg);
				}
				snprintf(select, SELECT_LEN, "select sum(amount) as Total from %s", TABLE);
				if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
			}
			else if (execution == 2) {
				printf("Select month 1-12: ");
				len = strlen(fgets(command, COMMAND_LEN, stdin)); command[len-1] = '\0';
				int month = atoi(command);
				if (0 < month && month <= 12) {
					snprintf(select, SELECT_LEN, "select type, sum(amount) as Forbruk from %s where date > '%4s-%02d-00' and date < '%4s-%02d-32' group by type order by type", TABLE, YEAR, month, YEAR, month);
					if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
						fprintf(stderr, "SQL error: %s\n", zErrMsg);
						sqlite3_free(zErrMsg);
					}
					snprintf(select, SELECT_LEN, "select sum(amount) as Total from %s where date > '%4s-%02d-00' and date < '%4s-%02d-32'", TABLE, YEAR, month, YEAR, month);
					if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
						fprintf(stderr, "SQL error: %s\n", zErrMsg);
						sqlite3_free(zErrMsg);
					}
				}
				else	printf("Illegal value; '%s', entered. Must be a number between 1-12\n", command);
			}
			else if (execution == 3) {
				printf("Select month 1-12: ");
				len = strlen(fgets(command, COMMAND_LEN, stdin)); command[len-1] = '\0';
				int month = atoi(command);
				if (0 < month && month <= 12) {
					snprintf(select, SELECT_LEN, "select * from %s where date < '%4s-%02d-32' and date > '%4s-%02d-00' order by date", TABLE, YEAR, month, YEAR, month);
					#ifdef DEBUG
					fprintf(stderr, "command: %s\nmonth: %02d\nselect: %s\n--------\n", command, month, select);
					#endif
					if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
						fprintf(stderr, "SQL error: %s\n", zErrMsg);
						sqlite3_free(zErrMsg);
					}
					snprintf(select, SELECT_LEN, "select sum(amount) as Total from %s where date < '%4s-%02d-32' and date > '%4s-%02d-00'", TABLE, YEAR, month, YEAR, month);
					if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
						fprintf(stderr, "SQL error: %s\n", zErrMsg);
						sqlite3_free(zErrMsg);
					}
				}
				else	printf("Illegal value; '%s', entered. Must be a number between 1-12\n", command);
			}
			else if (execution == 4) {
				int month_digit = 0;
				char *months = "January\0February\0March\0April\0May\0June\0July\0August\0September\0October\0November\0December\0";
				while (month_digit++ < 12) {
					snprintf(select, SELECT_LEN, "select sum(amount) as %s from %s where date < '%4s-%02d-32' and date > '%4s-%02d-00'", months, TABLE, YEAR, month_digit, YEAR, month_digit);
					#ifdef DEBUG
					fprintf(stderr, "%02d month = '%s'\n", month_digit, months);
					fprintf(stderr, "select : '%s'\n", select);
					#endif
					if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK ) {
						fprintf(stderr, "SQL error: %s\n", zErrMsg);
						sqlite3_free(zErrMsg);
					}
					while ((*months) != 0)	months++;
					months++;
				}
				snprintf(select, SELECT_LEN, "select sum(amount) as Total from %s", TABLE);
				if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK ) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
			}
			stats_cnt ++;
		}
		else	printf("No statistics on '%s'\n", command);
	}	while (strncmp(command, "e\0", 2) != 0);
	return stats_cnt;
}

/*
 *	Method to print help about core commands
 */
void print_help(char *command)
{
	printf("aCBudget.help >\n");
	printf("%-6s - %s\n", "select", "write commands directly to database");
	printf("%-6s - %s\n", "insert", "for easy insertions to database");
	printf("%-6s - %s\n", "read", "read insertions from file");
	printf("%-6s - %s\n", "update", "for easy update (and dividing) of existing entries");
	printf("%-6s - %s\n", "stats", "provides a menu to print out some predefined stats");
	printf("%-6s - %s\n", "config", "a menu to configurate database variables");
	printf("%-6s - %s\n", "help", "this menu");
	(*command) = '\0';	//	reset command pointer
}

/*
 *	Tells program a file is to be read
 * Prompts user for location of file.
 *	Then prompts user for filetype, implemented
 *	for files downloaded from DNB Norway(1) and
 *	Sparebanken Sør(2)
 */
int read_file(char *command, sqlite3 *database)
{
	int len, counter = 0;
	FILE *fp;
	char filename[50], type;
	filename[0] = '\0';
	while (1) {
		len = get_command(filename, "filename");
		if ((strncmp(filename, "e\0", 2) == 0) || (strncmp(filename, "end\0", 4) == 0))
			break;
		fp = fopen(filename, "r");
		if (fp == 0) {
			printf("'%s' does not exist\n", filename);
			continue;
		}
		printf("DNB (1), Sparebanken Sør (2) file or continue last(3) ? ");
		scanf("%c", &type);
		clean_stdin();
		if (type=='1') {
			(*READ_COUNTER) = 0;
			counter += read_DNB(fp, database);
		}
		else if (type=='2') {
			(*READ_COUNTER) = 0;
			counter += read_SBS(fp, database);
		}
		else if (type=='3') {
			printf("Last DNB (1) or Sparebanken Sør (2) file ? ");
			scanf("%c", &type);
			clean_stdin();
			if (type=='1')
				counter += read_DNB(fp, database);
			else if (type=='2')
				counter += read_SBS(fp, database);
			else
				printf("%c was not a valid file option.\n", type);
		}
		else
			printf("%c was not a valid option.\n", type);
		fclose(fp);
	}
	return counter;
}

/*
 *	Main method to read DNB files.
 *	Reads file from start or until user ends at given checkpoints.
 *
 */
int read_DNB(FILE *fp, sqlite3 *database)
{
	#if DEBUG
	fprintf(stderr, "read DNB\n");
	#endif
	int counter = 0, error, lines = 0;
	char correct,
			input[INPUT_LEN],				//	input lines
			insert_into[INSERT_LEN],	//	insert into statement
			*token,									//	tokens from input
			*zErrMsg;							//	used in errors from SQLite
	INSERT	insert;							//	struct with all variables of an insert as members
	while (fgets(input, INPUT_LEN, fp) != NULL) {
		lines++;
		if (strlen(input) <= 1)	continue;	//	empty line
		else if ((*READ_COUNTER) > 0)	//	skip line
		{
			(*READ_COUNTER)--;
			continue;
		}
		/*
		 *	"Dato";"Forklaring";"Rentedato";"Uttak";"Innskudd"
		 *	"14.12.2014";"Morsom sparing kort avrunding Reservert transaksjon";"";"3,00";""
		 */
		#if DEBUG
		fprintf(stderr, "input: '%s'", input);
		#endif
		/*
		 *	Checks if number is on "Uttak" or "Innskudd".
		 *	If number is on "Uttak", token will end with ""\0 (end of input)
		 * But if number is on "Innskudd", token will include end number,
		 *	leaving error > 3, and thus gets converted with (-) at beginning
		 *	to get budgeted correctly when summing in database.
		 */
		token = strstr(input, "\"\"");
		if (token)
			error = strlen(token);
		#if DEBUG
		fprintf(stderr, "split token: %s\n", token);
		#endif
		// Date of purchase in "Dato" field of input.
		token = strtok(input, "\";");
		#if DEBUG
		fprintf(stderr, "date: %s\n", token);
		#endif
		copy_date(insert.date, token);
		/*
		 *	Copies "Forklaring" into comment,
		 *	for further analysis by user
		 */
		token = strtok(NULL, "\";");
		#if DEBUG
		fprintf(stderr, "comment: %s\n", token);
		#endif
		strncpy(insert.comment, token, COMMENT_LEN);
		/*
		 *	Unimportant for my personal budget,
		 *	Date for interest rate
		 */
		token = strtok(NULL, "\";");
		#if DEBUG
		fprintf(stderr, "Interest date: %s\n", token);
		#endif
		//	Amount token
		token = strtok(NULL, "\";");
		#if DEBUG
		fprintf(stderr, "amount: %s\n", token);
		#endif
		if (error > 4) {	//	Amount is deposited, decreasing money spent
			insert.amount[0] = '-';
			copy_number(1, insert.amount, token);
		} else	//	Amount is withdrawn, increasing money spent
			copy_number(0, insert.amount,token);
		/*
		 *	Prints out rows with equal date and/or amount
		 *	To check for double-entries.
		 *	Will remove equal amount check when I've
		 *	finnished adding my personal information for year 2014
		 */
		printf("---Rows on same date:\n");
		snprintf(insert_into, INSERT_LEN, "select * from %s where date like '%s';", TABLE, insert.date);
		//	Execute sql select statement
		if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		#if DEBUG
		fprintf(stderr, "%s\n", insert_into);
		#endif
		/*
		 *	Shows user information about date, comment and amount
		 *	And prompts for input about what to do with current insert
		 *	Add? y_es continues
		 *	Add? n_o skips insertion, and continue read file
		 *	Add? q_uit exits reading from file and ends current loop
		 *
		 *	I recommend to delete lines added manually until I implement
		 *	automatic deletion from file after insertion
		 */
		printf("---END-ROWS---\n-'%s', '%s', %s\nAdd? (y/n/q): ", insert.date, insert.comment, insert.amount);
		scanf("%c", &correct);
		clean_stdin();
		if (correct == 'q') {
			lines--;
			break;
		}
		else if (correct == 'y') {
			// Add comment
			printf("Comment: ");
			fgets(insert.comment, COMMENT_LEN, stdin);
			insert.comment[strlen(insert.comment)-1] = '\0';
			//	Generate unique ID for insertion
			//	Prompts user for type of budget-line
			printf("Type: ");
			fgets(insert.type, 16, stdin);
			insert.type[strlen(insert.type)-1] = '\0'; // end type with \0 instead of \n
			generate_id(insert.id);
			//	Generate SQL statement for insertion based on information given
			snprintf(insert_into, INSERT_LEN, "insert into %s values('%s', '%s', '%s', %s, '%s');", TABLE, insert.date, insert.comment, insert.type, insert.amount, insert.id);
			#if DEBUG
			fprintf(stderr, "%s\n", insert_into);
			#endif
			//	Execute SQL insertion statement
			if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK ) {
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
				return;
			} else {
				#if DEBUG
				printf("%s\n", insert_into);
				#endif
			}
			counter++;
		}
	}
	(*READ_COUNTER) = lines;
	return counter;	// Insertions made
}

/*
 *	Main method for reading files from Sparebanken Sør
 *	Almost same functionality as DNB method, with
 *	some tweaks to read correctly from a different filetype (.csv)
 */
int read_SBS(FILE *fp, sqlite3 *database)
{
	#if DEBUG
	fprintf(stderr, "read SBS\n");
	#endif
	int counter = 0, error, lines = 0;
	char	correct,
			input[INPUT_LEN],					//	input
			insert_into[INSERT_LEN],		//	insert into statemtn
			dateday[3],								//	dd daydate
			datemonth[3],							//	mm monthdate
			*token,										//	tokens from input
			*datetoken,								//	used to temporarily store date elements from token
			*zErrMsg;								//	used in errors from SQLite
	INSERT insert;
	while (fgets(input, INPUT_LEN, fp) != NULL)	{	//	while file has input
		lines++;
		if (strlen(input) <= 1)	continue;	//	empty line
		else if ((*READ_COUNTER) > 0)	{	//	skip line
			(*READ_COUNTER)--;
			continue;
		}
		/*
		 *	This is input format of SBS files. Separations
		 *	is tabulated, and not spaced.
		 *	Dato	Forklaring	Ut av konto	Inn på konto
		 * 30.01.2014	30.01 INFORMATIKKKAFE GAUSTADALLEE OSLO	28,00
		 */
		//	First token is interest date, stored to use if date is not given in comment
		token = xstrtok(input, "	");
		strncpy(insert.date, token, DATE_LEN);
		#if DEBUG
		fprintf(stderr, "Date: %s\n", token);
		#endif
		//	Second token is explanation (comment)
		token = xstrtok(NULL, "	");
		#if DEBUG
		fprintf(stderr, "Comment: %s\n", token);
		#endif
		strncpy(insert.comment, token, COMMENT_LEN);
		/*
		 *	Explanation may contain date in beginning
		 *	which will be date of purchase (my budget date)
		 *	first datetoken is then the DAY
		 */
		datetoken = strtok(token, ".");
		if (strlen(datetoken) < 4) {
			//	Date was found, storing day
			strncpy(dateday, datetoken, 3);
			datetoken = strtok(NULL, " ");
			//	Storing month
			strncpy(datemonth, datetoken, 3);
			/*
			 *	Replacing old date with correct format, importing YEAR from header
			 */
			snprintf(insert.date, DATE_LEN, "%s-%s-%s", YEAR, datemonth, dateday);
		} else {
			//	No date was found in explanation, prompts user for input
			datetoken = calloc(1, sizeof(char)*DATE_LEN);
			strncpy(datetoken, insert.date, DATE_LEN);
			copy_date(insert.date, datetoken);
			free(datetoken);
		}
		//	Last token is amount
		token = xstrtok(NULL, "	");
		if (strlen(token) > 1) {
			//	Number was a withdrewal, increasing budget spent
			copy_number(0, insert.amount, token);
		} else {
			//	Number was a deposit, decreasing budget spent
			token = xstrtok(NULL, "	");
			insert.amount[0] = '-';
			copy_number(1, insert.amount, token);
		}
		#if DEBUG
		fprintf(stderr, "amount: %s\n", token);
		#endif
		//	All information gathered, check for equal date in database
		printf("---Rows on same date:\n");
		snprintf(insert_into, INSERT_LEN, "select * from %s where date like '%s';", TABLE, insert.date);
		if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		#if DEBUG
		fprintf(stderr, "%s\n", insert_into);
		#endif
		//	Prompts user if information is to be added.
		printf("---END-ROWS---\n-'%s', '%s', %s\nAdd? (y/n/q): ", insert.date, insert.comment, insert.amount);
		scanf("%c", &correct);
		clean_stdin();
		if (correct == 'y') {
			//	Replace comment
			printf("New comment: ");
			fgets(insert.comment, COMMENT_LEN, stdin);
			insert.comment[strlen(insert.comment)-1] = '\0';
			//	Add a type
			printf("Type: ");
			fgets(insert.type, TYPE_LEN, stdin);
			insert.type[strlen(insert.type)-1] = '\0';
			//	Generate unique ID
			generate_id(insert.id);
			//	Store SQL statement for execution
			snprintf(insert_into, INSERT_LEN, "insert into %s values('%s', '%s', '%s', %s, '%s');", TABLE, insert.date, insert.comment, insert.type, insert.amount, insert.id);
			//	Execute SQL statement
			#ifdef DEBUG
			fprintf(stderr, "inserted '%s'\n", insert_into);
			#endif
			if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK ) {
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
				return;
			}
			counter++;
		} else if (correct == 'n') {
			#if DEBUG
			fprintf(stderr, "Values NOT added.\n");
			#endif
		} else if (correct == 'q') {
			lines--;
			break;
		}
	}
	(*READ_COUNTER) = lines;
	return counter;	//	Insertions made
}

/*
 *	Lets user update amount column of existing rows, and the possibility of inserting a new row
 */
int update(char *command, sqlite3 *database)
{
	int updated = 0, sql_len, len;
	char	commandhelp[COMMAND_LEN],	//	to store usage-help
			select[SELECT_LEN] ,	//	used by sql-queries
			comment[COMMENT_LEN],	//	to store comments
			type[TYPE_LEN],	//	to store types
			amount[AMOUNT_LEN],	//	to store amounts
			*day, *zErrMsg, correct;	//	helpful
	//	allocating space for id from rownumbers
	UNIQUE_ID = calloc(1, sizeof(char) * ID_LEN);
	P_COUNTER = calloc(1, sizeof(int));
	do {
		snprintf(commandhelp, COMMAND_LEN, "day in month (%s)", MONTH);
		len = get_update_command(command, commandhelp);
		if ((strncmp(command, "e\0", 2) == 0) || (strncmp(command, "end\0", 4) == 0))	break;
		else if (command[1] == '.' || command[2] == '.') {
		    int i = 1;
		    while (command[i] != '.') i++;
		    command[i] = 0;
		    int month = strtol(command+i+1, &command+i+1, 10);
		    if (month > 0 && month < 13) {
			(*MONTH)=0;
			snprintf(MONTH, 3, "%02d", month);
		    }
		    else {
			printf("Month outside range 1-12: %d\n", month);
			continue;
		    }
		}
		else if (strncmp(command, "month=", 6) == 0) {
		    int i = 0;
		    len = strtol(command+6, &command, 10);
		    if (len > 0 && len < 13) {
			(*MONTH)=0;
			snprintf(MONTH, 3, "%02d", len);
		    }
		    else
			printf("Month outside range 0-12: %d\n", len);
		    continue;
		}
		//	find entries based on day of month
		
		snprintf(select, SELECT_LEN, "select comment, amount, type from %s where date = '%04d-%02d-%02d'", TABLE, atoi(YEAR), atoi(MONTH), atoi(command));
		#if DEBUG
		fprintf(stdout, "Running select on %s: '%s'\n", DATABASE, select);
		#endif
		(*P_COUNTER) = 1;	//	reset row count
		if ( sqlite3_exec(database, select, numbered_callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			return -1;
		}
		if (*P_COUNTER > 1) {
			//	give user ability to divide/update entries
			day = calloc(1, sizeof(char) * 3);
			snprintf(day, 3, "%02d", atoi(command));
			len = get_update_command(command, "number to update"); command[len-1] = '\0';
			#if DEBUG
			fprintf(stderr, "day: '%s', command: '%s'\n", day, command);
			#endif
			len = atoi(command);
			if ((strncmp(command, "n\0", 2)!=0) && ((( len < 1) ) || (len >= (*P_COUNTER))))
				printf("Invalid input: '%s', either pick number in range %d-%d or 'n' for none\n", command, 1, ((*P_COUNTER)-1));
			else if (strncmp(command, "n\0", 2) != 0) {
				(*P_COUNTER) = atoi(command);
				//	row selected for update
				#if DEBUG
				fprintf(stderr, "Finding rownumber %d (P_COUNTER: %d)\n", len, (*P_COUNTER));
				#endif
				len = snprintf(select, SELECT_LEN, "select id from %s where date = '%04d-%02d-%02d'", TABLE, atoi(YEAR), atoi(MONTH), atoi(day));
				//	prepare statement to find correct row for update
				sqlite3_stmt **statement = calloc(1, 1* sizeof( sqlite3_stmt *));
				if (sqlite3_prepare_v2(database, select, len, statement, NULL) != SQLITE_OK) {
					fprintf(stderr, "SQL prepare error from '%s': %s\nContact system creator\n", select, zErrMsg);
					sqlite3_finalize(*statement);
					sqlite3_free(zErrMsg);
					continue;
				}
				while ((*P_COUNTER) > 0) {	//	while rownumber not met
					#if DEBUG
					fprintf(stderr, " %d", (*P_COUNTER));
					#endif
					int sql_result = 0;
					do {
						sql_result = sqlite3_step(*statement);	//	fetch next row
					} while (sql_result == SQLITE_BUSY);
					if (sql_result == SQLITE_ROW)	{	//	a row was found -> reduce P_COUNTER
						(*P_COUNTER)--;
					}
					else {	//	unexpected behaviour
						if (sql_result == SQLITE_DONE) {
							printf("\nNumber chosen was higher than number of rows returned.\n");
							break;
						}
						else if (sql_result == SQLITE_MISUSE) {
							fprintf(stderr, "\nSQL misuse: Contact system creator\n");
							break;
						}
						else if (sql_result == SQLITE_ERROR) {
							fprintf(stderr, "\nSQL step-error: Contact system creator\n");
							break;
						}
						continue;
					}
				}
				#if DEBUG
				fprintf(stderr, "\nCopying unique id into UNIQUE_ID\n");
				#endif
				strncpy(UNIQUE_ID, sqlite3_column_text((*statement), 0), ID_LEN);
				/*
				 *	Prompts user for input on all three values COMMENT, TYPE and AMOUNT.
				 *	Date is currently not available for update.
				 *	It is not neccesary to update all three, or anything at all, the program only updates fields with input.
				 */
				sql_len = 0;	//	the total length of sql-command initialization
				sql_len += snprintf(select, SELECT_LEN, "update %s set", TABLE);
				(*P_COUNTER) = sql_len;
				len = get_update_command(comment, "comment"); comment[len-1] = '\0';
				if (len > 1) {	//	comment is to be updated
					sql_len += snprintf(select+sql_len, SELECT_LEN-sql_len, " comment = '%s'", comment);
				}
				len = get_update_command(type, "type"); type[len-1] = '\0';
				if (len > 1) {	//	type is to be updated
					if (sql_len > (*P_COUNTER)) {	//	need to add comma after previous variable
						*(select+sql_len++) = ',';
						*(select+sql_len) = '\0';
					}
					sql_len += snprintf(select+sql_len, SELECT_LEN-sql_len, " type = '%s'", type);
				}
				len = get_update_command(amount, "amount"); amount[len-1] = '\0';
				if (len > 1) {	//	amount is to be updated
					if (sql_len > (*P_COUNTER)) {	//	need to add comma after previous variable
						*(select+sql_len++) = ',';
						*(select+sql_len) = '\0';
					}
					sql_len += snprintf(select+sql_len, SELECT_LEN-sql_len, " amount = %s", amount);
					#if DEBUG
					fprintf(stderr,"select: %s\n", select);
					#endif
				}
				#if DEBUG
				fprintf(stderr, "comment, type, amount = '%s', '%s', '%s'\n%s\n", comment, type, amount, select);
				#endif
				//snprintf(select, SELECT_LEN, "update %s set comment = '%s', type = '%s', amount = %s where id = '%s'", TABLE, comment, type, amount, UNIQUE_ID);
				if (sql_len > (*P_COUNTER)) {	//	at least one variable to update
					(*P_COUNTER) = sql_len;
					len = strlen(" where id = 'abc12'");	//	length of rest
					sql_len += snprintf(select+sql_len, SELECT_LEN-sql_len, " where id = '%s';", UNIQUE_ID);
					if (((*P_COUNTER)+len != sql_len) && (sqlite3_exec(database, select, NULL, 0, &zErrMsg) != SQLITE_OK)) {
						fprintf(stderr, "%d =? %d || SQL error %s\n", (*P_COUNTER)+len, sql_len, zErrMsg);
						sqlite3_free(zErrMsg);
						return -1;
					}
					updated++;	//	row updated
					#if DEBUG
					fprintf(stderr, "Row with id='%s' updated\n", UNIQUE_ID);
					#endif
				}
				//	check if user wants to add another row
				printf("Want to add another row with new type and same comment on same date?: ");
				scanf("%c", &correct);
				clean_stdin();
				if (correct == 'y') {	//	add another row with new comment, type and amount on same date
					get_update_command(comment, "comment");
					get_update_command(type, "type");
					get_update_command(amount, "amount");
					generate_id(UNIQUE_ID);																					//	id
					//	insert into TABLE values (DATE, COMMENT, TYPE, AMOUNT, ID);
					snprintf(select, SELECT_LEN, "insert into %s values('%04d-%02d-%02d', '%s', '%s', %s, '%s');",
						TABLE, atoi(YEAR), atoi(MONTH), atoi(day), comment, type, amount, UNIQUE_ID);
					if (sqlite3_exec(database, select, NULL, 0, &zErrMsg) != SQLITE_OK) {
						fprintf(stderr, "SQL error %s\n", zErrMsg);
						sqlite3_free(zErrMsg);
						free(day);
						break;
					}
				}
				//	free update specific malloc
				sqlite3_finalize(*statement);
				free(statement);
			}
			//	free day-storage
			free(day);
		}	else printf("No entries in given day (%s) of month (%s)\n", command, MONTH);
	}	while ((strncmp(command, "e\0", 2) != 0) && (strncmp(command, "end\0", 4) != 0));
	free(P_COUNTER);
	free(UNIQUE_ID);
	return updated;
}
