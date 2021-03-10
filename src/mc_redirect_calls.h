#ifndef MC_REDIRECT_CALLS_H
#define MC_REDIRECT_CALLS_H
#define	malloc(SIZE)			d_alloc(file, __LINE__, SIZE)
#define	realloc(POINTER, SIZE)	d_realloc(file, __LINE__, POINTER, SIZE)
#define	free(POINTER)			d_free(file, __LINE__, POINTER)
#define memcpy(DST, SRC, SIZE)	d_memcpy(file, __LINE__, DST, SRC, SIZE)
#define memset(DST, VAL, SIZE)	d_memset(file, __LINE__, DST, VAL, SIZE)
#endif