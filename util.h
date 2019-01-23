#include <string.h>	// for strcmp


#ifndef UTIL_H
#define UTIL_H

#define	MALLOCARRAY_NOFAIL(ptr, num) (ptr) = xmalloc(sizeof *(ptr) * num)
void *xmalloc(size_t);

#define	streq(a,b)	(0 == strcmp((a),(b)))

#undef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))


#ifndef pbm_allocrow_packed
#define pbm_allocrow_packed(cols) \
    ((unsigned char *) pm_allocrow(pbm_packed_bytes(cols), \
                                   sizeof(unsigned char)))
#endif
#ifndef	pbm_freerow_packed
#define pbm_freerow_packed(packed_bits) \
    pm_freerow((char *) packed_bits)
#endif


#endif	/* UTIL_H */
