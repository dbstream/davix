/**
 * (v)snprintf implementation.
 * Copyright (C) 2024  dbstream
 */
#include <davix/snprintf.h>
#include <davix/stddef.h>
#include <davix/string.h>

struct fmtspec {
	int width;
	int precision;
	unsigned int type;
	unsigned int flags;
	unsigned int size;
	union {
		char char_val;
		long long signed_val;
		unsigned long long unsigned_val;
		void *ptr_val;
	};
};

#define PR_LEFT	 0x01
#define PR_PLUS	 0x02
#define PR_SPACE	0x04
#define PR_ZERO	 0x08
#define PR_SPECIAL      0x10
#define PR_UPPER	0x20

static char *
do_unsigned(char *o, char *end, struct fmtspec *spec, int base)
{
	const char *alpha = (spec->flags & PR_UPPER)
		? "0123456789ABCDEF"
		: "0123456789abcdef";

	char buf[64];
	int i = 0;
	do {
		buf[i++] = alpha[spec->unsigned_val % base];
		spec->unsigned_val /= base;
	} while(spec->unsigned_val);

	spec->width -= i;

	if(spec->flags & PR_SPECIAL) {
		if(base == 8)
			spec->width--;
		else if(base == 16)
			spec->width -= 2;
	}

	if(!(spec->flags & (PR_ZERO | PR_LEFT))) {
		while(spec->width > 0) {
			if(o < end)
				*o = ' ';
			o++;
			spec->width--;
		}
	}

	if(spec->flags & PR_SPECIAL) {
		if(base == 8 || base == 16) {
			if(o < end)
				*o = '0';
			o++;
			if(base == 16) {
				if(o < end)
					*o = (spec->flags & PR_UPPER)
						? 'X'
						: 'x';
				o++;
			}
		}
	}

	if(spec->flags & PR_ZERO) {
		while(spec->width > 0) {
			if(o < end)
				*o = '0';
			o++;
			spec->width--;
		}
	}

	do {
		i--;
		if(o < end)
			*o = buf[i];
		o++;
	} while(i);

	if(spec->flags & PR_LEFT) {
		while(spec->width > 0) {
			if(o < end)
				*o = ' ';
			o++;
			spec->width--;
		}
	}

	return o;
}

static char *
do_signed(char *o, char *end, struct fmtspec *spec)
{
	char buf[64];
	unsigned long long x;
	unsigned long i = 0;
	char sign = 0;
	if(spec->signed_val < 0) {
		sign = '-';
		x = -spec->signed_val;
	} else {
		x = spec->signed_val;
	}

	if(!sign) {
		if(spec->flags & PR_PLUS)
			sign = '+';
		else if(spec->flags & PR_SPACE)
			sign = ' ';
	}

	do {
		buf[i++] = '0' + (x % 10);
		x /= 10;
	} while(x);

	spec->width -= i;
	if(sign)
		spec->width--;

	if(!(spec->flags & (PR_ZERO | PR_LEFT))) {
		while(spec->width > 0) {
			if(o < end)
				*o = ' ';
			o++;
			spec->width--;
		}
	}

	if(sign) {
		if(o < end)
			*o = sign;
		o++;
	}

	if(spec->flags & PR_ZERO) {
		while(spec->width > 0) {
			if(o < end)
				*o = '0';
			o++;
			spec->width--;
		}
	}

	do {
		i--;
		if(o < end)
			*o = buf[i];
		o++;
	} while(i);

	if(spec->flags & PR_LEFT) {
		while(spec->width > 0) {
			if(o < end)
				*o = ' ';
			o++;
			spec->width--;
		}
	}

	return o;
}

static char *
do_char(char *o, char *end, struct fmtspec *spec)
{
	if(o < end)
		*o = spec->char_val;
	o++;
	return o;
}

static char *
do_string(char *o, char *end, struct fmtspec *spec)
{
	if(!spec->ptr_val)
		spec->ptr_val = "(null)";

	unsigned long n;
	if(spec->precision)
		n = strnlen(spec->ptr_val, spec->precision);
	else
		n = strlen(spec->ptr_val);

	spec->width -= n;
	int pad = 0;
	if(spec->width > 0)
		pad = spec->width;

	if(!(spec->flags & PR_LEFT)) {
		if(o < end) {
			if(end - o < pad)
				memset(o, ' ', end - o);
			else
				memset(o, ' ', pad);
		}
		o += pad;
	}

	if(o < end) {
		if(end - o < (ssize_t) n)
			memcpy(o, spec->ptr_val, end - o);
		else
			memcpy(o, spec->ptr_val, n);
	}
	o += n;

	if(spec->flags & PR_LEFT) {
		if(o < end) {
			if(end - o < pad)
				memset(o, ' ', end - o);
			else
				memset(o, ' ', pad);
		}
		o += pad;
	}

	return o;
}

static char *
print_specifier(char *o, char *end, char c, struct fmtspec *spec)
{
	switch(c) {
	case 'X':
		spec->flags |= PR_UPPER;
		__attribute__((fallthrough));
	case 'x':
	case 'p':
		return do_unsigned(o, end, spec, 16);
	case 'u':
		return do_unsigned(o, end, spec, 10);
	case 'o':
		return do_unsigned(o, end, spec, 8);
	case 'd':
	case 'i':
		return do_signed(o, end, spec);
	case 'c':
		return do_char(o, end ,spec);
	case 's':
		return do_string(o, end, spec);
	default:
		return NULL;
	}
}

#define PR_CHAR		0
#define PR_SHORT	1
#define PR_INT		2
#define PR_LONG		3
#define PR_LONGLONG	4
#define PR_INTMAX	5
#define PR_SIZE		6
#define PR_INTPTR	7

#define FMT_NULL	0
#define FMT_COPY	1
#define FMT_WIDTH	2
#define FMT_PRECISION	3
#define FMT_POINTER	4
#define FMT_UCHAR	5
#define FMT_USHORT	6
#define FMT_UINT	7
#define FMT_ULONG	8
#define FMT_ULONGLONG	9
#define FMT_UINTMAX	10
#define FMT_USIZE	11
#define FMT_UINTPTR	12
#define FMT_SCHAR	13
#define FMT_SSHORT	14
#define FMT_SINT	15
#define FMT_SLONG	16
#define FMT_SLONGLONG	17
#define FMT_SSIZE	18
#define FMT_SINTPTR	19
#define FMT_ESCAPED	20
#define FMT_BAD		21
#define FMT_CHAR	22
#define FMT_SINTMAX	23

static int
parse_fmt_int(const char *pos, const char **end)
{
	int x = 0;
	do {
		x *= 10;
		x += *(pos++) - '0';
	} while(*pos >= '0' && *pos <= '9');
	*end = pos;
	return x;
}

static unsigned long
parse_fmt(const char *fmt, struct fmtspec *spec, char **o, char *end)
{
	const char *pos = fmt;

	/*
	 * Are we parsing an escaped '%'?
	 */
	if(spec->type == FMT_ESCAPED) {
		spec->type = FMT_COPY;
		return 1;
	}

	/*
	 * Are we starting over?
	 */
	if(spec->type == FMT_COPY || spec->type == FMT_NULL) {
		if(*fmt != '%') {
			/*
			 * Copy non-format characters.
			 */
			spec->type = FMT_COPY;
			do {
				pos++;
			} while(*pos && *pos != '%');
			return pos - fmt;
		}

		/*
		 * We are at the start of a new format specifier.
		 */

		pos++;
		if(!*pos) {
			/*
			 * Bad format string: reached end of string.
			 */
			spec->type = FMT_BAD;
			return pos - fmt;
		}

		/*
		 * This is a double-percentage ('%%').
		 */
		if(*pos == '%') {
			spec->type = FMT_ESCAPED;
			return pos - fmt;
		}

		/*
		 * Parse '%p' early, as it is a bit special.
		 */
		if(*pos == 'p') {
			spec->type = FMT_POINTER;
			return pos - fmt;
		}

		/*
		 * Parse the flags here.
		 */
		spec->flags = 0;
		for(;;) {
			unsigned int new_flag = 0;
			if(*pos == '-')
				new_flag = PR_LEFT;
			else if(*pos == '+')
				new_flag = PR_PLUS;
			else if(*pos == ' ')
				new_flag = PR_SPACE;
			else if(*pos == '0')
				new_flag = PR_ZERO;
			else if(*pos == '#')
				new_flag = PR_SPECIAL;
			if(!new_flag)
				break;
			if(spec->flags & new_flag) {
				/*
				 * Bad format string: duplicate flag.
				 */
				spec->type = FMT_BAD;
				return pos - fmt;
			}

			pos++;
			spec->flags |= new_flag;
		}

		/*
		 * Parse the field width here. Set spec->type unconditionally
		 * so we enter the precision parsing below no matter what.
		 */
		spec->type = FMT_WIDTH;
		spec->width = 0;
		if(*pos == '*') {
			pos++;
			return pos - fmt;
		} else if(*pos >= '1' && *pos <= '9') {
			spec->width = parse_fmt_int(pos, &pos);
		}
	}

	if(spec->type == FMT_WIDTH) {
		/*
		 * Parse the field precision here. Set spec->type
		 * unconditionally so we enter the size parsing below no matter
		 * what.
		 */
		spec->type = FMT_PRECISION;
		spec->precision = 0;
		if(*pos == '.') {
			pos++;
			if(*pos == '*') {
				pos++;
				return pos - fmt;
			} else if(*pos >= '1' && *pos <= '9') {
				spec->precision = parse_fmt_int(pos, &pos);
			} else {
				spec->type = FMT_BAD;
				return pos - fmt;
			}
		}
	}

	if(spec->type == FMT_PRECISION) {
		/*
		 * Parse the field size here.
		 */
		if(*pos == 'h') {
			pos++;
			if(*pos == 'h') {
				pos++;
				spec->size = PR_CHAR;
			} else {
				spec->size = PR_SHORT;
			}
		} else if(*pos == 'l') {
			pos++;
			if(*pos == 'l') {
				pos++;
				spec->size = PR_LONGLONG;
			} else {
				spec->size = PR_LONG;
			}
		} else if(*pos == 'j') {
			pos++;
			spec->size = PR_INTMAX;
		} else if(*pos == 'z') {
			pos++;
			spec->size = PR_SIZE;
		} else if(*pos == 't') {
			pos++;
			spec->size = PR_INTPTR;
		} else {
			spec->size = PR_INT;
		}

		/*
		 * Parse the format specifier here.
		 */
		switch(*pos) {
		case 'c':
			spec->type = FMT_CHAR;
			break;
		case 's':
			spec->type = FMT_POINTER;
			break;
		case 'd':
		case 'i':
			if(spec->size == PR_CHAR) {
				spec->type = FMT_SCHAR;
			} else if(spec->size == PR_SHORT) {
				spec->type = FMT_SSHORT;
			} else if(spec->size == PR_LONG) {
				spec->type = FMT_SLONG;
			} else if(spec->size == PR_LONGLONG) {
				spec->type = FMT_SLONGLONG;
			} else if(spec->size == PR_INTMAX) {
				spec->type = FMT_SINTMAX;
			} else if(spec->size == PR_SIZE) {
				spec->type = FMT_SSIZE;
			} else if(spec->size == PR_INTPTR) {
				spec->type = FMT_SINTPTR;
			} else {
				spec->type = FMT_SINT;
			}
			break;
		case 'u':
		case 'x':
		case 'X':
			if(spec->size == PR_CHAR) {
				spec->type = FMT_UCHAR;
			} else if(spec->size == PR_SHORT) {
				spec->type = FMT_USHORT;
			} else if(spec->size == PR_LONG) {
				spec->type = FMT_ULONG;
			} else if(spec->size == PR_LONGLONG) {
				spec->type = FMT_ULONGLONG;
			} else if(spec->size == PR_INTMAX) {
				spec->type = FMT_UINTMAX;
			} else if(spec->size == PR_SIZE) {
				spec->type = FMT_USIZE;
			} else if(spec->size == PR_INTPTR) {
				spec->type = FMT_UINTPTR;
			} else {
				spec->type = FMT_UINT;
			}
			break;
		default:
			spec->type = FMT_BAD;
			break;
		}

		return pos - fmt;
	}

	if(*pos == 'p') {
		spec->size = PR_INTPTR;
		spec->flags = PR_ZERO | PR_SPECIAL;
		spec->width = 18;
		spec->precision = 0;
	}

	char *pr_end = print_specifier(*o, end, *(pos++), spec);
	if(pr_end) {
		*o = pr_end;
		spec->type = FMT_NULL;
	} else {		/* This should not be possible. */
		spec->type = FMT_BAD;
	}
	return pos - fmt;
}

int
vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args)
{
	char *o = buf;
	char *end = buf + size;
	int bad = 0;

	/*
	 * We do pointer comparison between o and end. We still want to function
	 * if someone does something silly like snprintf(buf, -1), so end needs
	 * to be above o.
	 */
	if((unsigned long) end < (unsigned long) o)
		end = (char *) -1UL;

	struct fmtspec spec = { .type = FMT_NULL };

	while(*fmt) {
		unsigned long n = parse_fmt(fmt, &spec, &o, end);

		if(spec.type == FMT_BAD) {
			bad = 1;
			break;
		}

		if(spec.type == FMT_NULL || spec.type == FMT_ESCAPED) {
			/* Skip the specified number of characters. */
			fmt += n;
			continue;
		}

		if(spec.type == FMT_COPY) {
			if(o < end) {
				long m = n;
				if(end - o < m)
					m = end - o;
				memcpy(o, fmt, m);
			}

			fmt += n;
			o += n;
			continue;
		}

		fmt += n;
		switch(spec.type) {
		case FMT_WIDTH:
			spec.width = va_arg(args, int);
			break;
		case FMT_PRECISION:
			spec.precision = va_arg(args, int);
			break;
		case FMT_CHAR:
			spec.char_val = va_arg(args, int);
			break;
		case FMT_POINTER:
			spec.ptr_val = va_arg(args, void *);
			break;
		case FMT_SCHAR:
		case FMT_SSHORT:
		case FMT_SINT:
			spec.signed_val = va_arg(args, int);
			break;
		case FMT_SLONG:
			spec.signed_val = va_arg(args, long);
			break;
		case FMT_SLONGLONG:
			spec.signed_val = va_arg(args, long long);
			break;
		case FMT_SINTMAX:
			spec.signed_val = va_arg(args, long long);
			break;
		case FMT_SSIZE:
			spec.signed_val = va_arg(args, long);
			break;
		case FMT_SINTPTR:
			spec.signed_val = va_arg(args, long);
			break;
		case FMT_UCHAR:
		case FMT_USHORT:
			spec.unsigned_val = va_arg(args, int);
			break;
		case FMT_UINT:
			spec.unsigned_val = va_arg(args, unsigned int);
			break;
		case FMT_ULONG:
			spec.unsigned_val = va_arg(args, unsigned long);
			break;
		case FMT_ULONGLONG:
			spec.unsigned_val = va_arg(args, unsigned long long);
			break;
		case FMT_UINTMAX:
			spec.unsigned_val = va_arg(args, unsigned long long);
			break;
		case FMT_USIZE:
			spec.unsigned_val = va_arg(args, unsigned long);
			break;
		case FMT_UINTPTR:
			spec.unsigned_val = va_arg(args, unsigned long);
			break;
		default:
			bad = 1;
			break;
		}

		if(bad)
			break;
	}

	/*
	 * Null-terminate the string so users who don't check are safe.
	 */
	if(o < end)
		*o = 0;
	else if(size)
		end[-1] = 0;
	if(bad)
		return -1;
	return o - buf;
}

int
snprintf (char *buf, unsigned long size, const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start (args, fmt);
	rv = vsnprintf (buf, size, fmt, args);
	va_end (args);

	return rv;
}
