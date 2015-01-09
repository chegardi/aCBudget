#include"acbudget.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (i=0; i<argc; i++) {
		printf("%s = %s | ", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}
/*
static int indexcallback(void *NotUsed, int argc, char **argv, char **azColName)
{
	if (argc > 0) {
		INDEX = strtoul(argv[0]);
	}
	return 0;
}
*/
int get_command(char *command, char *command_text)
{
	fflush(stdin);
	printf("aCBudget.%s > ", command_text);
	return strlen(fgets(command, COMMAND_LEN, stdin));
}

void generate_id(char *id)
{
	char letters[10+('z'-'a')+('Z'-'A')];
	strcpy(letters, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
	int i;
	time_t t;
	srand((unsigned) time(&t));
	for (i=0;i<ID_LEN-1; i++) {
		id[i] = letters[rand()%strlen(letters)];
	}
	id[ID_LEN-1] = '\0';
}

int insert(char *command, sqlite3 *database)
{
	int len, i, j, counter = 0;	
	char *zErrMsg = 0, insert_into[INSERT_LEN], delim[2] = ",", *token, *temp;
	/*if ( sqlite3_exec(database, "select max(number) from regnskap;", indexcallback, 0, &zErrMsg) != SQLITE_OK ) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return("See error message\n");
	}*/
	printf("usage (e or end to quit);\n"\
			 "***\naCBudget.insert > date,comment,type,amount\n"\
			 "aCBudget.insert > 2014-12-11,Rema 1000,mat,123\n***\n");
	len = get_command(command, "insert"); command[len-1] = '\0';
	while ((strncmp(command, "e\0", 2) != 0) && (strncmp(command, "end\0", 4) != 0)) {	   
		snprintf(insert_into, INSERT_LEN, "insert into regnskap values(");
		INDEX++;
		token = strtok(command, delim);
		for (i=0; i<3; i++) {
			sprintf(temp, "'%s',", token);
			strncat(insert_into, temp, INSERT_LEN);
			token = strtok(NULL, delim);
		}
		sprintf(temp, "%s,", token);
		strncat(insert_into, temp, INSERT_LEN);
		char id[ID_LEN];
		generate_id(id);
		sprintf(temp, "'%s');", id);
		strncat(insert_into, temp, INSERT_LEN);
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
		len = get_command(command, "insert"); command[len-1] = '\0';
	}
	return counter;
}

char *select(sqlite3 *database)
{
	int len, counter = 0;
	char select[250], *zErrMsg;
	select[0] = '\0';
	len = get_command(select, "select"); select[len-1] = '\0';
	while ((strncmp(select, "e\0", 2) != 0) && (strncmp(select, "end\0", 4) != 0)) {
		if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		len = get_command(select, "select"); select[len-1] = '\0';
	}
	return "\0";
}

int readfile(char *command, sqlite3 *database)
{
	int len, counter = 0;
	FILE *fp;
	char filename[50], type[2];
	filename[0] = '\0';
	len = get_command(filename, "filename"); filename[len-1] = '\0';
	while ((strncmp(filename, "e\0", 2) != 0) && (strncmp(filename, "end\0", 4) != 0)) {
		fp = fopen(filename, "r");
		printf("(1) DNB file or (2) Sparebanken Sør file? ");
		fgets(type, 2, stdin); type[1] = '\0';
		if (type[0]=='1')
			counter += readDNB(fp, database);
		else if (type[0]=='2')
			counter += readSBS(fp, database);
		else
			printf("%c was not a valid file option.\n", type);
		fclose(fp);
		len = get_command(filename, "filename"); filename[len-1] = '\0';
	}
	return counter;
}

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
	printf("token.amount=%s vs %s=sql.amount\n", from,to);
	#endif
}

void copydate(char *date, char *token)
{
	#ifdef DEBUG
	printf("token.date=%s\n", token);
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
	printf("regnskap.date=%s\n", date);
	#endif
}

int readDNB(FILE *fp, sqlite3 *database)
{
	#ifdef DEBUG
	fprintf(stderr, "read DNB\n");
	#endif
	int counter = 0, error;
	char input[INPUT_LEN], date[11], comment[36], type[16], amount[16], *token, correct, *zErrMsg = 0, insert_into[INSERT_LEN];
	date[0] = '\0'; comment[0] = '\0'; type[0] = '\0';
	while (fgets(input, INPUT_LEN, fp) != NULL) {
		/*
		"Dato";"Forklaring";"Rentedato";"Uttak";"Innskudd"
		"14.12.2014";"Morsom sparing kort avrunding Reservert transaksjon";"";"3,00";""
		*/
		fflush(stdin);
		#ifdef DEBUG
		fprintf(stderr, "input: '%s'", input);
		#endif
		token = strstr(input, "\"\"");
		error = strlen(token);
		#ifdef DEBUG
		fprintf(stderr, "token1: %s\n", token);
		#endif
		token = strtok(input, "\";");
		#ifdef DEBUG
		fprintf(stderr, "token2: %s\n", token);
		#endif
		copydate(date, token);
		token = strtok(NULL, "\";");
		#ifdef DEBUG
		fprintf(stderr, "token3: %s\n", token);
		#endif
		strncpy(comment, token, 35);
		token = strtok(NULL, "\";");
		#ifdef DEBUG
		fprintf(stderr, "token4: %s\n", token);
		#endif
		token = strtok(NULL, "\";");
		#ifdef DEBUG
		fprintf(stderr, "token5: %s\n", token);
		#endif
		if (error > 3) {
			amount[0] = '-';
			copynumber(1, amount, token);
		} else
			copynumber(0, amount,token);
		printf("@Rows with equal date and/or amount:\n");
		snprintf(insert_into, INSERT_LEN, "select * from regnskap where amount like %s or date like '%s';",amount, date);
		if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		#ifdef DEBUG
		fprintf(stderr, "%s\n", insert_into);
		#endif
		printf("-'%s', '%s', %s\nAdd? (y/n/q): ", date, comment, amount);
		correct = fgetc(stdin);
		if (correct == 'q')
			break;
		else if (correct == 'y') {
			fflush(stdin);
			printf("Type?: ");
			fgets(type, 16, stdin);
			type[strlen(type)-1] = '\0';
			printf("---'%s', '%s', '%s', %s\nCorrect (y/n/e)? ", date, comment, type, amount);
			fflush(stdin);
			correct = fgetc(stdin);
			if (correct == 'y' || correct == 'e') {
				if (correct = 'e') {
					fflush(stdin);
					printf("New comment: ");
					fgets(comment,36,stdin);
					comment[strlen(comment)-1] = '\0';
				}
				char id[ID_LEN];
				generate_id(id);
				snprintf(insert_into, INSERT_LEN, "insert into regnskap values('%s', '%s', '%s', %s, '%s');", date, comment, type, amount, id);
				//#ifdef DEBUG
				fprintf(stderr, "%s\n", insert_into);
				//#endif
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
	return counter;
}

int readSBS(FILE *fp, sqlite3 *database)
{
	#ifdef DEBUG
	fprintf(stderr, "read SBS\n");
	#endif
	int counter = 0, error, i;
	char input[INPUT_LEN], date[11], dateday[3], datemonth[3], comment[36], type[16], amount[16], correct, *token = 0, *datetoken = 0, *zErrMsg = 0, insert_into[INSERT_LEN], year[] = "2014";
	date[0] = '\0'; comment[0] = '\0'; type[0] = '\0';
	while (fgets(input, INPUT_LEN, fp) != NULL)
	{
		// Dato	Forklaring	Ut av konto	Inn på konto
		fflush(stdin);
		token = xstrtok(input, "	");
		strncpy(date, token, 11);
		#ifdef DEBUG
		fprintf(stderr, "token1: %s\n", counter, token);
		#endif
		token = xstrtok(NULL, "	");
		#ifdef DEBUG
		fprintf(stderr, "token2: %s\n", counter, token);
		#endif
		strncpy(comment, token, 36);
		datetoken = strtok(token, ".");
		if (strlen(datetoken) < 4) {
			strncpy(dateday, datetoken, 3);
			datetoken = strtok(NULL, " ");
			strncpy(datemonth, datetoken, 3);
			snprintf(date, 11, "%s-%s-%s", year, datemonth, dateday);
		} else {
			printf("Date: %s | Comment: %s \nNew date YYYY-MM-DD: ", date, token);
			fgets(date, 11, stdin);
			fflush(stdin);
		}
		token = xstrtok(NULL, "	");
		#ifdef DEBUG
		fprintf(stderr, "token3: %s\n", counter, token);
		#endif
		if (strlen(token) > 1)
		{
			copynumber(0, amount, token);
		} else {
			token = xstrtok(NULL, "	");
			amount[0] = '-';
			copynumber(1, amount, token);
		}
		#ifdef DEBUG
		fprintf(stderr, "token4: %s\n", counter, token);
		#endif
		snprintf(insert_into, INSERT_LEN, "select * from regnskap where amount like %s or date like '%s';",amount, date);
		if ( sqlite3_exec(database, insert_into, callback, 0, &zErrMsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		#ifdef DEBUG
		fprintf(stderr, "%s\n", insert_into);
		#endif
		fflush(stdin);
		printf("'%s', '%s', %s\nAdd (y/n/q)? ", date, comment, amount);
		fflush(stdin);
		correct = fgetc(stdin);
		fflush(stdin);
		if (correct == 'y') {
			printf("New comment: ");
			fgets(comment, 36, stdin);
			comment[strlen(comment)-1] = '\0';
			fflush(stdin);
			printf("Type: ");
			fgets(type, 16, stdin);
			type[strlen(type)-1] = '\0';
			fflush(stdin);
			char id[ID_LEN];
			generate_id(id);
			snprintf(insert_into, INSERT_LEN, "insert into regnskap values('%s', '%s', '%s', %s, '%s');", date, comment, type, amount, id);
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
	return counter;
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

void printhelp(char *command)
{
	command += snprintf(command, COMMAND_LEN, "aCBudget.help >\n");
	command += snprintf(command, COMMAND_LEN, "%-10s - %s\n", "read", "read insertions from file");
	command += snprintf(command, COMMAND_LEN, "%-10s - %s\n", "select", "write commands directly to database");
	command += snprintf(command, COMMAND_LEN, "%-10s - %s\n", "insert", "for easy insertions to database");
	command += snprintf(command, COMMAND_LEN, "%-10s - %s\n", "help", "this menu");
}

char *execute_command(char *command, sqlite3 *database)
{
	if (strncmp(command, "insert\0", 7) == 0) {
		snprintf(command, COMMAND_LEN, "%d insertions made.\n", insert(command, database));
	} else if (strncmp(command, "select\0", 6) == 0) {
		return select(database);
	} else if (strncmp(command, "read\0", 5) == 0) {
		snprintf(command, COMMAND_LEN, "%d insertions made.\n", readfile(command, database));
	} else if ((strncmp(command, "help\0", 5) == 0) || (strncmp(command, "h\0", 2) == 0)) {
		printhelp(command);
	} else {
		printf("aCBudget.%s > no such command\n", command);
	}
	return command;
}

int main(int argc, char **argv)
{
	sqlite3 *database;
	char *zErrMsg = 0, command[COMMAND_LEN], *printout;
	int rc, len;
	
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
	}
	else {
		rc = sqlite3_open(DATABASE, &database);
		if ( rc ) {
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(database));
			return(1);
		}
		len = get_command(command, "main"); command[len-1] = '\0';
		while ((strncmp(command, "q\0", 2) != 0) && (strncmp(command, "quit\0", 5) != 0)) {
			#ifdef DEBUG
			fprintf(stderr, "%s (%d)\n", command, len);
			#endif
			printout = execute_command(command, database);
			if (strlen(printout) > 1) {
				printf("%s", printout);
			}
			len = get_command(command, "main"); command[len-1] = '\0';
		}
		sqlite3_close(database);
	}
	return 0;
}

