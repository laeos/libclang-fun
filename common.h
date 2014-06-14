#ifndef COMMON_H
#define COMMON_H

#include "clang-c/Index.h"

const char *type2str(enum CXCursorKind k);
const char *linkage2str(enum CXLinkageKind l);
void dump_tokens(CXCursor cursor);

#endif
