#include "acbudget.h"

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
