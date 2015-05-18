#include "acbudget.h"

/*
 *	Most basic sql execution environment
 */
int easyExecuteSQL(int argc, char **argv)
{
	sqlite3 *database;
	char *zErrMsg = 0;
	int rc = sqlite3_open(argv[1], &database);
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
	return 0;
}

/*
 *	Used to print out calls from database
 */
int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (i=0; i<argc; i++) { // number of columns
		printf("%s = %s | ", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

/*
 *	Used to print out numbered calls from database using P_COUNTER
 */
int numbered_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (i=0; i<argc; i++) { // number of columns
		if (i == 0)	printf("%d: %s = %s | ", (*P_COUNTER)++, azColName[i], argv[i] ? argv[i] : "NULL");
		else printf("%s = %s | ", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

/*
 *	Used to find correct UNIQUE ID after numbered call
 *	This implementation is BAD, and only a hack until I figure out the SQLite way of doing this
 *	Will be removed after a better implementation is used in future updates
 */
int decreasing_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	#ifdef DEBUG
	fprintf(stderr, "Checking P_POINTER: %d == 1\n", (*P_COUNTER));
	#endif
	if ((*P_COUNTER) == 1)	//	one value above because '''(*P_COUNTER)--;''' has not been called by this method yet
	{
		int i;
		for (i=0; i<argc; i++) {
			if (strncmp(azColName[i], "id", 2) == 0) {
				strncpy(UNIQUE_ID, argv[i], ID_LEN);
				#ifdef DEBUG
				fprintf(stderr, "Copyied '%s' into UNIQUE_ID, = '%s'\n", argv[i], UNIQUE_ID);
				#endif
			}
		}
	}
	 (*P_COUNTER)--;
	return 0;
}

/*
 *	insert command
 *	Used for standardized insertions to database
 */
int insert(char *command, sqlite3 *database)
{
	int len, i, counter = 0;	
	char *zErrMsg = 0, *insert = malloc(sizeof(char)*INSERT_LEN), *token, id[ID_LEN];
	if (insert == NULL)	return -1;
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
			//snprintf(insert, INSERT_LEN, "\0");
			(*insert) = '\0'; len = 0;
			len += snprintf(insert + len, INSERT_LEN, "insert into %s values(", TABLE);
			//	read and store date, comment and type
			for (i =0; i<3; i++) {
				if (token != NULL) {
					#ifdef DEBUG
					fprintf(stderr, "%s\n", token);
					#endif
					len += snprintf(insert + len, INSERT_LEN, "'%s', ", token);
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
				len += snprintf(insert + len, INSERT_LEN, "%s, ", token);
				//	Generate and store unique ID
				generate_id(id);
				len += snprintf(insert + len, INSERT_LEN, "'%s');", id);
				#ifdef DEBUG
				fprintf(stderr, "%s\n", id);
				#endif
				//	execute SQL command
				if ( sqlite3_exec(database, insert, NULL, 0, &zErrMsg) != SQLITE_OK ) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
					return;
				}
				#ifdef DEBUG
				fprintf(stderr, "%s\n", insert);
				#endif
				counter++;
			}
		}
		len = get_command(command, "insert"); command[len-1] = '\0';
	}
	free(insert);
	return counter;
}

/*
** Method is copied from sqlite.org, altered to fit my code.
** REWRITTEN ORIGINAL DESCRIPTION
** This function is used to load the contents of a database file on disk 
** into the "main" database of open database connection database, or
** to save the current contents of the database opened by database into
** a database file on disk. database is probably an in-memory database, 
** but this function will also work fine if it is not.
*/
int revertOrBackup(sqlite3 *database, int isSave)
{
	int rc;                   /* Function return code */
	sqlite3 *file;           /* Database connection opened on DATABASE */
	sqlite3_backup *pBackup;  /* Backup object used to copy data */
	sqlite3 *to;             /* Database to copy to (file or database) */
	sqlite3 *from;           /* Database to copy from (file or database) */
	int len = (strlen(DATABASE) + strlen(".backup"))+1;	/* string-length */
	char *backupname = malloc(sizeof(char) * len);		/* name of backup-db */
	snprintf(backupname, len, "%s.backup", DATABASE);

	/* Open the database file identified by DATABASE. Exit early if this fails
	** for any reason. */
	rc = sqlite3_open(backupname, &file);
	if( rc==SQLITE_OK )
	{
		/* If this is a 'load' operation (isSave==0), then data is copied
		** from the database file just opened to database database. 
		** Otherwise, if this is a 'save' operation (isSave==1), then data
		** is copied from database to file.  Set the variables from and
		** to accordingly. */
		from = (isSave ? database : file);
		to   = (isSave ? file     : database);

		/* Set up the backup procedure to copy from the "main" database of 
		** connection file to the main database of connection database.
		** If something goes wrong, pBackup will be set to NULL and an error
		** code and  message left in connection to.
		**
		** If the backup object is successfully created, call backup_step()
		** to copy data from file to database. Then call backup_finish()
		** to release resources associated with the pBackup object.  If an
		** error occurred, then  an error code and message will be left in
		** connection to. If no error occurred, then the error code belonging
		** to to is set to SQLITE_OK.
		*/
		pBackup = sqlite3_backup_init(to, "main", from, "main");
		if( pBackup )
		{
			(void)sqlite3_backup_step(pBackup, -1);
			(void)sqlite3_backup_finish(pBackup);
		}
		else fprintf(stdout, "backup_init failed\n");
		rc = sqlite3_errcode(to);
	}
	/* Close the database connection opened on database file DATABASE
	** and return the result of this function. */
	(void)sqlite3_close(file);
	free(backupname);
	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\nContact program developer.\n", sqlite3_errmsg(database));
		return -1;
	}
	#ifdef DEBUG
	(isSave ? printf("Database backuped\n") : printf("Database reverted\n") );
	#endif
	return 0;
}

/*
 *	Select command
 *	Executes commands from user as long as e/end is not typed
 */
char *myselect(char *command, sqlite3 *database)
{
	int len, counter = 0;
	char *select = malloc(sizeof(char) * SELECT_LEN), *zErrMsg, execute;
	if (select == NULL) {
		snprintf(command, COMMAND_LEN, "Error allocating memory for operation");
	}
	else
		{
		printf("===WARNING===\nAny statements written WILL be executed! Be careful NOT to execute unintended statements on database.\n===WARNING===\n");
		len = get_command(select, "select"); select[len-1] = '\0';
		while ((strncmp(select, "e\0", 2) != 0) && (strncmp(select, "end\0", 4) != 0)) {
			if (strncmp(select, "select ", 7) != 0) {
				printf("Really execute '%s', ?: ", select);
				execute = getc(stdin); fflush(stdin);
			}	else execute = 'y';
			if (execute == 'y') {
				if ( sqlite3_exec(database, select, callback, 0, &zErrMsg) != SQLITE_OK) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				} else {
					counter++;
				}
			}
			len = get_command(select, "select"); select[len-1] = '\0';
		}
		snprintf(command, COMMAND_LEN, "%d commands excuted\n", counter);
		free(select);
	}
	return command;
}
