#include "acbudget.h"

/**
 *	To use instead of fflush(stdin)
 */
void clean_stdin( void )
{
    char *dummy = calloc(1, sizeof(char) * 2);
    fgets(dummy, 2, stdin);
    free(dummy);
}

/**
 *	Numbers in both files are given with
 *	"," where it should be "." in database.
 *	This method converts number to remove
 *	 "." from original and replace "," with "." in
 *	code.
 */
void copy_number( int error, char* to, char* from )
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
	#if DEBUG
	fprintf(stderr, "token.amount=%s vs %s=sql.amount\n", from,to);
	#endif
}

/**
 *	Date are reversed and wrong in files from DNB
 *	This method stores date correctly in *date pointer
 */
char* copy_date( char *date, char *token )
{

    #if DEBUG
	fprintf( stderr, "token.date=%s\n", token );
	#endif
	int i;
	for ( i = 0; i< 4; i++ )	date[i] = ( int ) token[6+i];
	
	date[i++] = '-';
	date[i++] = ( int ) token[3];
	date[i++] = ( int ) token[4];
	date[i++] = '-';
	date[i++] = ( int ) token[0];
	date[i++] = ( int ) token[1];
	date[i++] = '\0';
	#if DEBUG
	fprintf( stderr, "regnskap.date=%s\n", date );
	#endif
	return date;
	
}


/**
 * Checks if input is one of either correct values
 */
char correct_input( char in, char *correct )
{

	while ( (*correct) != 0 ) {
		if ( (*correct) == in ) {
			return in;
		}
		correct++;
	}
	return 0;
	
}

/**
 * Takes a string and checks it against the second
 * string for as long as the second string is.
 * The 0-terminator is therefore included in the
 * check to be sure not a start of any word is
 * matched to a abbreviated command.
 * Example user types 'button' which would be
 * correct for '2' tokens against 'bu' - unless
 * the 0-termination is tested as well.
 */
int equals( char *challenger, char *matcher )
{
	
	int len = strlen( matcher ) + 1; // +1 is to include the 0-terminator
	#ifdef DEBUG
	fprintf( stderr, "strncmp( %s, %s, %d ) = %d\n", challenger, matcher, len, strncmp( challenger, matcher, len ) );
	#endif
	return strncmp( challenger, matcher, len);
	
}


/*
 *	Frees all allocated pre-defined pointers if needed.
 */
int free_all( void )
{

    #if DEBUG
	fprintf( stderr, "Trying to free...\n" );
	#endif
	if	( DATABASE != NULL ) {	
		#if DEBUG
		fprintf( stderr, "DATABASE..." );
		#endif
		free( DATABASE );
		#ifdef DEBUG
		fprintf( stderr, "freed\n" );
		#endif
	}
	if	( TABLE != NULL ) {	
		#if DEBUG
		fprintf( stderr, "TABLE..." );
		#endif
		free( TABLE );
		#ifdef DEBUG
		fprintf( stderr, "freed\n" );
		#endif
	}
	if	( MONTH != NULL ) {
		#if DEBUG
		fprintf( stderr, "MONTH..." );
		#endif
		free( MONTH );
		#ifdef DEBUG
		fprintf( stderr, "freed\n" );
		#endif
	}
	if	( YEAR != NULL ) {
		#if DEBUG
		fprintf( stderr, "YEAR..." );
		#endif
		free( YEAR );
		#ifdef DEBUG
		fprintf( stderr, "freed\n" );
		#endif
	}
	if	( BACKUP_FILENAME != NULL ) {
		#if DEBUG
		fprintf( stderr, "BACKUP_FILENAME..." );
		#endif
		free( BACKUP_FILENAME );
		#ifdef DEBUG
		fprintf( stderr, "freed\n" );
		#endif
	}
	if ( READ_COUNTER ) {
		free( READ_COUNTER );
	}
	#if DEBUG
	fprintf( stderr, "All non-null variables freed!\n" );
	#endif
	return 0;
}

/**
 * Used to generate unique ID-token for use in database.
 * The possible letters are stored in 'letters' variable,
 * and uses pseudorandom 'rand()' to fetch next letter
 * modulo.
 */
void generate_id( char *id )
{
	char letters[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i, len = strlen( letters );
	time_t t;
	srand( ( unsigned ) time( &t ) ); // Seed

	// Generation
	for ( i=0;i<ID_LEN-1; i++ ) {
		id[i] = letters[ rand() % len ];
	}
	id[ID_LEN-1] = '\0';
}

/**
 *	Prints out the possible commands within "stats" command.
 */
int print_stats_help( void )
{
	
	int cnt_commands = 0;
	printf("---Printable statistics---\n");
	printf("%-2d: Number of types within year %4s with corresponding sums\n", ++cnt_commands, YEAR);
	printf("%-2d: Number of types within a month (prompted) with corresponding sums\n", ++cnt_commands);
	printf("%-2d: All transactions within a month (prompted)\n", ++cnt_commands);
	printf("%-2d: All months with balances\n", ++cnt_commands);
	printf("%-2c: Prints this screen\n", 'h');
	printf("%-2c: exit\n", 'e');
	return cnt_commands;
	
}

int store_month( char *string_month )
{
	int month = strtol( string_month, 0, 10 );
	if ( month > 0 && month < 13 ) {
		//  Month within legal range (1-12)
		snprintf(MONTH, 3, "%02d", month);
		return 0;
	}
	printf("Month outside range 1-12: %d\n", month);
	return 1;
		
}

/**
 * strtok version that handles null fields
 */
char *xstrtok( char *line, char *delims )
{
	static char *saveline = NULL;
	char *p;
	int n;

	if( line != NULL )
	   saveline = line;

	/*
	*see if we have reached the end of the line 
	*/
	if( saveline == NULL || *saveline == '\0' ) {
            return(NULL);
	}
	
	/*
	*return the number of characters that aren't delims 
	*/
	n = strcspn( saveline, delims );
	p = saveline; /*save start of this token*/

	saveline += n; /*bump past the delim*/

	if( (*saveline) != '\0') /*trash the delim if necessary*/
		(*(saveline++)) = '\0';
	return(p);
}

