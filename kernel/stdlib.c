/* SPDX-License-Identifier: MIT */
unsigned long strlen(const char *s);
unsigned long strlen(const char *s) {
        unsigned long l = 0;
        while(*s) {
                l++;
                s++;
        }
        return l;
}

int strcmp(const char *s0, const char *s1);
int strcmp(const char *s0, const char *s1) {
        while(*s0 == *s1) {
                if(*s0 == 0)
                        return 0;
                s0++;
                s1++;
        }
        return *s0 - *s1;
}

int strncmp(const char *s0, const char *s1, unsigned long n);
int strncmp(const char *s0, const char *s1, unsigned long n)
{
        for(; n; s0++, s1++, n--) {
                if(*s0 - *s1)
                        return *s0 - *s1;
        }
        return 0;
}

char *strcpy(char *dst, const char *src);
char *strcpy(char *dst, const char *src)
{
        char *dst_old = dst;
        for(; *src; dst++, src++)
                *dst = *src;
        *dst = 0;
        return dst_old;
}

char *strncpy(char *dst, const char *src, unsigned long n);
char *strncpy(char *dst, const char *src, unsigned long n)
{
        char *dst_old = dst;
        for(; n && (n - 1) && *src; src++, n--)
                *(dst++) = *src;
        for(; n; dst++, n--)
                *dst = 0;
        return dst_old;
}

char *strcat(char *dst, const char *src);
char *strcat(char *dst, const char *src)
{
        char *dst_old = dst;
        for(; *dst; dst++);
        for(; *src; dst++, src++)
                *dst = *src;
        *dst = 0;
        return dst_old;
}

char *strncat(char *dst, const char *src, unsigned long n);
char *strncat(char *dst, const char *src, unsigned long n)
{
        char *dst_old = dst;
        for(; *dst; dst++);
        for(; n && *src; dst++, src++, n--)
                *dst = *src;
        *dst = 0;
        return dst_old;
}

int memcmp(const void *mem1, const void *mem2, unsigned long n);
int memcmp(const void *mem1, const void *mem2, unsigned long n)
{
	const char *str1 = (const char *) mem1;
	const char *str2 = (const char *) str2;

        if(n == 0)
                return 0;

        for(; n && (*str1 == *str2); str1++, str2++, n--);

        return *str1 - *str2;
}

void *memcpy(void *dst, const void *src, unsigned long n);
void *memcpy(void *dst, const void *src, unsigned long n)
{
	const char *src_s = (const char *) src;
        for(char *i = (char *) dst; n > 0; i++, src_s++, n--)
                *i = *src_s;
        return dst;
}

void *memset(void *dst, int val, unsigned long n);
void *memset(void *dst, int val, unsigned long n)
{
        for(unsigned long i = 0; i < n; i++)
		((char *) dst)[i] = (char) val;
        return dst;
}

void *memmove(void *dst, const void *src, unsigned long n);
void *memmove(void *dst, const void *src, unsigned long n)
{
	char *dst_s = (char *) dst;
	const char *src_s = (const char *) src;
        if(src_s > dst_s) {
                for(unsigned long i = 0; i < n; i++)
                        dst_s[i] = src_s[i];
        } else {
                for(unsigned long i = n; i > 0;) {
                        i--;
                        dst_s[i] = src_s[i];
                }
        }

        return dst;
}
