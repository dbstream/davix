/**
 * Defines the VISIBLE macro, which will be used to make symbols available
 * to modules, when those are implemented.
 */
#ifndef _DAVIX_EXPORT_H
#define _DAVIX_EXPORT_H 1

#define VISIBLE __attribute__ ((visibility ("default")))

#endif /* _DAVIX_EXPORT_H */
