#include "acbudget.h"

/*
 *	Used to print out general calls from database
 */
int callback( void *NotUsed, int argc, char **argv, char **azColName )
{

	int i;
	for ( i=0; i<argc; i++ ) { // number of columns
		
		printf( "%s = %s | ", azColName[i], argv[i] ? argv[i] : "NULL" );
		
	}
	printf( "\n" );
	return 0;
	
}

/*
 *	Most basic sql execution environment
 */
int easy_execute_sql( int argc, char **argv )
{

	sqlite3 *database;
	char *zErrMsg = 0;
	int rc = sqlite3_open( argv[1], &database );
	
	if ( rc ) {
		fprintf( stderr, "Can't open database: %s\n", sqlite3_errmsg( database ) );
		return(1);
	}
	
	rc = sqlite3_exec( database, argv[2], callback, 0, &zErrMsg );

	if ( rc != SQLITE_OK ) {
		fprintf( stderr, "SQL error: %s\n", zErrMsg );
		sqlite3_free( zErrMsg );
	}
	
	sqlite3_close( database );
	return 0;
	
}

/*
 * Executes the sql-query
 */
int regular_execute_sql( char *query )
{

	if ( sqlite3_exec( database, query, callback, 0, &zErrMsg ) != SQLITE_OK ) {

		fprintf( stderr, "SQL error: %s\n", zErrMsg );
		sqlite3_free( zErrMsg );
		return 0; // NOT OK
		
	} else {
		
		#if DEBUG
		printf("%s\n", query);
		#endif
		
	}
	
	return 1; // OK
	
}

/*
 *	insert command
 *	Used for standardized insertions to database
 */
int insert( char *command, sqlite3 *database )
{

	int len, i, counter = 0;	
	char *zErrMsg = 0, *insert = calloc( 1, sizeof( char ) * INSERT_LEN ), *token, id[ID_LEN];

	if ( !insert ) {

		return -1;
		
	}
	
	char usage[] = "usage (e or end to quit)\n"\
		"***\naCBudget.insert > YYYY-MM-DD,comment,type,amount\n"\
		"aCBudget.insert > 2014-12-11,Rema 1000,mat,123\n***\n";
	printf( "%s", usage );
	len = prompt( command, "insert" );
	
	while ( ( strncmp( command, "e\0", 2 ) != 0 ) && ( strncmp( command, "end\0", 4 ) != 0 ) ) {
		
		if ( ( strncmp( command, "h\0", 2 ) == 0 ) || ( strncmp( command, "help\0", 5 ) == 0 ) ) {	// help
		
			printf("%s", usage);
			*command = '\0';
			
		} else {
		
			token = strtok(command, ",");
			//snprintf(insert, INSERT_LEN, "\0");
			( *insert ) = '\0'; len = 0;
			len += snprintf( insert + len, INSERT_LEN, "insert into %s values(", TABLE );
			//	read and store date, comment and type

			for ( i=0; i<3; i++ ) {
		
				if ( !token ) {		
					break;
				}
				
				#if DEBUG
				fprintf( stderr, "token[%d]: %s\n", i, token );
				#endif
				len += snprintf( insert + len, INSERT_LEN, "'%s', ", token );
				token = strtok( NULL, "," );
		
				
			}

			if (token) {
				
				//	store amount
				#if DEBUG
				fprintf( stderr, "%s\n", token );
				#endif
				len += snprintf( insert + len, INSERT_LEN, "%s, ", token );

				//	Generate and store unique ID
				generate_id( id );
				len += snprintf( insert + len, INSERT_LEN, "'%s');", id );
				#if DEBUG
				fprintf( stderr, "%s\n", id );
				#endif
				
				//	execute SQL command
				if ( sqlite3_exec( database, insert, NULL, 0, &zErrMsg ) != SQLITE_OK ) {
		
					fprintf( stderr, "SQL error: %s\n", zErrMsg );
					sqlite3_free( zErrMsg );
					return;
					
				}
				
				#if DEBUG
				fprintf( stderr, "%s\n", insert );
				#endif
				counter++;
				
			}
			else { // not enough inputs
		
				printf( "Wrong input: %s\n%s", command, usage );
				
			}
			
		}

		len = prompt( command, "insert" );
		
	}
	
	free( insert );
	return counter;
	
}

/*
 *	Used to print out numbered calls from database using P_COUNTER
 */
int numbered_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for ( i=0; i<argc; i++ ) { // number of columns
		if ( i == 0 ) {
			printf("%d: %s = %s | ", ( *P_COUNTER )++, azColName[i], argv[i] ? argv[i] : "NULL");
		}
		else  {
			printf( "%s = %s | ", azColName[i], argv[i] ? argv[i] : "NULL" );
		}
	}
	printf( "\n" );
	return 0;
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
int revert_or_backup(sqlite3 *database, int isSave)
{
	int rc;                   /* Function return code */
	sqlite3 *file;           /* Database connection opened on DATABASE */
	sqlite3_backup *pBackup;  /* Backup object used to copy data */
	sqlite3 *to;             /* Database to copy to (file or database) */
	sqlite3 *from;           /* Database to copy from (file or database) */
	int len = (strlen(DATABASE) + strlen(".backup"))+1;	/* string-length */
	char *backupname = calloc(1, sizeof(char) * len);		/* name of backup-db */
	snprintf(backupname, len, "%s.backup", DATABASE);

	/* Open the database file identified by DATABASE. Exit early if this fails
	** for any reason. */
	rc = sqlite3_open(backupname, &file);
	if( rc==SQLITE_OK ) {
		/* If this is a 'load' operation (isSave==0), then data is copied
		** from the database file just opened to database database. 
		** Otherwise, if this is a 'save' operation (isSave==1), then data
		** is copied from database to file.  Set the variables from and
		** to accordingly. */
		from = ( isSave ? database : file );
		to   = ( isSave ? file     : database );

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
		pBackup = sqlite3_backup_init( to, "main", from, "main" );
		if( pBackup ) {
	
			( void ) sqlite3_backup_step( pBackup, -1 );
			( void ) sqlite3_backup_finish( pBackup );
			
		}
		else {

			fprintf( stderr, "backup_init failed\n" );
			
		}
		rc = sqlite3_errcode( to );
	}
	
	/* Close the database connection opened on database file DATABASE
	** and return the result of this function. */
	( void ) sqlite3_close( file );
	free( backupname );
	if ( rc != SQLITE_OK ) {
	
		fprintf(stderr, "SQL error: %s\nContact program developer.\n", sqlite3_errmsg(database));
		return -1;
		
	}
	#if DEBUG
	( isSave ? printf( "Database backuped\n" ) : printf( "Database reverted\n" ) );
	#endif
	return 0;
	
}

