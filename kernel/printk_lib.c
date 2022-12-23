/* SPDX-License-Identifier: MIT */
#include <davix/printk_lib.h>

typedef int bitwise prflag_t;

#define PR_CAPITALIZE ((force prflag_t)(1 << 0))
#define PR_ALTERNATIVE ((force prflag_t)(1 << 1))
#define PR_LONG ((force prflag_t)(1 << 2))
#define PR_ZEROPAD ((force prflag_t)(1 << 3))
#define PR_LEFTPAD ((force prflag_t)(1 << 4))
#define PR_SPACE ((force prflag_t)(1 << 5))
#define PR_SIGN ((force prflag_t)(1 << 6))

static const char alpha_lower[] = "0123456789abcdef";
static const char alpha_upper[] = "0123456789ABCDEF";

static int get_number(const char **fmt)
{
	int result = 0;
	while(**fmt >= '0' && **fmt <= '9') {
		result *= 10;
		result += **fmt - '0';
		(*fmt)++;
	}

	return result;
}

static int do_number(struct printk_callback *cb, prflag_t flag, int radix, int w, unsigned long long num)
{
	const char *alpha = flag & PR_CAPITALIZE ? alpha_upper : alpha_lower;
	int len = 0;
	char buf[64];

	do {
		buf[len++] = alpha[num % radix];
		num /= radix;
	} while(num);

	if(flag & PR_ZEROPAD)
		while(len < w) {
			if(cb->putc(cb, '0'))
				return 1;
			w--;
		}

	while(len)
		if(cb->putc(cb, buf[--len]))
			return 1;
	return 0;
}

static int number_len(int radix, unsigned long long num)
{
	int len = 0;
	do {
		len++;
		num /= radix;
	} while(num);
	return len;
}


static int do_unsigned(struct printk_callback *cb, prflag_t flag, int radix, int w, va_list *ap)
{
	unsigned long long num;
	if(flag & PR_LONG)
		num = va_arg(*ap, unsigned long long);
	else
		num = va_arg(*ap, unsigned int);

	if(flag & PR_ZEROPAD) {
		if((flag & PR_ALTERNATIVE) && radix == 16) {
			w -= 2;
			if(w < 0)
				w = 0;
			if(cb->putc(cb, '0'))
				return 1;
			if(cb->putc(cb, 'x'))
				return 1;
		} else if((flag & PR_ALTERNATIVE) && radix == 8) {
			w -= 2;
			if(w < 0)
				w = 0;
			if(cb->putc(cb, '0'))
				return 1;
			if(cb->putc(cb, 'o'))
				return 1;
		}
		return do_number(cb, flag, radix, w, num);
	}

	int nlen = number_len(radix, num);
	if((flag & PR_ALTERNATIVE) && (radix == 8 || radix == 16))
		nlen += 2;
	if(!(flag & PR_LEFTPAD)) {
		while(nlen < w) {
			if(cb->putc(cb, ' '))
				return 1;
			w--;
		}
	}
	if((flag & PR_ALTERNATIVE) && radix == 16) {
		if(cb->putc(cb, '0'))
			return 1;
		if(cb->putc(cb, 'x'))
			return 1;
	} else if((flag & PR_ALTERNATIVE) && radix == 8) {
		if(cb->putc(cb, '0'))
			return 1;
		if(cb->putc(cb, 'o'))
			return 1;
	}

	if(do_number(cb, flag, radix, w, num))
		return 1;
	while(nlen < w) {
		if(cb->putc(cb, ' '))
			return 1;
		w--;
	}
	return 0;
}

static int do_signed(struct printk_callback *cb, prflag_t flag, int radix, int w, va_list *ap)
{
	long long raw_num;
	if(flag & PR_LONG)
		raw_num = va_arg(*ap, long long);
	else
		raw_num = va_arg(*ap, int);

	unsigned long long num;
	num = raw_num < 0 ? -raw_num : raw_num;

	char sign = 0;
	if(raw_num < 0)
		sign = '-';
	else if(flag & PR_SIGN)
		sign = '+';
	else if(flag & PR_SPACE)
		sign = ' ';

	if(flag & PR_ZEROPAD) {
		if(sign) {
			w--;
			if(cb->putc(cb, sign))
				return 1;
		}
		return do_number(cb, flag, radix, w, num);
	}
	int nlen = number_len(radix, num);
	if(sign)
		nlen++;
	if(!(flag & PR_LEFTPAD)) {
		while(nlen < w) {
			if(cb->putc(cb, ' '))
				return 1;
			w--;
		}
	}
	if(sign)
		if(cb->putc(cb, sign))
			return 1;
	if(do_number(cb, flag, radix, w, num))
		return 1;
	while(nlen < w) {
		if(cb->putc(cb, ' '))
			return 1;
		w--;
	}
	return 0;
}

static int do_string(struct printk_callback *cb, prflag_t flag, int w, int p, const char *s)
{
	unsigned long len = 0;
	while(s[len] && (len < p || !p))
		len++;

	unsigned long pad;
	if(w < len)
		pad = 0;
	else
		pad = w - len;

	if(!(flag & PR_LEFTPAD))
		for(; pad; pad--)
			if(cb->putc(cb, ' '))
				return 1;
	for(unsigned long i = 0; i < len; i++)
		if(cb->putc(cb, s[i]))
			return 1;
	for(; pad; pad--)
		if(cb->putc(cb, ' '))
			return 1;

	return 0;
}

int raw_vprintk(struct printk_callback *cb, const char *fmt, va_list *ap)
{
	while(1) {
		char c = *(fmt++);
		if(!c)
			return 0;
		if(c == '%') {
			prflag_t flag = 0;
			while(1) {
				c = *(fmt++);
				switch(c) {
				case 0:
					return 0;
				case '%':
					if(cb->putc(cb, '%'))
						return 1;
					goto no_fmt;
				case '#':
					flag |= PR_ALTERNATIVE;
					break;
				case '0':
					flag |= PR_ZEROPAD;
					break;
				case '-':
					flag |= PR_LEFTPAD;
					break;
				case ' ':
					flag |= PR_SPACE;
					break;
				case '+':
					flag |= PR_SIGN;
					break;
				default:
					fmt--;
					goto do_fmt;
				}
			}
do_fmt:;
			int w = 0, p = 0;
			if(*fmt == '*') {
				w = va_arg(*ap, int);
				fmt++;
			} else
				w = get_number(&fmt);
			if(*fmt == '.') {
				fmt++;
				if(*fmt == '*') {
					p = va_arg(*ap, int);
					fmt++;
				} else
					p = get_number(&fmt);
			}

			if(*fmt == 'h') {
				fmt++;
				if(*fmt == 'h')
					fmt++;
			} else if(*fmt == 'l') {
				if(sizeof(long) > sizeof(int))
					flag |= PR_LONG;
				fmt++;
				if(*fmt == 'l') {
					if(!(sizeof(long) > sizeof(int)))
						flag |= PR_LONG;
					fmt++;
				}
			} else if(*fmt == 'j' || *fmt == 'z' || *fmt == 't') {
				if(sizeof(long) > sizeof(int))
					flag |= PR_LONG;
				fmt++;
			}

			c = *(fmt++);
			switch(c) {
			case 'c':
				if(cb->putc(cb, (char) va_arg(*ap, int)))
					return 1;
				break;
			case 's':
				if(do_string(cb, flag, w, p, va_arg(*ap, const char *)))
					return 1;
				break;
			case 'i':
			case 'd':
				if(do_signed(cb, flag, 10, w, ap))
					return 1;
				break;
			case 'u':
				if(do_unsigned(cb, flag, 10, w, ap))
					return 1;
				break;
			case 'p':
				flag |= PR_ALTERNATIVE | PR_ZEROPAD;
				if(sizeof(long) > sizeof(int))
					flag |= PR_LONG;
				if(do_unsigned(cb, flag, 16, 2 * sizeof(long) + 2, ap))
					return 1;
				break;
			case 'X':
				flag |= PR_CAPITALIZE;
			case 'x':
				if(do_unsigned(cb, flag, 16, w, ap))
					return 1;
				break;
			case 'o':
				if(do_unsigned(cb, flag, 8, w, ap))
					return 1;
				break;
			case 'b':
				if(do_unsigned(cb, flag, 2, w, ap))
					return 1;
				break;
			default:
				return 0;
			}
		} else {
			if(cb->putc(cb, c))
				return 1;
		}
no_fmt:;
	}
}

int raw_printk(struct printk_callback *cb, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = raw_vprintk(cb, fmt, &ap);
	va_end(ap);
	return ret;
}

struct snprintf_buf {
	struct printk_callback cb;
	char *buf;
	unsigned long bufsiz;
};

static int snprintf_buf_putc(struct printk_callback *cb, char c)
{
	struct snprintf_buf *buf = container_of(cb, struct snprintf_buf, cb);
	*(buf->buf) = c;
	buf->buf++;
	return --(buf->bufsiz) == 1;
}

static void _vsnprintk(char *buf, unsigned long bufsiz, const char *fmt, va_list *ap)
{
	if(!bufsiz)
		return;

	struct snprintf_buf prbuf = {
		.cb.putc = snprintf_buf_putc,
		.buf = buf,
		.bufsiz = bufsiz
	};

	raw_vprintk(&prbuf.cb, fmt, ap);
	*(prbuf.buf) = 0;
}

void vsnprintk(char *buf, unsigned long bufsiz, const char *fmt, va_list ap)
{
	/*
	 * This has to be done for my linter not to complain about passing
	 * a `struct __va_list_tag **` to a parameter of type `va_list *`.
	 */
	va_list ap2;
	va_copy(ap2, ap);

	_vsnprintk(buf, bufsiz, fmt, &ap2);

	va_end(ap2);
}

void snprintk(char *buf, unsigned long bufsiz, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_vsnprintk(buf, bufsiz, fmt, &ap);
	va_end(ap);	
}
