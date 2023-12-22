#ifndef __LAMA_RUNTIME__
#define __LAMA_RUNTIME__

# include <stdio.h>
# include <stdio.h>
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
# include <sys/mman.h>
# include <assert.h>
# include <errno.h>
# include <regex.h>
# include <time.h>
# include <limits.h>
# include <ctype.h>

#define WORD_SIZE (CHAR_BIT * sizeof(int))

# define STRING_TAG  0x00000001
# define ARRAY_TAG   0x00000003
# define SEXP_TAG    0x00000005
# define CLOSURE_TAG 0x00000007
# define UNBOXED_TAG 0x00000009 // Not actually a tag; used to return from LkindOf

# define LEN(x) ((x & 0xFFFFFFF8) >> 3)
# define TAG(x)  (x & 0x00000007)
# define TO_SEXP(x) ((sexp*)((char*)(x)-2*sizeof(int)))
# define GET_SEXP_TAG(x) (LEN(x))

void failure (char *s, ...);

typedef struct {
    int tag;
    char contents[0];
} data;

typedef struct {
    int tag;
    data contents;
} sexp;

# define TO_DATA(x) ((data*)((char*)(x)-sizeof(int)))

# define UNBOXED(x)  (((int) (x)) &  0x0001)
# define UNBOX(x)    (((int) (x)) >> 1)
# define BOX(x)      ((((int) (x)) << 1) | 0x0001)

# define ASSERT_STRING(memo, x)              \
  do if (!UNBOXED(x) && TAG(TO_DATA(x)->tag) \
	 != STRING_TAG) failure ((char*)"string value expected in %s\n", memo); while (0)

# define ASSERT_BOXED(memo, x)               \
  do if (UNBOXED(x)) failure ((char*) "boxed value expected in %s\n", memo); while (0)
# define ASSERT_UNBOXED(memo, x)             \
  do if (!UNBOXED(x)) failure ((char*) "unboxed value expected in %s\n", memo); while (0)

static char* chars = (char*)"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'";

#endif
