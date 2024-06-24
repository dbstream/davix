/**
 * Endianness conversions.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_ENDIAN_H
#define _DAVIX_ENDIAN_H 1

#define bswap16(x) __builtin_bswap16(x)
#define bswap32(x) __builtin_bswap32(x)
#define bswap64(x) __builtin_bswap64(x)

#define identity16(x) (x)
#define identity32(x) (x)
#define identity64(x) (x)

#if defined(__LITTLE_ENDIAN__) || defined(__ORDER_LITTLE_ENDIAN__)
#define __le_bswap(n, x) identity##n(x)
#define __be_bswap(n, x) bswap##n(x)
#else
#define __le_bswap(n, x) bswap##n(x)
#define __be_bswap(n, x) identity##n(x)
#endif

#define le16toh(x) __le_bswap(16, x)
#define le32toh(x) __le_bswap(32, x)
#define le64toh(x) __le_bswap(64, x)
#define htole16(x) __le_bswap(16, x)
#define htole32(x) __le_bswap(32, x)
#define htole64(x) __le_bswap(64, x)
#define be16toh(x) __be_bswap(16, x)
#define be32toh(x) __be_bswap(32, x)
#define be64toh(x) __be_bswap(64, x)
#define htobe16(x) __be_bswap(16, x)
#define htobe32(x) __be_bswap(32, x)
#define htobe64(x) __be_bswap(64, x)

#define ntoh16(x) be16toh(x)
#define ntoh32(x) be32toh(x)
#define ntoh64(x) be64toh(x)
#define hton16(x) htobe16(x)
#define hton32(x) htobe32(x)
#define hton64(x) htobe64(x)

#endif /* _DAVIX_ENDIAN_H */
