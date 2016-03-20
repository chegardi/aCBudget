#include "acbudget.h"


/*
 *	Provides runtime configuration-options
 */
char *config_command( char *command, sqlite3 *database )
{
	int len;
	char *variable, *value;
	len = prompt( command, "config" );
	while ( ( equals( command, "e" ) != 0 ) &&
	        ( equals( command, "end" ) != 0 ) ) {
		// 'e' or 'end' ends config command'


		len = strlen( command );
		variable = xstrtok( command, "=" );
		if ( len != strlen( variable ) ) {	//	unequal length implies a '=' sign
			
			value = xstrtok( NULL, "" );
			if ( equals( variable, "year" ) == 0 ) {
				
				free( YEAR );
				YEAR = calloc( 1, sizeof( char ) * strlen( value ) );
				strcpy( YEAR, value );
				
			} else if ( equals( variable, "month" ) == 0 ) {
				
				free( MONTH );
				MONTH = calloc( 1, sizeof( char ) * strlen( value ) );
				strcpy( MONTH, value );
				
			} else if ( equals( variable, "table" ) == 0 ) {

				free( TABLE );
				TABLE = calloc( 1, sizeof( char ) * strlen( value ) );
				strcpy( TABLE, value );
				
			} else if ( equals( variable, "database" ) == 0 ) {
				/*
				 * database to be changed, this section is a bit messy because
				 * sometimes it works, and sometimes it doesn't 
				 */
				int retval = sqlite3_close( database );
				if ( retval == SQLITE_OK ) {

					if ( sqlite3_open( value, &database ) != SQLITE_OK ) {
						//	database could not be opened						
						fprintf( stderr,
						        "Failed to open database (%s). SQLError-message: %s\n%s",
						        "Trying to open old database...", value, sqlite3_errmsg( database ) );
						
						if ( sqlite3_open( DATABASE, &database ) != SQLITE_OK ) { 
							//	old database could not be opened
							fprintf( stderr, "Failed to open old database (%s). SQLError-message: %s\nProgram shutdown to prevent damage to files.", DATABASE, sqlite3_errmsg( database ) );
							exit(EXIT_FAILURE);
							
						}
						
					}
					else {
						//	database opened successfully
						len = strlen( value ) + 1;
						if ( strlen( DATABASE ) != strlen( value ) ) {	//	unequal lengths, new mallocation neccesary

							free( DATABASE );
							DATABASE = calloc( 1, sizeof( char ) * len );
							
						}
						strncpy( DATABASE, value, len );
						
					}
					
				}
				else {

					printf( "Could not change database to '%s', try again\n", value );
				}
			}
			else if ( equals( variable, "read" ) == 0 ) {
				
				(*READ_COUNTER) = atoi( value );
				
			}
			else {
				printf( "No such configurable variable: %s\n", variable );
			}
		}
		else if ( ( equals( command, "sv") == 0 ) ||
		          ( equals( command, "save" ) == 0 ) ) {
			// 'sv' or 'save' is save config command
			save_config( command, database );
			printf( "%s", command );
			
		}
		else if ( ( equals( command, "ld" ) == 0 ) ||
		          ( equals( command, "load" ) == 0 ) ) {
			// 'ld' or 'load' load configuration command
			if ( configurate( command ) ) {
				// true means it could NOT load config
				printf( "Loading configuration from '%s' failed.\n", CONFIG_FILENAME );
			}
			else {
				// configuration loaded
				printf( "Configuration from '%s' loaded.\n", CONFIG_FILENAME );
			}
			
		}
		else if ( ( equals( command, "sw") == 0 ) ||
		          ( equals( command, "show" ) == 0 ) ) {
			// 'sw' or 'show' displays current configurations
			printf( "Database: %s\nTable: %s\nMonth: %s\nYear: %s\nRead: %d\n",
			       DATABASE, TABLE, MONTH, YEAR, (*READ_COUNTER) );
			
		}
		else if ( ( equals( command, "yr") == 0 ) ||
		          ( equals( command, "year" ) == 0 ) ) {
			// 'yr' or 'year' shows current variable for YEAR
			printf( "Year=%s\n", YEAR );
			
		}
		else if ( ( equals( command, "mn" ) == 0 ) ||
		          ( equals( command, "month" ) == 0 ) ) {
			// 'mn' or 'month' shows current variable for MONTH
			printf( "Month=%s\n", MONTH );
			
		}
		else if ( ( equals( command, "tb" ) == 0 ) ||
		          ( equals( command, "table" ) == 0 ) ) {
			// 'tb' or 'table' shows current variable for TABLE
			printf( "Table=%s\n", TABLE );
			
		}
		else if ( ( equals( command, "rd" ) == 0 ) ||
		          ( equals( command, "read") == 0 ) ) {
			// 'rd' or 'read' shows current variable for READ_COUNTER
			printf( "Read=%d\n", (*READ_COUNTER) );
			
		}
		else if ( ( equals( command, "db" ) == 0 ) ||
		          ( equals( command, "database") == 0 ) ) {
			// 'db' or 'database' shows current variable for DATABASE
			printf( "Database=%s\n", DATABASE );
			
		}
		else if ( ( equals( command, "bu" ) == 0 ) ||
		          ( equals( command, "backup" ) == 0 ) ) {
			// 'bu' or 'backup' stores a copy of the current database
			if ( revert_or_backup( database, 1 ) ) {
				// TRUE if backup failed
				snprintf( command, COMMAND_LEN, "Failed to backup, exit program and contact developer!\n" );
				return command;
				
			}
			
		}
		else if ( ( equals( command, "rv" ) == 0 ) ||
		          ( equals( command, "revert" ) == 0 ) ) {
			// 'rv' or 'revert' restores a previous backup of database
			if ( revert_or_backup( database, 0 ) ) { // TRUE if revert failed

				snprintf( command, COMMAND_LEN, "Failed to revert: exit program and contact developer!\n" );
				return command;
				
			}
		}
		else if ( ( equals( command, "h" ) == 0 ) ||
		          ( equals( command, "help" ) == 0 ) ) {
			// 'h' or 'help' prints out commands available within config
			// setup
			int shortLen = 2, longLen = 8, tabulateLen = 2;
			char *usageString, *tempString;
			asprintf( &tempString, "%%%ds%%%%-%ds or %%%%%ds - %%%%s\n", tabulateLen, shortLen, longLen );
			asprintf( &usageString, tempString, " " );		   
			free( tempString );

			// print
			printf( "Commands withing config:\n" );
			printf( usageString, "h", "help", "Displays this help text" );
			printf( usageString, "e", "end", "Ends the configuration session" );
			printf( usageString, "sv", "save", "Saves the current configuration to file" );
			printf( usageString, "ld", "load", "Loads configuration from file" );
			printf( usageString, "sw", "show", "Shows the current configuration in program" );
			printf( usageString, "bu", "backup", "Backups current database" );
			printf( usageString, "rv", "revert", "Reverts the database to backup" );
			// print special commands
			printf( "Following commands are able to be redfined,\n" );
			printf( "%14s - %s\n", "variable=value", "sets variable(v) to new value" );
			printf( usageString, "db", "database", "Displays current database(v)" );
			printf( usageString, "tb", "table", "Displays current table(v) in database" );
			printf( usageString, "yr", "year", "Displays default year(v)" );
			printf( usageString, "mn", "month", "Displays default month(v)" );
			printf( usageString, "rd", "read", "How many lines to skip if continuing reading from file" );
			free( usageString );
			
		} else {
			printf( "No such command: %s\n", command );
		}
		// next command
		len = prompt( command, "config" );
		
	}
	
	return command;
}

/*
 *	Executes or give feedback to user about given command
 */
char *execute_command(char *command, sqlite3 *database)
{
	if ( equals(command, "insert" ) == 0 ) {
		
		snprintf(command, COMMAND_LEN, "%d insertions made.\n", insert( command, database ) );
		
	} else if ( equals( command, "select" ) == 0 ) {
		
		return myselect( command, database );
		
	} else if ( equals( command, "read" ) == 0 ) {
		
		snprintf(command, COMMAND_LEN, "%d insertions made.\n", read_file( command, database ));
		
	} else if ( equals( command, "update" ) == 0 ) {
		
		int updated = update( command, database );
		if ( updated < 0 ) {
			return 0;
		}
		snprintf(command, COMMAND_LEN, "%d entries updated.\n", updated);
		
	} else if ( equals( command, "stats" ) == 0 ) {
		
		snprintf(command, COMMAND_LEN, "%d stats printed.\n", print_stats( command, database ) );
		
	} else if ( equals( command, "config" ) == 0 ) {
		
		return config_command( command, database );
		
	} else if ( ( equals( command, "help" ) == 0 ) ||
	            ( equals( command, "h" ) == 0 ) ) {
		
		print_help();
		
	} else {

		printf("aCBudget.%s > no such command\n", command);
		(*command) = 0;
		
	}
	return command;
}

/*
 *
 */
char insertRequest( char *correct, insert_t insert, char *insert_into ) {
	char singleInput = 0;
	
	snprintf( insert_into,
	          INSERT_LEN,
	          "select * from %s where date like '%s';",
	          TABLE,
	          insert.date );
	if ( SETTINGS&4 ) {
		// Amount exact only, added in the end of insert_into string
		snprintf( insert_into + (int)(strlen( insert_into )-1 ),
		          INSERT_LEN,
		          " and amount like %s;", insert.amount );
		
	}
#if DEBUG
	fprintf(stderr, "%s\n", insert_into);
#endif
	if ( !regular_execute_sql( insert_into ) ) {
		//  Insertion failed
		fprintf( stderr, "Failed to execute '%s' on database '%s'\n", insert_into, DATABASE );
		return -1;
			
	}
	printf( "---END-ROWS---\n-'%s', '%s', %s\nAdd? (y/n/q): ",
	        insert.date, insert.comment, insert.amount );

	//	Prompts user if information is to be added.
	scanf( "%c", &singleInput );
	clean_stdin();

	/*
	 *	Shows user information about date, comment and amount
	 *	And prompts for input about what to do with current insert
	 *	Add? y_es continues
	 *	Add? n_o skips insertion, and continues read of file
	 *	Add? q_uit exits reading from file and ends current loop
	 */
	while ( !correct_input( singleInput, correct ) ) {
		//  Illegal input
		printf( "Please answer either of: %s\nAdd? ", correct );
		scanf( "%c", &singleInput );
		clean_stdin();
		
	}
	if ( singleInput == 'y' ) {
		// y[es] data is to be inserted
		
		// Add comment
		printf( "Comment: " );
		fgets( insert.comment, COMMENT_LEN, stdin );
		insert.comment[strlen( insert.comment )-1] = '\0';
		
		//	Prompts user for type of budget-line
		printf( "Type: " );
		fgets(insert.type, TYPE_LEN, stdin);
		insert.type[strlen( insert.type )-1] = '\0'; // end type with \0 instead of \n
		
		//	Generate unique ID for insertion
		generate_id( insert.id );
		
		//	Generate SQL statement for insertion based on information given
		snprintf( insert_into, INSERT_LEN, "insert into %s values('%s', '%s', '%s', %s, '%s');",
		          TABLE, insert.date, insert.comment, insert.type, insert.amount, insert.id );
#if DEBUG
		fprintf( stderr, "%s\n", insert_into );
#endif
		
		//	Execute SQL insertion statement
		if ( !regular_execute_sql( insert_into ) ) {
			//  Execution failed
			printf( "Execution failed on '%s'\n", insert_into );
			return -1;
			
		}
		
	}
	return singleInput;
	
}

/*
 *	Select command
 *	Executes commands from user as long as e/end is not typed
 */
char *myselect( char *command, sqlite3 *database )
{
	int len, counter = 0;
	char select[SELECT_LEN], execute;
	
	printf( "===WARNING===\nAny statements written WILL be executed! Be careful NOT to execute unintended statements on database.\n===WARNING===\n" );
	len = prompt( select, "select" );
	while ( ( equals( select, "e" ) != 0 ) && ( equals( select, "end" ) != 0 ) ) {

		if ( strncmp( select, "select ", 7 ) != 0 ) {

			printf( "Really execute '%s', ?: ", select );
			scanf( "%c", &execute );
			clean_stdin();
		}	else {
			
			execute = 'y';

		}
		if ( execute == 'y' ) {

			#ifdef DEBUG
			fprintf( stderr, "sql statement: '%s'\n", select );
			#endif

			if ( regular_execute_sql( select ) ) {
				counter++;
			}
			
		}
		len = prompt( select, "select" );
		
	}
	snprintf( command, COMMAND_LEN, "%d commands excuted\n", counter );
	return command;
}

/*
 *	Method to print help about core commands from 'aCBudget.main >'
 */
void print_help( void )
{
	int tabulateLen = 0, commandLen = 7;
	char *tempString, *usageString;
	asprintf( &tempString, "%%-%ds%%%%-%%ds - %%%%s\n", tabulateLen );
	asprintf( &usageString, tempString, "", commandLen );
	free( tempString );

	printf( "aCBudget.main > command\n" );
	printf( usageString, "command", "Description" );
	printf( usageString, "select", "write commands directly to database" );
	printf( usageString, "insert", "for easy insertions to database" );
	printf( usageString, "read", "read insertions from file" );
	printf( usageString, "update", "for easy update (and dividing) of existing entries" );
	printf( usageString, "stats", "provides a menu to print out some predefined stats" );
	printf( usageString, "config", "a menu to configurate database variables" );
	printf( usageString, "help", "shows this menu" );
	printf( usageString, "q", "quits the program" );
	
	free( usageString );
}

/*
 * Command gives a menu of possible stats commands
 * User then choose the number corresponding to desired
 * stat, and program executes predefined 
 */
int print_stats( char *command, sqlite3 *database )
{
	int	stats_cnt = 0,
			len = -1,
			execution = -1,
			max_commands = print_stats_help();
	char	select[SELECT_LEN];
	do {
		/*	prompt user for command	*/
		len = prompt( command, "stats" );
		/*	print stat selected or exit	*/
		execution = atoi( command );
		if ( equals( command, "h" ) == 0 ) {
			print_stats_help();
		}
		else if ( equals( command, "e" ) == 0 ) {
			break;
		}
		else if (execution > 0 && execution <= max_commands) {
			if (execution == 1) {
				snprintf(select, SELECT_LEN, "select type, sum(amount) as Balance from %s group by type order by type", TABLE);
				regular_execute_sql(select);
				snprintf(select, SELECT_LEN, "select sum(amount) as Total from %s", TABLE);
				regular_execute_sql(select);
			}
			else if (execution == 2) {
				printf("Select month 1-12: ");
				len = strlen(fgets(command, COMMAND_LEN, stdin)); command[len-1] = '\0';
				int month = atoi(command);
				if (0 < month && month <= 12) {
					snprintf(select, SELECT_LEN, "select type, sum(amount) as Forbruk from %s where date > '%4s-%02d-00' and date < '%4s-%02d-32' group by type order by type",
					         TABLE, YEAR, month, YEAR, month);
					regular_execute_sql(select);
					snprintf(select, SELECT_LEN, "select sum(amount) as Total from %s where date > '%4s-%02d-00' and date < '%4s-%02d-32'", TABLE, YEAR, month, YEAR, month);
					regular_execute_sql(select);
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
					regular_execute_sql(select);
					snprintf(select, SELECT_LEN, "select sum(amount) as Total from %s where date < '%4s-%02d-32' and date > '%4s-%02d-00'", TABLE, YEAR, month, YEAR, month);
					regular_execute_sql(select);
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
					regular_execute_sql(select);
					while ((*months) != 0)	months++;
					months++;
				}
				snprintf(select, SELECT_LEN, "select sum(amount) as Total from %s", TABLE);
				regular_execute_sql(select);
			}
			stats_cnt ++;
		}
		else	printf("No statistics on '%s'\n", command);
	}	while (strncmp(command, "e", 2) != 0);
	return stats_cnt;
}

/*
 *	Prompts for and stores next command
 */
int prompt( char *command, char *command_text )
{
	printf( "aCBudget.%s > ", command_text );
	if ( !fgets( command, COMMAND_LEN, stdin ) ) { // EOT if 0

		free_all();
		exit(4);
		
	}
	
	int command_len = strlen( command );
	command[command_len-1] = '\0';
	return command_len;
}

/*
 *	Need a slightly different command_argument for the update command
 */
int prompt_update(char *command, char *command_text)
{
	printf( "aCBudget.update > %s: ", command_text );
	int command_len = strlen( fgets( command, COMMAND_LEN, stdin ) );
	command[command_len-1] = '\0';
	return command_len;
}

/*
 *	Tells program a file is to be read
 *  Prompts user for location of file.
 *	Then prompts user for filetype, implemented
 *	for files downloaded from DNB Norway (1) and
 *	Sparebanken Sør (2). It is also possible to
 *  continue reading from a pre-stored position
 *  of a file (3).
 */
int read_file( char *command, sqlite3 *database )
{
	int len, counter = 0;
	FILE *fp;
	char filename[50], type;
	filename[0] = '\0';
	
	len = prompt(filename, "filename");
	while ( ( equals( filename, "e" ) & equals( filename, "end" ) ) ) {
		
		fp = fopen( filename, "r" );
		if ( fp == 0 ) {
			// File could not be found
			printf( "'%s' does not exist\n", filename );
			len = prompt(filename, "filename");
			continue;
			
		}
		
		printf( "DNB (1), Sparebanken Sør (2) file or continue last (3) ? " );
		scanf( "%c", &type );
		clean_stdin();
		
		if ( type == '1' ) {
			// DNB file
			counter += read_DNB( fp, database, 0 );
			
		}
		else if ( type == '2' ) {
			// SBS file
			counter += read_SBS( fp, database, 0 );
			
		}
		else if ( type == '3' ) {
			// Continue last
			printf( "Last DNB (1) or Sparebanken Sør (2) file ? " );
			scanf( "%c", &type );
			clean_stdin();
			
			if ( type == '1' ) {
				// DNB file
				counter += read_DNB( fp, database, (*READ_COUNTER) );
				
			}
			else if ( type == '2' ) {
				// SBS file
				counter += read_SBS( fp, database, (*READ_COUNTER) );

			}
			else {
				// Illegal option
				printf("%c was not a valid file option.\n", type);
				
			}
			
		}
		else {
			// Illegal option
			printf("%c was not a valid option.\n", type);
			
		}
		
		fclose( fp );
		len = prompt( filename, "filename" );
	}
	
	return counter;
}

/*
 *	Main method to read DNB files.
 *	Reads file from start or until user ends at given checkpoints.
 *
 */
int read_DNB( FILE *fp, sqlite3 *database, int skip_counter )
{
	#if DEBUG
	fprintf( stderr, "read DNB\n" );
	#endif
	int insertion_counter = 0, error = 0, line_counter = 0;
	char singleInput,               //  input from user
		*correct,                   //  legal input values
		input[INPUT_LEN],			//	input lines
		insert_into[INSERT_LEN],	//	insert into statement
		*token;						//	tokens from input
	insert_t insert;				//	struct with all variables of an insert as members
	correct = calloc( 4, sizeof( char ) );
	strncpy( correct, "ynq", 3 );
	
	while ( fgets( input, INPUT_LEN, fp ) != NULL ) {
		
		line_counter++;
		
		if ( strlen( input ) <= 1 )	{
			//	empty line
			continue;
			
		}
		else if ( skip_counter > 0 ) {
			//	skip line 
			skip_counter--;
			continue;
			
		}
		
		/*
		 *	"Dato";"Forklaring";"Rentedato";"Uttak";"Innskudd"
		 *	"14.12.2014";"Morsom sparing kort avrunding Reservert transaksjon";"14.12.2014";"3,00";""
		 */
		#if DEBUG
		fprintf( stderr, "input: '%s'", input );
		#endif
		/*
		 *	Checks if number is on "Uttak" or "Innskudd".
		 *	If number is on "Uttak", token will end with ""\0 (end of input)
		 * But if number is on "Innskudd", token will include end number,
		 *	leaving error > 3, and thus gets converted with (-) at beginning
		 *	to get budgeted correctly when summing in database.
		 */
		token = strstr( input, "\"\"" );
		if ( token ) {
			// Should have commented during implementation
			error = strlen( token );
		}
		#if DEBUG
		fprintf( stderr, "split token: %s\n", token );
		#endif
		
		/*
		 *  Date of purchase in "Dato" field of input.
		 */
		token = strtok( input, "\";" );
		#if DEBUG
		fprintf( stderr, "date: %s\n", token );
		#endif
		copy_date( insert.date, token );
		
		/*
		 *	Copies "Forklaring" into comment,
		 *	for further analysis by user
		 */
		token = strtok( NULL, "\";" );
		#if DEBUG
		fprintf( stderr, "comment: %s\n", token );
		#endif
		strncpy( insert.comment, token, COMMENT_LEN );
		
		/*
		 *	Unimportant for my personal budget,
		 *	Date for interest rate
		 */
		token = strtok( NULL, "\";" );
		#if DEBUG
		fprintf( stderr, "Interest date: %s\n", token );
		#endif
		
		/*
		 *  Amount token
		 */
		token = strtok( NULL, "\";" );
		#if DEBUG
		fprintf( stderr, "amount: %s\n", token );
		#endif
		if ( error > 4 ) {
			//	Amount was deposited, decreasing money spent
			insert.amount[0] = '-';
			copy_number( 1, insert.amount, token );
			
		} else	{
			//	Amount is withdrawn, increasing money spent
			copy_number( 0, insert.amount, token );
			
		}

		// If test makes sure there actually is something to add.
		if ( strlen( insert.date ) > 0 && atol( insert.amount ) != 0 ) {
			/*
			 *	Prints out rows with equal date and/or amount
			 *	To check for double-entries.
			 */
			
			printf( "---Rows on same date:\n" );
			if ( ( singleInput = insertRequest( correct, insert, insert_into ) ) != 'y' ) {
				if (singleInput == 'n') {
#if DEBUG
					fprintf(stderr, "Values NOT added.\n");
#endif
				
				} else {
					// implicit if (singleInput == 'q')
					line_counter--;
					break;
				
				}
			}
			insertion_counter++;
		}
			
	}
	
	free( correct );
	(*READ_COUNTER) = line_counter;
	// Returns insertions made
	return insertion_counter;
	
}

/*
 *	Main method for reading files from Sparebanken Sør
 *	Almost same functionality as DNB method, with
 *	some tweaks to read correctly from a different filetype (.csv)
 */
int read_SBS( FILE *fp, sqlite3 *database, int skip_counter )
{
	#if DEBUG
	fprintf(stderr, "read SBS\n");
	#endif
	int insertion_counter = 0, line_counter = 0;
	char singleInput, *correct,
		input[INPUT_LEN],         //	input
		insert_into[INSERT_LEN],  //	insert into statemtn
		dateday[3],				  //	dd daydate
		datemonth[3],			  //	mm monthdate
		*token,					  //	tokens from input
		*datetoken;				  //	used to temporarily store date elements from token
	insert_t insert;
	correct = calloc( 4, sizeof( char ) );
	strncpy( correct, "ynq", 4 );

	while ( fgets( input, INPUT_LEN, fp ) != NULL )	{
		//	file has input

		line_counter++;
		
		if ( strlen( input ) < 2 ) {
			//	empty line
			continue;
		}
		else if ((*READ_COUNTER) > 0)	{
			//	skip line
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
		token = xstrtok( input, "	" );
		strncpy( insert.date, token, DATE_LEN );
		#if DEBUG
		fprintf(stderr, "Date: %s\n", token);
		#endif
		
		//	Second token is explanation (comment)
		token = xstrtok( NULL, "	" );
		#if DEBUG
		fprintf( stderr, "Comment: %s\n", token );
		#endif
		strncpy( insert.comment, token, COMMENT_LEN );

        /*
		 *	Explanation may contain date in beginning
		 *	which will be date of purchase (my budget date)
		 *	first datetoken is then the DAY
		 */
		datetoken = strtok( token, "." );
		if ( strlen( datetoken ) < 4 ) {
			//	Date was found, storing day
			strncpy( dateday, datetoken, 3 );
			datetoken = strtok(NULL, " ");

			//	Storing month
			strncpy( datemonth, datetoken, 3 );
			//	Replacing old date with correct format, importing YEAR from header
			snprintf( insert.date, DATE_LEN, "%s-%s-%s", YEAR, datemonth, dateday );
			
		} else {
			//	No date was found in explanation, prompts user for input
			datetoken = calloc( 1, sizeof( char ) * DATE_LEN );
			strncpy( datetoken, insert.date, DATE_LEN );
			copy_date( insert.date, datetoken );
			free( datetoken );
			
		}
		
		//	Last token is amount
		token = xstrtok(NULL, "	");
		if ( strlen( token ) > 1 ) {
			//	Number was a withdrewal, increasing budget spent
			copy_number( 0, insert.amount, token );
			
		} else {
			//	Number was a deposit, decreasing budget spent
			token = xstrtok(NULL, "	");
			insert.amount[0] = '-';
			copy_number( 1, insert.amount, token );
			
		}
		#if DEBUG
		fprintf( stderr, "amount: %s\n", token );
		#endif

		if ( strlen( insert.date ) > 0 && atol( insert.amount ) != 0 ) {
		
			//	All information gathered, check for equal date in database
			printf( "---Rows on same date:\n" );
			if ( singleInput = insertRequest( correct, insert, insert_into ) != 'y' ) {
				if (singleInput == 'n') {
#if DEBUG
					fprintf(stderr, "Values NOT added.\n");
#endif
				
				} else {
					// implicit if (singleInput == 'q')
					line_counter--;
					break;
				
				}
			}
			insertion_counter++;
		}
			
	}
	(*READ_COUNTER) = line_counter;
	return insertion_counter;	//	Insertions made
}

/*
 *	Lets user update amount column of existing rows, and the possibility of inserting a new row
 */
int update(char *command, sqlite3 *database)
{
	int updated = 0, sql_len, len;
	char commandhelp[COMMAND_LEN],	//	to store usage-help
		select[SELECT_LEN] ,	    //	used by sql-queries
		comment[COMMENT_LEN],	    //	to store comments
		type[TYPE_LEN],	            //	to store types
		amount[AMOUNT_LEN],	        //	to store amounts
		day[DAY_LEN], correct;	    //	helpful
	
	//	allocating space for id from rownumbers
	UNIQUE_ID = calloc( 1, sizeof( char ) * ID_LEN );
	P_COUNTER = calloc( 1, sizeof( uint32_t ) );

	printf( "Enter date 'dd' to update\n" );
	printf( "If month is wrong, type 'dd.mm' to check date 'dd' in month 'mm'\n" );
	printf( "Type 'e' or 'end' to quit\n" );
	
	while ( equals( command, "e" ) & equals( command, "end" ) ) {

		snprintf( commandhelp, COMMAND_LEN, "day in month (%s)", MONTH );	
		len = prompt_update( command, commandhelp );

		if ( !( equals( command, "e") & equals( command, "end" ) ) ) {
			//  User wants to e[nd]
			break;
		}
		else if ( ( !equals( command, "h" ) || !equals( command, "help" ) ) ) {
			//  User wants h[elp]
				printf( "Enter date 'dd' to update\n" );
				printf( "If month is wrong, type 'dd.mm' to check date 'dd' in month 'mm'\n" );
				printf( "Type 'e' or 'end' to quit\n" );
				continue;
			
		}
		else if ( ( len > 0 && command[1] == '.' ) || ( len > 1 && command[2] == '.' ) ) {
			//  User wants to change month in input
			int i = 1;
			while ( command[i] != '.' ) {
				//  Increase i until '.' is found
				i++;
				
			}
			command[i++] = 0; // split date.month
			if ( store_month ( &command[i] ) ) {
				//  Storing of month failed
				continue;
			}
			
		}
		else if ( ( strncmp(command, "month=", 6 ) == 0 ) &&
		          ( !store_month ( &command[6] ) )) {
			//  User wanted to change month, but month failed
			continue;
			
		}

		//	find entries based on day of month
		snprintf( select, SELECT_LEN,
		          "select comment, amount, type from %s where date = '%04d-%02d-%02d'",
		          TABLE, atoi( YEAR ), atoi( MONTH ), atoi( command ) );
		#if DEBUG
		fprintf( stdout, "Running select on %s: '%s'\n", DATABASE, select );
		#endif
		
		//	reset row count
		(*P_COUNTER) = 1;
		numbered_execute_sql( select );
		
		if (*P_COUNTER < 2) {
			//  No rows found
			printf( "No entries in given day (%s) of month (%s)\n", command, MONTH );
			continue;
		}

		//	Give user ability to divide/update entries
		snprintf( day, 3, "%02d", atoi( command ) );
		len = prompt_update( command, "number to update" );
		#if DEBUG
		fprintf( stderr, "day: '%s', command: '%s'\n", day, command );
		#endif

		len = atoi( command );
		
		if ( equals(command, "n" ) == 0 ) {
			//  User do not want to update any entries, continue
			continue;
			
		}
		else if ( ( ( len < 1 ) ) || ( len >= (*P_COUNTER) ) ) {
			//  command unequals "n\0", but user entered number outside range
			printf( "Invalid input: '%s', either pick number in range %d-%d or 'n' for none\n",
			        command, 1, (*P_COUNTER)-1 );
			continue;
			
		}
		
		//  User entered legal range and not 'n'
		(*P_COUNTER) = atoi( command );
			
		//	row selected for update
		#if DEBUG
		fprintf( stderr, "Finding rownumber %d (P_COUNTER: %d)\n", len, (*P_COUNTER) );
		#endif
		len = snprintf( select, SELECT_LEN,
		                "select id from %s where date = '%04d-%02d-%02d'",
		                TABLE, atoi( YEAR ), atoi( MONTH ), atoi( day ) );
			
		//	prepare statement to find correct row for update
		sqlite3_stmt **statement = calloc( 1, 1* sizeof( sqlite3_stmt* ) );
		if ( sqlite3_prepare_v2( database, select, len, statement, NULL ) != SQLITE_OK ) {
			//  Preparation of statement failed
			fprintf( stderr, "SQL prepare error from '%s': %s\nContact system creator\n",
			         select, zErrMsg );
			sqlite3_finalize( (*statement) );
			sqlite3_free( zErrMsg );
			break;
		}

		//  statement prepared
		len = (*P_COUNTER);
		while ( (*P_COUNTER) > 0 ) {
			//	while rownumber not met
			#if DEBUG
			fprintf( stderr, " %d", (*P_COUNTER) );
			#endif
			
			int sql_result = 0;
			do {
				//	keep trying to fetch next row
				sql_result = sqlite3_step( (*statement) );
				
			} while ( sql_result == SQLITE_BUSY );
			
			if ( sql_result == SQLITE_ROW )	{
				//	a row was found -> reduce P_COUNTER
				(*P_COUNTER)--;
				
			}
			else if ( sql_result == SQLITE_DONE ) {
				// number chosen too high
				printf( "\nNumber chosen (%d) was higher than number of rows returned (%d).\n",
				        len, len - (*P_COUNTER) );
				break;
			}
			else {
				//	unexpected behaviour
				if ( sql_result == SQLITE_MISUSE ) {
					fprintf( stderr, "\nSQL misuse: Contact system creator\n" );
					break;
					
				}
				else if ( sql_result == SQLITE_ERROR ) {
					fprintf( stderr, "\nSQL step-error: Contact system creator\n" );
					break;
					
				}
				//  Should not happen
				fprintf( stderr, "\nSQL critical-error: Contact system creator\n");
				continue;
			}
		}
		#if DEBUG
		fprintf( stderr, "\nCopying unique id into UNIQUE_ID\n" );
		#endif
		strncpy( UNIQUE_ID, sqlite3_column_text( (*statement), 0 ), ID_LEN );
			
		/*
		 *	Prompts user for input on all three values COMMENT, TYPE and AMOUNT.
		 *	Date is currently not available for update.
		 *	It is not neccesary to update all three, or anything at all,
		 *  the program only updates fields with input.
		 */
		sql_len = 0;	//	the total length of sql-command initialization
		sql_len += snprintf( select, SELECT_LEN, "update %s set", TABLE );
		(*P_COUNTER) = sql_len;
		
		len = prompt_update( comment, "comment" );
		if ( len > 1 ) {
			//	comment is to be updated
			sql_len += snprintf( select+sql_len, SELECT_LEN-sql_len,
			                    " comment = '%s'", comment );
			
		}
		
		len = prompt_update( type, "type" );
		if ( len > 1 ) {
			//	type is to be updated
			if ( sql_len > (*P_COUNTER) ) {
				//	need to add comma after previous variable
				*(select+sql_len++) = ',';
				*(select+sql_len) = '\0';
				
			}
			sql_len += snprintf( select+sql_len, SELECT_LEN-sql_len,
			                     " type = '%s'", type );
			
		}
		
		len = prompt_update( amount, "amount" );
		if ( len > 1 ) {
			//	amount is to be updated
			if ( sql_len > (*P_COUNTER) ) {
				//	need to add comma after previous variable
				*(select+sql_len++) = ',';
				*(select+sql_len) = '\0';
				
			}
			sql_len += snprintf( select+sql_len, SELECT_LEN-sql_len,
			                     " amount = %s", amount );
			
		}

		#if DEBUG
		fprintf(stderr, "comment, type, amount = '%s', '%s', '%s'\n%s\n",
		        comment, type, amount, select);
		#endif
		
		if ( sql_len > (*P_COUNTER) ) {
			//	at least one variable to update
			(*P_COUNTER) = sql_len;
			len = strlen( " where id = 'abc12'" );	//	length of rest
			sql_len += snprintf( select+sql_len, SELECT_LEN-sql_len,
			                     " where id = '%s';", UNIQUE_ID );
			if ( ( (*P_COUNTER)+len != sql_len ) &&
			     ( sqlite3_exec( database, select, NULL, 0, &zErrMsg ) != SQLITE_OK ) ) {
				//  Illegal range or execution failed
				fprintf( stderr, "%d =? %d || SQL error %s\n",
				         (*P_COUNTER)+len, sql_len, zErrMsg );
				sqlite3_free( zErrMsg );
				return -1;
				
			}
			//	row updated
			updated++;	
			#if DEBUG
			fprintf( stderr, "Row with id='%s' updated:\n%s\n", UNIQUE_ID, select );
			#endif
			
		}
		
		//	check if user wants to add another row
		printf( "Want to add another row with new type and same comment on same date?: " );
		scanf( "%c", &correct );
		clean_stdin();
		
		if ( correct == 'y' ) {
			//	add another row with new comment, type and amount on same date
			prompt_update( comment, "comment" );
			prompt_update( type, "type" );
			prompt_update( amount, "amount" );
			generate_id( UNIQUE_ID );
			
			//	insert into TABLE values (DATE, COMMENT, TYPE, AMOUNT, ID);
			snprintf( select, SELECT_LEN,
			          "insert into %s values('%04d-%02d-%02d', '%s', '%s', %s, '%s');",
			          TABLE, atoi( YEAR ), atoi( MONTH ), atoi( day ),
			          comment, type, amount, UNIQUE_ID );
			
			if ( !regular_execute_sql( select ) ) {
				//  SQL execution failed
				break;
				
			}
		}
		
		//	free update specific malloc
		sqlite3_finalize(*statement);
		free( statement );
		
	}
	
	free( P_COUNTER );
	free( UNIQUE_ID );
	return updated;
	
}
