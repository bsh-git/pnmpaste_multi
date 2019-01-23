#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <malloc.h>
#include "util.h"

void *
xmalloc(size_t sz)
{
	void *p = malloc(sz);

	if (p == NULL)
		err(EXIT_FAILURE, "memory exhausted");

	return p;
}
