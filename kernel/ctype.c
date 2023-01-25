/* SPDX-License-Identifier: MIT */
#include <davix/ctype.h>

#define ALNUM (1 << 0)
#define ALPHA (1 << 1)
#define CNTRL (1 << 2)
#define DIGIT (1 << 3)
#define GRAPH (1 << 4)
#define LOWER (1 << 5)
#define PRINT (1 << 6)
#define PUNCT (1 << 7)
#define SPACE (1 << 8)
#define UPPER (1 << 9)
#define XDIGIT (1 << 10)

static unsigned short ctype_array[128] = {
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL | SPACE,
	CNTRL | SPACE,
	CNTRL | SPACE,
	CNTRL | SPACE,
	CNTRL | SPACE,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	CNTRL,
	PRINT | SPACE,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	ALNUM | DIGIT | GRAPH | PRINT | XDIGIT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER | XDIGIT,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER | XDIGIT,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER | XDIGIT,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER | XDIGIT,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER | XDIGIT,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER | XDIGIT,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	ALNUM | ALPHA | GRAPH | PRINT | UPPER,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT | XDIGIT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT | XDIGIT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT | XDIGIT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT | XDIGIT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT | XDIGIT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT | XDIGIT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	ALNUM | ALPHA | GRAPH | LOWER | PRINT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	GRAPH | PRINT | PUNCT,
	CNTRL
};

int isalnum(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & ALNUM) ? 1 : 0;
}

int isalpha(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & ALPHA) ? 1 : 0;
}

int iscntrl(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & CNTRL) ? 1 : 0;
}

int isdigit(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & DIGIT) ? 1 : 0;
}

int isgraph(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & GRAPH) ? 1 : 0;
}

int islower(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & LOWER) ? 1 : 0;
}

int isprint(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & PRINT) ? 1 : 0;
}

int ispunct(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & PUNCT) ? 1 : 0;
}

int isspace(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & SPACE) ? 1 : 0;
}

int isupper(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & UPPER) ? 1 : 0;
}

int isxdigit(int c)
{
	if(c < 0 || c > 127)
		return 0;
	return (ctype_array[c] & XDIGIT) ? 1 : 0;
}

int tolower(int c)
{
	if(!isupper(c))
		return c;
	return c + 32;
}

int toupper(int c)
{
	if(!islower(c))
		return c;
	return c - 32;
}
