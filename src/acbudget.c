#include"acbudget.h"

/*
 *	Used to print out calls from database
 */
static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (i=0; i<argc; i++) { // number of columns
		printf("%s = %s | ", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

/*
 *	Prompts for and stores next command	
 */
int get_command(char *command, char *command_text)
{
	fflush(stdin);
	printf("aCBudget.%s > ", command_text);
	return strlen(fgets(command, COMMAND_LEN, stdin));
}

/*
 *	Used to generate unique ID-token for use in database
 */
void generate_id(char *id)
{
	char letters[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;
	time_t t;
	srand((unsigned) time(&t)); // Seed
	for (i=0;i<ID_LEN-1; i++) {
		id[i] = letters[rand()%strlen(letters)];
	}
	id[ID_LEN-1] = '\0';
}

/*
 *	insert command
 *	Used for standardized insertions to database
 */
int insert(char *command, sqlite3 *database)
{
	int len, i, counter = 0;	
	char *zErrMsg = 0, *insert = malloc(sizeof(char)*INSERT_LEN), *insert_into = insert, *token, id[ID_LEN];
	char usage[] = "usage (e or end to quit)\n"\
			 "***\naCBudget.insert > YYYY-MM-DD,comment,type,amount\n"\
			 "aCBudget.insert > 2014-12-11,Rema 1000,mat,123\n***\n";
	printf("%s", usage);
	len = get_command(command, "insert"); command[len-1] = '\0';
	while ((strncmp(command, "e\0", 2) != 0) && (strncmp(command, "end\0", 4) != 0)) {	   
		if ((strncmp(command, "h\0", 2) == 0) || (strncmp(command, "help\0", 5) == 0))
		{	// help
			printf("%s", usage);
			*command = '\0';
		} else {
			token = strtok(command, ",");
			snprintf(insert, INSERT_LEN, "\0");
			insert += snprintf(insert, INSERT_LEN, "insert into %s values(", TABLE);
			//	read and store date, comment and type
			for (i =0; i<3; i++) {
				if (token != NULL) {
					#ifdef DEBUG
					fprintf(stderr, "%s\n", token);
					#endif
					insert += snprintf(insert, INSERT_LEN, "'%s', ", token);
					token = strtok(NULL, ",");
				} else {
					break;
				}
			}
			if (token == NULL) {
				printf("Wrong input: %s\n%s", command, usage);
			} else {
				//	store amount
				#ifdef DEBUG
				fprintf(stderr, "%s\n", token);
				#endif
				insert += snprintf(insert, INSERT_LEN, "%s, ", token);
				//	Generate and store unique ID
				generate_id(id);
				insert += snprintf(insert, INSERT_LEN, "'%s');", id);
				//	execute SQL command
				if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK ) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
					return;
				} else {
					#ifdef DEBUG
					fprintf(stderr, "%s\n", insert_into);
					#endif
				}
				counter++;
			}
		}
		len = get_command(command, "insert"); command[len-1] = '\0';
	}
	free(insert);
	return counter;
}

/*
 *	Select command
 *	Executes commands from user as long as e/end is not typed
 */
char *myselect(char *command, sqlite3 *database)
{
	int len, counter = 0;
	char select[250], *zErrMsg;
	select[0] = '\0';
	len = get_command(select, "select"); select[len-1] = '\0';
	while ((strncmp(select, "e\0", 2) != 0) && (strncmp(select, "end\0", 4) != 0)) {
		if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		} else {
			counter++;
		}
		len = get_command(select, "select"); select[len-1] = '\0';
	}
	snprintf(command, COMMAND_LEN, "%d commands excuted\n", counter);
	return command;
}

/*
 *	Tells program a file is to be read
 * Prompts user for location of file.
 *	Then prompts user for filetype, implemented
 *	for files downloaded from DNB Norway(1) and
 *	Sparebanken Sør(2)
 */
int readfile(char *command, sqlite3 *database)
{
	int len, counter = 0;
	FILE *fp;
	char filename[50], type;
	filename[0] = '\0';
	len = get_command(filename, "filename"); filename[len-1] = '\0';
	while ((strncmp(filename, "e\0", 2) != 0) && (strncmp(filename, "end\0", 4) != 0)) {
		fp = fopen(filename, "r");
		printf("(1) DNB file or (2) Sparebanken Sør file? ");
		type = fgetc(stdin);
		fflush(stdin);
		if (type=='1')
			counter += readDNB(fp, database);
		else if (type=='2')
			counter += readSBS(fp, database);
		else
			printf("%c was not a valid file option.\n", type);
		fclose(fp);
		len = get_command(filename, "filename"); filename[len-1] = '\0';
	}
	return counter;
}

/*
 *	Numbers in both files are given with
 *	"," where it should be "." in database.
 *	This method converts number to remove
 *	 "." from original and replace "," with "." in
 *	code.
 */
void copynumber(int error, char* to, char* from)
{
	int i = 0;
	while (from[i] != '\0') {
		if (from[i] == '.')
			error--;
		else if (from[i] == ',')
			to[i+error] = '.';
		else
			to[i+error] = from[i];
		i++;
	}
	to[i+error] = '\0';
	#ifdef DEBUG
	fprintf(stderr, "token.amount=%s vs %s=sql.amount\n", from,to);
	#endif
}

/*
 *	Date are reversed and wrong in files from DNB
 *	This method stores date correctly in *date pointer
 */
void copydate(char *date, char *token)
{
	#ifdef DEBUG
        fprintf(stderr, "token.date=%s\n", token);
	#endif
	int i;
	for (i = 0; i< 4; i++)
			date[i] = (int) token[6+i];
	date[i++] = '-';
	date[i++] = (int) token[3];
	date[i++] = (int) token[4];
	date[i++] = '-';
	date[i++] = (int) token[0];
	date[i++] = (int) token[1];
	date[i++] = '\0';
	#ifdef DEBUG
	fprintf(stderr, "regnskap.date=%s\n", date);
	#endif
}

/*
 *	Main method to read DNB files.
 *	Reads file from start or until user ends at given checkpoints.
 *	
 */
int readDNB(FILE *fp, sqlite3 *database)
{
	#ifdef DEBUG
	fprintf(stderr, "read DNB\n");
	#endif
	int counter = 0, error;
	char correct, input[INPUT_LEN], date[DATE_LEN], comment[COMMENT_LEN], type[TYPE_LEN], amount[AMOUNT_LEN], insert_into[INSERT_LEN], *token, *zErrMsg = 0;
	date[0] = '\0'; comment[0] = '\0'; type[0] = '\0';
	while (fgets(input, INPUT_LEN, fp) != NULL) {
		/*
		 *	"Dato";"Forklaring";"Rentedato";"Uttak";"Innskudd"
		 *	"14.12.2014";"Morsom sparing kort avrunding Reservert transaksjon";"";"3,00";""
		 */
		fflush(stdin);
		#ifdef DEBUG
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
		error = strlen(token);
		#ifdef DEBUG
		fprintf(stderr, "split token: %s\n", token);
		#endif
		// Date of purchase in "Dato" field of input.
		token = strtok(input, "\";");
		#ifdef DEBUG
		fprintf(stderr, "date: %s\n", token);
		#endif
		copydate(date, token);
		/*
		 *	Copies "Forklaring" into comment,
		 *	for further analysis by user
		 */
		token = strtok(NULL, "\";");
		#ifdef DEBUG
		fprintf(stderr, "comment: %s\n", token);
		#endif
		strncpy(comment, token, 35);
		/*
		 *	Unimportant for my personal budget,
		 *	Date for interest rate
		 */
		token = strtok(NULL, "\";");
		#ifdef DEBUG
		fprintf(stderr, "Interest date: %s\n", token);
		#endif
		//	Amount token
		token = strtok(NULL, "\";");
		#ifdef DEBUG
		fprintf(stderr, "amount: %s\n", token);
		#endif
		if (error > 3) {
			//	Amount is deposited, decreasing money spent
			amount[0] = '-';
			copynumber(1, amount, token);
		} else
			//	Amount is withdrawn, increasing money spent
			copynumber(0, amount,token);
		/*
		 *	Prints out rows with equal date and/or amount
		 *	To check for double-entries.
		 *	Will remove equal amount check when I've
		 *	finnished adding my personal information for year 2014
		 */
		printf("@Rows with equal date and/or amount:\n");
		snprintf(insert_into, INSERT_LEN, "select * from %s where date like '%s';", TABLE, date);
		//	Execute sql select statement
		if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		#ifdef DEBUG
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
		printf("-'%s', '%s', %s\nAdd? (y/n/q): ", date, comment, amount);
		correct = fgetc(stdin);
		if (correct == 'q')
			break;
		else if (correct == 'y') {
			fflush(stdin);
			// Prompts user for type of budget-line
			printf("Type?: ");
			fgets(type, 16, stdin);
			type[strlen(type)-1] = '\0'; // end type with \0 instead of \n
			/* 
			 *	Gives user information about how insertion will look if y_es is answered
			 *	If user enteres e_dit, the comment section will be asked for editing
			 *	n_o will just skip insertion, and continue with rest of file
			 */
			printf("---'%s', '%s', '%s', %s\nCorrect (y/n/e)? ", date, comment, type, amount);
			fflush(stdin);
			correct = fgetc(stdin);
			if (correct == 'y' || correct == 'e') {
				fflush(stdin);
				if (correct = 'e') {
					// User wanted to replace comment
					printf("New comment: ");
					fgets(comment,36,stdin);
					comment[strlen(comment)-1] = '\0';
				}
				//	Generate unique ID for insertion
				char id[ID_LEN];
				generate_id(id);
				//	Generate SQL statement for insertion based on information given
				snprintf(insert_into, INSERT_LEN, "insert into %s values('%s', '%s', '%s', %s, '%s');", TABLE, date, comment, type, amount, id);
				#ifdef DEBUG
				fprintf(stderr, "%s\n", insert_into);
				#endif
				//	Execute SQL insertion statement
				if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK ) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
					return;
				} else {
					#ifdef DEBUG
					printf("%s\n", insert_into);
					#endif
				}
				counter++;
			} else if (correct == 'n') {
				#ifdef DEBUG
				fprintf(stderr, "Values NOT added.\n");
				#endif
			}
		}
	}
	return counter;	// Insertions made
}

/*
 *	Main method for reading files from Sparebanken Sør
 *	Almost same functionality as DNB method, with 
 *	some tweaks to read correctly from a different filetype (.csv)
 */
int readSBS(FILE *fp, sqlite3 *database)
{
	#ifdef DEBUG
	fprintf(stderr, "read SBS\n");
	#endif
	int counter = 0, error;
	char input[INPUT_LEN], date[DATE_LEN], insert_into[INSERT_LEN], dateday[3], datemonth[3], comment[COMMENT_LEN], type[TYPE_LEN], amount[AMOUNT_LEN], correct, *token = 0, *datetoken = 0, *zErrMsg = 0;
	while (fgets(input, INPUT_LEN, fp) != NULL)
	{
		/*
		 *	This is input format of SBS files. Separations
		 *	is tabulated, and not spaced.
		 *	Dato	Forklaring	Ut av konto	Inn på konto
		 * 30.01.2014	30.01 INFORMATIKKKAFE GAUSTADALLEE OSLO	28,00	
		 */
		fflush(stdin);
		//	First token is interest date, stored to use if date is not given in comment
		token = xstrtok(input, "	");
		strncpy(date, token, 11);
		#ifdef DEBUG
		fprintf(stderr, "Date: %s\n", token);
		#endif
		//	Second token is explanation (comment)
		token = xstrtok(NULL, "	");
		#ifdef DEBUG
		fprintf(stderr, "Comment: %s\n", token);
		#endif
		strncpy(comment, token, 36);
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
			snprintf(date, 11, "%s-%s-%s", YEAR, datemonth, dateday);
		} else {
			//	No date was found in explanation, prompts user for input
			printf("Date: %s | Comment: %s \nNew date YYYY-MM-DD: ", date, token);
			fgets(date, 11, stdin);
			fflush(stdin);
		}
		//	Last token is amount
		token = xstrtok(NULL, "	");
		if (strlen(token) > 1)
		{
			//	Number was a withdrewal, increasing budget spent
			copynumber(0, amount, token);
		} else {
			//	Number was a deposit, decreasing budget spent
			token = xstrtok(NULL, "	");
			amount[0] = '-';
			copynumber(1, amount, token);
		}
		#ifdef DEBUG
		fprintf(stderr, "amount: %s\n", token);
		#endif
		//	All information gathered, check for equal date in database
		snprintf(insert_into, INSERT_LEN, "select * from %s where date like '%s';", TABLE, date);
		if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		#ifdef DEBUG
		fprintf(stderr, "%s\n", insert_into);
		#endif
		//	Prompts user if information is to be added. 
		printf("'%s', '%s', %s\nAdd (y/n/q)? ", date, comment, amount);
		correct = fgetc(stdin);
		fflush(stdin);
		if (correct == 'y') {
			//	Replace comment
			printf("New comment: ");
			fgets(comment, 36, stdin);
			comment[strlen(comment)-1] = '\0';
			fflush(stdin);
			//	Add a type
			printf("Type: ");
			fgets(type, 16, stdin);
			type[strlen(type)-1] = '\0';
			fflush(stdin);
			//	Generate unique ID
			char id[ID_LEN];
			generate_id(id);
			//	Store SQL statement for execution
			snprintf(insert_into, INSERT_LEN, "insert into %s values('%s', '%s', '%s', %s, '%s');", TABLE, date, comment, type, amount, id);
			//	Execute SQL statement
			fprintf(stderr, "inserted \"%s\"\n", insert_into);
			if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK ) {
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
				return;
			}
			counter++;
		} else if (correct == 'n') {
			#ifdef DEBUG
			fprintf(stderr, "Values NOT added.\n");
			#endif
		} else if (correct == 'q') {
			break;
		}
	}
	return counter;	//	Insertions made
}

/*
 * strtok version that handles null fields
 */
char *xstrtok(char *line, char *delims)
{
	static char *saveline = NULL;
	char *p;
	int n;

	if(line != NULL)
	   saveline = line;

	/*
	*see if we have reached the end of the line 
	*/
	if(saveline == NULL || *saveline == '\0') 
            return(NULL);
	/*
	*return the number of characters that aren't delims 
	*/
	n = strcspn(saveline, delims);
	p = saveline; /*save start of this token*/

	saveline += n; /*bump past the delim*/

	if(*saveline != '\0') /*trash the delim if necessary*/
		*saveline++ = '\0';
	return(p);
}

/*
 *	Method to print help about core commands
 */
void printhelp(char *command)
{
	command += snprintf(command, COMMAND_LEN, "aCBudget.help >\n");
	command += snprintf(command, COMMAND_LEN, "%-6s - %s\n", "read", "read insertions from file");
	command += snprintf(command, COMMAND_LEN, "%-6s - %s\n", "select", "write commands directly to database");
	command += snprintf(command, COMMAND_LEN, "%-6s - %s\n", "print", "prints out balance for all accounts");
	command += snprintf(command, COMMAND_LEN, "%-6s - %s\n", "insert", "for easy insertions to database");
        command += snprintf(command, COMMAND_LEN, "%-6s - %s\n", "config", "a menu to configurate database variables");
	command += snprintf(command, COMMAND_LEN, "%-6s - %s\n", "help", "this menu");
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
 *	Executes or give feedback to user about given command
 */
char *execute_command(char *command, sqlite3 *database)
{
	if (strncmp(command, "insert\0", 7) == 0) {
		snprintf(command, COMMAND_LEN, "%d insertions made.\n", insert(command, database));
	} else if (strncmp(command, "select\0", 6) == 0) {
		return myselect(command, database);
	} else if (strncmp(command, "read\0", 5) == 0) {
		snprintf(command, COMMAND_LEN, "%d insertions made.\n", readfile(command, database));
	} else if (strncmp(command, "config\0", 7) == 0) {
		return config(command, database);
	} else if ((strncmp(command, "help\0", 5) == 0) || (strncmp(command, "h\0", 2) == 0)) {
		printhelp(command);
	} else {
		printf("aCBudget.%s > no such command\n", command);
	}
	return command;
}

/*
 *	Fetches/creates default configuration in 'config.ini' file
 */
void configurate(char *command, sqlite3 *db) {
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
		fprintf(config_file, "year=%s\n", YEAR);
		fprintf(config_file, "table=%s\n", TABLE);
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
	if (argc == 3) {
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
	} else {
		/*
		 *	General program purpose
		 *	Currently fetches information about database in header
		 *	Considering implementing a .config file for storing default/last
		 *	used database, table, current year etc.
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

