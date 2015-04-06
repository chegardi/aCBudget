#include "acbudget.h"

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
 *	Lets user update amount column of existing rows, and the possibility of inserting a new row
 */
int update(char *command, sqlite3 *database)
{
	int updated = 0, len, counter;
	char *commandhelp = malloc(sizeof(char) * COMMAND_LEN), *select = malloc(sizeof(char) * SELECT_LEN) , *zErrMsg, correct;
	(*commandhelp) = '\0'; (*select) = '\0';	//	default start
	if (commandhelp == NULL || select == NULL)	return -1;
	snprintf(commandhelp, COMMAND_LEN, "day in month (%s)", MONTH);
	len = get_update_command(command, commandhelp); command[len-1] = '\0';
	P_COUNTER = malloc(sizeof(int));
	while ((strncmp(command, "e\0", 2) != 0) && (strncmp(command, "end\0", 4) != 0))
	{
		//	find entries based on day of month
		snprintf(select, SELECT_LEN, "select * from %s where date = '%s-%s-%s'", TABLE, YEAR, MONTH, command);
		#ifdef DEBUG
		fprintf(stdout, "Running select on %s: '%s'\n", DATABASE, select);
		#endif
		(*P_COUNTER) = 1;
		if ( sqlite3_exec(database, select, numbered_callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			return -1;
		}
		if (*P_COUNTER > 1) {
			//	give user ability to divide/update entries
			printf("Select a number to update: ");
			correct = fgetc(stdin);
			fflush(stdin);
			if (correct != 'n' && ((correct < '1') || (correct >= ('1' + (*P_COUNTER)-1))))
				printf("Invalid input: '%c', either pick number in range %d-%d or 'n' for none\n", correct, 1, ((*P_COUNTER)-1));
			else if (correct != 'n') {
				//	row selected for update
				#ifdef DEBUG
				fprintf(stderr, "Finding rownumber %c\n", correct);
				#endif
				//	allocating space for id from rownumber
				UNIQUE_ID = malloc(sizeof(char) * ID_LEN);
				if (sqlite3_exec(database, select, decreasing_callback, 0, &zErrMsg) != SQLITE_OK) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
					return -1;
				}
				//	update amount of selected row
				char *amount = malloc(sizeof(char) * 15);
				len = get_update_command(amount, "amount"); amount[len-1] = '\0';
				snprintf(select, SELECT_LEN, "update %s set amount = %s where id = '%s'", TABLE, amount, UNIQUE_ID);
				if (sqlite3_exec(database, select, NULL, 0, &zErrMsg) != SQLITE_OK) {
					fprintf(stderr, "SQL error %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
					return -1;
				}
				#ifdef DEBUG
				fprintf(stderr, "Row with id='%s' updated\n", UNIQUE_ID);
				#endif
				//	check if user wants to add another row
				printf("Want to add another row with new type and same comment on same date?: ");
				correct = fgetc(stdin);	fflush(stdin);
				if (correct == 'y') {	//	add another row with new comment, type and amount on same date
					char *day = malloc(sizeof(char) * 3), *comment = malloc(sizeof(char) * COMMENT_LEN);
					strncpy(day, command, 3);
					len = get_update_command(comment, "comment"); comment[len-1] = '\0';	//	comment
					len = get_update_command(command, "type"); command[len-1] = '\0';			//	type
					get_update_command(amount, "amount"); amount[ID_LEN-1] = '\0';				//	amount
					generate_id(UNIQUE_ID);																					//	id
					//	insert into TABLE values (DATE, COMMENT, TYPE, AMOUNT, ID);
					snprintf(select, SELECT_LEN, "insert into %s values('%s-%s-%s', '%s', '%s', %s, '%s');",
						TABLE, YEAR, MONTH, day, comment, command, amount, UNIQUE_ID);
					if (sqlite3_exec(database, select, NULL, 0, &zErrMsg) != SQLITE_OK) {
						fprintf(stderr, "SQL error %s\n", zErrMsg);
						sqlite3_free(zErrMsg);
						return -1;
					}
					//	free insert specific malloc
					free(day); free(comment); 
				}
				//	free update specific malloc
				free(amount); free(UNIQUE_ID);
			}
		}	else printf("No entries in given day (%s) of month (%s)\n", command, MONTH);
		snprintf(commandhelp, COMMAND_LEN, "day in month (%s)", MONTH);
		len = get_update_command(command, commandhelp); command[len-1] = '\0';
	}
	free(commandhelp); free(select); free(P_COUNTER);
	return updated;
}

/*
 *	Need a slightly different command_argument for the update command
 */
int get_update_command(char *command, char *command_text)
{
	fflush(stdin);
	printf("aCBudget.update > %s: ", command_text);
	return strlen(fgets(command, COMMAND_LEN, stdin));
}

/*
 *	Method to print help about core commands
 */
void printhelp(char *command)
{
	printf("aCBudget.help >\n");
	printf("%-6s - %s\n", "read", "read insertions from file");
	printf("%-6s - %s\n", "select", "write commands directly to database");
	printf("%-6s - %s\n", "insert", "for easy insertions to database");
	printf("%-6s - %s\n", "update", "for easy update (and dividing) of existing entries");
	printf("%-6s - %s\n", "config", "a menu to configurate database variables");
	printf("%-6s - %s\n", "help", "this menu");
	(*command) = '\0';	//	reset command pointer
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
	} else if (strncmp(command, "update\0",  7) == 0) {
		snprintf(command, COMMAND_LEN, "%d entries updated.\n", update(command, database));
	} else if (strncmp(command, "config\0", 7) == 0) {
		return config(command, database);
	} else if ((strncmp(command, "help\0", 5) == 0) || (strncmp(command, "h\0", 2) == 0)) {
		printhelp(command);
	} else {
		printf("aCBudget.%s > no such command\n", command);
		(*command) = '\0';
	}
	return command;
}
