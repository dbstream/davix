/**
 * vsnprintf.
 * Copyright (C) 2025-present  dbstream
 */
#include <dsl/minmax.h>
#include <vsnprintf.h>
#include <stdint.h>
#include <string.h>

struct snprintf_output {
	char *pos;
	size_t len;
};

static inline void
put_single_char (snprintf_output &o, char c)
{
	if (o.len) {
		*(o.pos++) = c;
		o.len--;
	}
}

static inline void
rep_single_char (snprintf_output &o, char c, size_t n)
{
	n = dsl::min (n, o.len);
	memset (o.pos, c, n);
	o.pos += n;
	o.len -= n;
}

static inline void
put_string (snprintf_output &o, const char *s, size_t n)
{
	n = dsl::min (n, o.len);
	memcpy (o.pos, s, n);
	o.pos += n;
	o.len -= n;
}

static inline void
put_string_auto (snprintf_output &o, const char *s)
{
	put_string (o, s, strlen (s));
}

struct fmtspec {
	int width;
	int precision;
	unsigned int flags;
};

enum {
	PR_LEFT = 0x01U,
	PR_PLUS = 0x02U,
	PR_SPACE = 0x04U,
	PR_ZERO = 0x08U,
	PR_SPECIAL = 0x10U,
	PR_UPPER = 0x20U
};

template<int base>
static void
do_unsigned (snprintf_output &o, fmtspec &spec, uintmax_t value)
{
	const char *alpha = (spec.flags & PR_UPPER)
			? "0123456789ABCDEF"
			: "0123456789abcdef";

	static constexpr int buflen = 32;
	char buf[buflen];
	int i = buflen;
	do {
		buf[--i] = alpha[value % base];
		value /= base;
	} while (value);
	spec.width -= buflen - i;

	if (spec.flags & PR_SPECIAL) {
		if (base == 8)
			spec.width--;
		else if (base == 16)
			spec.width -= 2;
	}

	if (!(spec.flags & (PR_ZERO | PR_LEFT)) && spec.width > 0)
		rep_single_char (o, ' ', spec.width);

	if (spec.flags & PR_SPECIAL) {
		if (base == 8 || base == 16) {
			put_single_char (o, '0');
			if (base == 16)
				put_single_char (o, (spec.flags & PR_UPPER)
						? 'X'
						: 'x');
		}
	}

	if ((spec.flags & PR_ZERO) && spec.width > 0) {
		rep_single_char (o, '0', spec.width);
		spec.width = 0;
	}

	put_string (o, &buf[i], buflen - i);

	if ((spec.flags & PR_LEFT) && spec.width > 0)
		rep_single_char (o, ' ', spec.width);
}

static void
do_signed (snprintf_output &o, fmtspec &spec, intmax_t value)
{
	char sign = 0;
	if (spec.flags & PR_PLUS)
		sign = '+';
	else if (spec.flags & PR_SPACE)
		sign = ' ';

	uintmax_t x;
	if (value < 0) {
		sign = '-';
		/** Avoid overflow when value == INTxx_MIN.  */
		x = -(value + 1) + 1;
	} else {
		x = value;
	}

	static constexpr int buflen = 32;
	char buf[buflen];
	int i = buflen;
	do {
		buf[--i] = '0' + (x % 10);
		x /= 10;
	} while (x);
	spec.width -= buflen - i;
	if (sign)
		spec.width--;

	if (!(spec.flags & (PR_ZERO | PR_LEFT)) && spec.width > 0)
		rep_single_char (o, ' ', spec.width);

	if (sign)
		put_single_char (o, sign);

	if ((spec.flags & PR_ZERO) && spec.width > 0) {
		rep_single_char (o, '0', spec.width);
		spec.width = 0;
	}

	put_string (o, &buf[i], buflen - i);

	if ((spec.flags & PR_LEFT) && spec.width > 0)
		rep_single_char (o, ' ', spec.width);
}

static void
do_string (snprintf_output &o, fmtspec &spec, const char *s)
{
	if (!s)
		s = "(nullptr)";

	size_t n = strnlen (s, spec.precision ? spec.precision : 1024);
	spec.width -= n;

	if (!(spec.flags & PR_LEFT) && spec.width > 0)
		rep_single_char (o, ' ', spec.width);

	put_string (o, s, n);

	if ((spec.flags & PR_LEFT) && spec.width > 0)
		rep_single_char (o, ' ', spec.width);
}

static inline bool
try_consume (const char *&fmt, char c)
{
	if (*fmt == c) {
		fmt++;
		return true;
	}

	return false;
}

static inline int
dumb_atoi (const char *&fmt)
{
	int x = *(fmt++) - '0';
	for (int i = 0; i < 4; i++) {
		if (*fmt < '0' || '9' < *fmt)
			return x;
		x *= 10;
		x += *(fmt++) - '0';
	}
	return 0;
}

extern "C" void
vsnprintf (char *buf, size_t size, const char *fmt, va_list args)
{
	if (!size)
		return;

	snprintf_output o = {
		.pos = buf,
		.len = size - 1 /** Leave room for the terminating null.  */
	};

	for (;;) {
		const char *p = strchrnul (fmt, '%');
		put_string (o, fmt, p - fmt);
		if (!*p)
			break;

		fmt += 1 + p - fmt;
		fmtspec spec;
		spec.width = 0;
		spec.precision = 0;
		spec.flags = 0;
		for (;;) {
			unsigned int flag = 0;
			if (try_consume (fmt, '-'))
				flag = PR_LEFT;
			else if (try_consume (fmt, '+'))
				flag = PR_PLUS;
			else if (try_consume (fmt, ' '))
				flag = PR_SPACE;
			else if (try_consume (fmt, '0'))
				flag = PR_ZERO;
			else if (try_consume (fmt, '#'))
				flag = PR_SPECIAL;
			else
				break;
			if (spec.flags & flag) {
				put_string_auto (o, "(bad format string)");
				*o.pos = 0;
				return;
			}
			spec.flags |= flag;
		}

		if (try_consume (fmt, '*')) {
			spec.width = va_arg (args, int);
			if (spec.width < -1024 || spec.precision > 1024) {
				put_string_auto (o, "(bad format parameters)");
				*o.pos = 0;
				return;
			} else if (spec.width < 0) {
				spec.width = -spec.width;
				spec.flags |= PR_LEFT;
			}
		} else if ('1' <= *fmt && *fmt <= '9') {
			spec.width = dumb_atoi (fmt);
			if (!spec.width) {
				put_string_auto (o, "(bad format string)");
				*o.pos = 0;
				return;
			}
		}

		if (try_consume (fmt, '.')) {
			if (try_consume (fmt, '*')) {
				spec.precision = va_arg (args, int);
				if (spec.precision < 0 || spec.precision > 1024) {
					put_string_auto (o, "(bad format parameters)");
					*o.pos = 0;
					return;
				}
			} else if ('1' <= *fmt && *fmt <= '9') {
				spec.precision = dumb_atoi (fmt);
				if (!spec.precision) {
					put_string_auto (o, "(bad format string)");
					*o.pos = 0;
					return;
				}
			} else {
				put_string_auto (o, "(bad format string)");
				*o.pos = 0;
				return;
			}
		}

		int size = 2;
		if (try_consume (fmt, 'h')) {
			size--;
			if (try_consume (fmt, 'h'))
				size--;
		} else if (try_consume (fmt, 'l')) {
			size++;
			if (try_consume (fmt, 'l'))
				size++;
		} else if (try_consume (fmt, 'j'))
			size = 5;	/** intmax_t  */
		else if (try_consume (fmt, 'z'))
			size = 6;	/** size_t  */
		else if (try_consume (fmt, 't'))
			size = 7;	/** intptr_t  */

		switch (*(fmt++)) {
		case '%':
			/** an escaped '%' */
			put_single_char (o, '%');
			break;
		case 'c':
			put_single_char (o, va_arg (args, int));
			break;
		case 's':
			do_string (o, spec, va_arg (args, const char *));
			break;
		case 'd':
		case 'i':
			if (size == 0 || size == 1 || size == 2)
				do_signed (o, spec, va_arg (args, int));
			else if (size == 3)
				do_signed (o, spec, va_arg (args, long));
			else if (size == 4)
				do_signed (o, spec, va_arg (args, long long));
			else if (size == 5)
				do_signed (o, spec, va_arg (args, intmax_t));
			else if (size == 6)
				do_signed (o, spec, va_arg (args, ptrdiff_t));
			else
				do_signed (o, spec, va_arg (args, intptr_t));
			break;
		case 'u':
			if (size == 0 || size == 1 || size == 2)
				do_unsigned<10> (o, spec, va_arg (args, unsigned int));
			else if (size == 3)
				do_unsigned<10> (o, spec, va_arg (args, unsigned long));
			else if (size == 4)
				do_unsigned<10> (o, spec, va_arg (args, unsigned long long));
			else if (size == 5)
				do_unsigned<10> (o, spec, va_arg (args, uintmax_t));
			else if (size == 6)
				do_unsigned<10> (o, spec, va_arg (args, size_t));
			else
				do_unsigned<10> (o, spec, va_arg (args, uintptr_t));
			break;
		case 'o':
			if (size == 0 || size == 1 || size == 2)
				do_unsigned<8> (o, spec, va_arg (args, unsigned int));
			else if (size == 3)
				do_unsigned<8> (o, spec, va_arg (args, unsigned long));
			else if (size == 4)
				do_unsigned<8> (o, spec, va_arg (args, unsigned long long));
			else if (size == 5)
				do_unsigned<8> (o, spec, va_arg (args, uintmax_t));
			else if (size == 6)
				do_unsigned<8> (o, spec, va_arg (args, size_t));
			else
				do_unsigned<8> (o, spec, va_arg (args, uintptr_t));
			break;
		case 'X':
			spec.flags |= PR_UPPER;
			[[fallthrough]];
		case 'x':
			if (size == 0 || size == 1 || size == 2)
				do_unsigned<16> (o, spec, va_arg (args, unsigned int));
			else if (size == 3)
				do_unsigned<16> (o, spec, va_arg (args, unsigned long));
			else if (size == 4)
				do_unsigned<16> (o, spec, va_arg (args, unsigned long long));
			else if (size == 5)
				do_unsigned<16> (o, spec, va_arg (args, uintmax_t));
			else if (size == 6)
				do_unsigned<16> (o, spec, va_arg (args, size_t));
			else
				do_unsigned<16> (o, spec, va_arg (args, uintptr_t));
			break;
		case 'p':
			spec.flags = PR_ZERO | PR_SPECIAL;
			spec.width = 2 + 2 * sizeof (void *);
			do_unsigned<16> (o, spec, reinterpret_cast<uintptr_t> (va_arg (args, void *)));
			break;
		default:
			put_string_auto (o, "(bad format string)");
			*o.pos = 0;
			return;
		}
	}

	*o.pos = 0;
}

extern "C" void
snprintf (char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	vsnprintf (buf, size, fmt, args);
	va_end (args);
}
