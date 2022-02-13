// RUN: %check_clang_tidy %s modernize-redundant-void-arg %t

#ident "@RCS@"

#include "util-pp-tree.h"
#include <stdio.h>

#define NULL 0

#if defined(NULL)
#ifdef NULL
#elif defined(NOT_NULL)
#endif
#else
#endif

#undef NULL
