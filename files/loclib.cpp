#if defined(_MSC_VER)
#include <conio.h>  /* if your compiler does not
					   support this  header file,
					   remove it */
#endif

#include <cstdio>
#include <cstdlib>
#include "globals.h"
#include "loclib.h"
#include "parser.h"

					   /* Get a character from console. (Use getchar() if
						  your compiler does not support       _getche().) */
int call_getche(void)
{
	char ch;
#if defined(_QC)
	ch = (char)getche();
#elif defined(_MSC_VER)
	ch = (char)_getche();
#else
	ch = (char)getchar();
#endif
	while (*prog != ')') prog++;
	prog++;   /* advance to end of line */
	return ch;
}

/* Put a character to the display. */
int call_putch(void)
{
	int value;

	eval_exp(&value, 1);
	printf("%c", value);
	return value;
}

/* Call puts(). */
int call_puts(void)
{
	get_token();
	if (*token != '(') sntx_err(PAREN_EXPECTED);
	get_token();
	if (token_type != STRING) sntx_err(QUOTE_EXPECTED);
	puts(token);
	get_token();
	if (*token != ')') sntx_err(PAREN_EXPECTED);

	get_token();
	if (*token != ';') sntx_err(SEMI_EXPECTED);
	putback();
	return 0;
}

/* A built-in console output function. */
int print(void)
{
	int i;

	get_token();
	if (*token != '(')  sntx_err(PAREN_EXPECTED);

	get_token();
	if (token_type == STRING) { /* output a string */
		printf("%s ", token);
	}
	else {  /* output a number */
		putback();
		eval_exp(&i, 1);
		printf("%d ", i);
	}

	get_token();

	if (*token != ')') sntx_err(PAREN_EXPECTED);

	get_token();
	if (*token != ';') sntx_err(SEMI_EXPECTED);
	putback();
	return 0;
}

/* Read an integer from the keyboard. */
int getnum(void)
{
	char s[80];

	if (fgets(s, sizeof(s), stdin) != NULL) {
		while (*prog != ')') prog++;
		prog++;  /* advance to end of line */
		return atoi(s);
	}
	else {
		return 0;
	}
}
