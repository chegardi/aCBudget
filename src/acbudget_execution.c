#include "acbudget.h"

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
	while (1) {
		len = get_command(filename, "filename"); filename[len-1] = '\0';
		if ((strncmp(filename, "e\0", 2) == 0) || (strncmp(filename, "end\0", 4) == 0))
			break;
		fp = fopen(filename, "r");
		if (fp == 0) {
			printf("'%s' does not exist\n", filename);
			continue;
		}
		printf("DNB (1), Sparebanken Sør (2) file or continue last(3) ? ");
		type = fgetc(stdin);
		fflush(stdin);
		if (type=='1') {
			(*READ_COUNTER) = 0;
			counter += readDNB(fp, database);
		}
		else if (type=='2') {
			(*READ_COUNTER) = 0;
			counter += readSBS(fp, database);
		}
		else if (type=='3') {
			printf("Last DNB (1) or Sparebanken Sør (2) file ? ");
			type = fgetc(stdin);
			fflush(stdin);
			if (type=='1')
				counter += readDNB(fp, database);
			else if (type=='2')
				counter += readSBS(fp, database);
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
int readDNB(FILE *fp, sqlite3 *database)
{
	#if DEBUG
	fprintf(stderr, "read DNB\n");
	#endif
	int counter = 0, error, lines = 0;
	char correct, input[INPUT_LEN], date[DATE_LEN], comment[COMMENT_LEN], type[TYPE_LEN], amount[AMOUNT_LEN], insert_into[INSERT_LEN], *token, *zErrMsg = 0;
	date[0] = '\0'; comment[0] = '\0'; type[0] = '\0';
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
		fflush(stdin);
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
		error = strlen(token);
		#if DEBUG
		fprintf(stderr, "split token: %s\n", token);
		#endif
		// Date of purchase in "Dato" field of input.
		token = strtok(input, "\";");
		#if DEBUG
		fprintf(stderr, "date: %s\n", token);
		#endif
		copydate(date, token);
		/*
		 *	Copies "Forklaring" into comment,
		 *	for further analysis by user
		 */
		token = strtok(NULL, "\";");
		#if DEBUG
		fprintf(stderr, "comment: %s\n", token);
		#endif
		strncpy(comment, token, 35);
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
		if (error > 3) {	//	Amount is deposited, decreasing money spent
			amount[0] = '-';
			copynumber(1, amount, token);
		} else	//	Amount is withdrawn, increasing money spent
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
		printf("-'%s', '%s', %s\nAdd? (y/n/q): ", date, comment, amount);
		correct = fgetc(stdin);
		if (correct == 'q') {
			lines--;
			break;
		}
		else if (correct == 'y') {
			fflush(stdin);
			// Add comment
			printf("Comment: ");
			fgets(comment,36,stdin);
			comment[strlen(comment)-1] = '\0';
			//	Generate unique ID for insertion
			//	Prompts user for type of budget-line
			printf("Type: ");
			fgets(type, 16, stdin);
			type[strlen(type)-1] = '\0'; // end type with \0 instead of \n
			char id[ID_LEN];
			generate_id(id);
			//	Generate SQL statement for insertion based on information given
			snprintf(insert_into, INSERT_LEN, "insert into %s values('%s', '%s', '%s', %s, '%s');", TABLE, date, comment, type, amount, id);
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
int readSBS(FILE *fp, sqlite3 *database)
{
	#if DEBUG
	fprintf(stderr, "read SBS\n");
	#endif
	int counter = 0, error, lines = 0;
	char	input[INPUT_LEN],					//	input
			insert_into[INSERT_LEN],		//	insert_into TABLE values(
			date[DATE_LEN],					//	date
			comment[COMMENT_LEN],	//	comment
			type[TYPE_LEN],					//	type
			amount[AMOUNT_LEN],		//	amount
			dateday[3],								//	dd daydate
			datemonth[3],							//	mm monthdate
			correct, *token, *datetoken, *zErrMsg;	//	diverse
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
		fflush(stdin);
		//	First token is interest date, stored to use if date is not given in comment
		token = xstrtok(input, "	");
		strncpy(date, token, 11);
		#if DEBUG
		fprintf(stderr, "Date: %s\n", token);
		#endif
		//	Second token is explanation (comment)
		token = xstrtok(NULL, "	");
		#if DEBUG
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
			datetoken = malloc(sizeof(char)*DATE_LEN);
			strncpy(datetoken, date, DATE_LEN);
			copydate(date, datetoken);
			/*printf("Date: %s | Comment: %s \nNew date YYYY-MM-DD: ", copydate(date, datetoken), token);
			fgets(date, 11, stdin);
			fflush(stdin);*/
		}
		//	Last token is amount
		token = xstrtok(NULL, "	");
		if (strlen(token) > 1) {
			//	Number was a withdrewal, increasing budget spent
			copynumber(0, amount, token);
		} else {
			//	Number was a deposit, decreasing budget spent
			token = xstrtok(NULL, "	");
			amount[0] = '-';
			copynumber(1, amount, token);
		}
		#if DEBUG
		fprintf(stderr, "amount: %s\n", token);
		#endif
		//	All information gathered, check for equal date in database
		snprintf(insert_into, INSERT_LEN, "select * from %s where date like '%s';", TABLE, date);
		if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		#if DEBUG
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
			#if DEBUG
			fprintf(stderr, "Values NOT added.\n");
			#endif
		} else if (correct == 'q') {
			lines--;
			break;
		}
	}
	#if	DEBUG
	fprintf(stderr, "storing %d into READ_COUNTER (%d)\n", lines, (*READ_COUNTER));
	#endif
	(*READ_COUNTER) = lines;
	return counter;	//	Insertions made
}

/*
 *	Lets user update amount column of existing rows, and the possibility of inserting a new row
 */
int update(char *command, sqlite3 *database)
{
	int updated = 0, sql_len, len;
	char	*commandhelp = malloc(sizeof(char) * COMMAND_LEN),	//	to store usage-help
			*select = malloc(sizeof(char) * SELECT_LEN) ,	//	used by sql-queries
			*comment = malloc(sizeof(char) * COMMENT_LEN),	//	to store comments
			*type = malloc(sizeof(char) * TYPE_LEN),	//	to store types
			*amount = malloc(sizeof(char) * AMOUNT_LEN),	//	to store amounts
			*day, *zErrMsg, correct;	//	helpful
	if (commandhelp == NULL || select == NULL)	return -1;	//	fail-safe
	(*commandhelp) = '\0'; (*select) = '\0';	//	default start
	P_COUNTER = malloc(sizeof(int));
	do {
		snprintf(commandhelp, COMMAND_LEN, "day in month (%s)", MONTH);
		len = get_update_command(command, commandhelp); command[len-1] = '\0';
		(*P_COUNTER) = 1;
		//	find entries based on day of month
		snprintf(select, SELECT_LEN, "select comment, amount, type from %s where date = '%s-%s-%s'", TABLE, YEAR, MONTH, command);
		#if DEBUG
		fprintf(stdout, "Running select on %s: '%s'\n", DATABASE, select);
		#endif
		if ( sqlite3_exec(database, select, numbered_callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			return -1;
		}
		if (*P_COUNTER > 1) {
			//	give user ability to divide/update entries
			day = malloc(sizeof(char) * 3); 
			snprintf(day, 3, "%s", command);
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
				//	allocating space for id from rownumber
				UNIQUE_ID = malloc(sizeof(char) * ID_LEN);
				len = snprintf(select, SELECT_LEN, "select id from %s where date = '%s-%s-%s'", TABLE, YEAR, MONTH, day);
				//	prepare statement to find correct row for update
				sqlite3_stmt **statement = malloc(1* sizeof( sqlite3_stmt *));
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
							printf("\nNumber chosen must have been higher than number of rows returned.\n");
						}
						else if (sql_result == SQLITE_MISUSE) {
							fprintf(stderr, "\nSQL misuse: Contact system creator", zErrMsg);
						}
						else if (sql_result == SQLITE_ERROR) {
							fprintf(stderr, "\nSQL step-error: Contact system creator");
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
				len = get_update_command(amount, "amount"); amount[len-1] = '\0';
				if (len > 1) {	//	amount is to be updated
					if (sql_len > (*P_COUNTER)) {	//	need to add comma after previous variable
						*(select+sql_len++) = ',';
						*(select+sql_len) = 0;
					}
					sql_len += snprintf(select+sql_len, SELECT_LEN-sql_len, " amount = %s", amount);
				}
				len = get_update_command(type, "type"); type[len-1] = '\0';
				if (len > 1) {	//	type is to be updated
					if (sql_len > (*P_COUNTER)) {	//	need to add comma after previous variable
						*(select+sql_len++) = ',';
						*(select+sql_len) = '\0';
					}
					sql_len += snprintf(select+sql_len, SELECT_LEN-sql_len, " type = '%s'", type);
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
					//	check if user wants to add another row
					printf("Want to add another row with new type and same comment on same date?: ");
					correct = fgetc(stdin);	fflush(stdin);
					if (correct == 'y') {	//	add another row with new comment, type and amount on same date
						comment = malloc(sizeof(char) * COMMENT_LEN);
						len = get_update_command(comment, "comment"); comment[len-1] = '\0';	//	comment
						len = get_update_command(type, "type"); type[len-1] = '\0';			//	type
						get_update_command(amount, "amount"); amount[ID_LEN-1] = '\0';				//	amount
						generate_id(UNIQUE_ID);																					//	id
						//	insert into TABLE values (DATE, COMMENT, TYPE, AMOUNT, ID);
						snprintf(select, SELECT_LEN, "insert into %s values('%s-%s-%s', '%s', '%s', %s, '%s');",
							TABLE, YEAR, MONTH, day, comment, type, amount, UNIQUE_ID);
						if (sqlite3_exec(database, select, NULL, 0, &zErrMsg) != SQLITE_OK) {
							fprintf(stderr, "SQL error %s\n", zErrMsg);
							sqlite3_free(zErrMsg);
							return -1;
						}
					}
				}
				//	free update specific malloc
				sqlite3_finalize(*statement);
				free(UNIQUE_ID);
			}
			//	free day-storage
			free(day); 
		}	else printf("No entries in given day (%s) of month (%s)\n", command, MONTH);
	}	while ((strncmp(command, "e\0", 2) != 0) && (strncmp(command, "end\0", 4) != 0));
	free(commandhelp); free(comment), free(type), free(amount), free(select); 
	return updated;
}
