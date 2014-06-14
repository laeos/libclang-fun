#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

struct expando {
    const char **args;
    size_t used;
    size_t alloced;
};

static void grow(struct expando *e)
{
    if (e->alloced)
	e->alloced *= 2;
    else
	e->alloced = 10;

    e->args = realloc(e->args, sizeof(char *) * e->alloced);
}

static void push(struct expando *e, const char *f)
{
    if (e->used >= e->alloced)
	grow(e);

    e->args[e->used++] = f;
}

enum CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
    if (clang_getCursorKind(cursor) == CXCursor_InclusionDirective) {
	CXSourceRange range = clang_getCursorExtent(cursor);
	CXSourceLocation location = clang_getRangeStart(range);

	CXFile file;
	unsigned line;
	unsigned column;
	clang_getFileLocation(location, &file, &line, &column, NULL);

	CXString filename = clang_getFileName(file);
	CXString name = clang_getCursorSpelling(cursor);

	printf("%s -> %s\r\n",
		clang_getCString(filename),
		clang_getCString(name));

	clang_disposeString(name);
	clang_disposeString(filename);
    }
    return CXChildVisit_Recurse;
}

static void parse(struct expando *e, const char *fname)
{
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(index, fname, e->args, e->used, NULL, 0, CXTranslationUnit_DetailedPreprocessingRecord);

    clang_visitChildren(
	    clang_getTranslationUnitCursor(tu),
	    visitor,
	    NULL);

    clang_disposeTranslationUnit(tu);
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-I<include> ...] <file> ...\n", prog);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct expando e = {0};
    int ch;

    while ((ch = getopt(argc, argv, "I:")) != -1) {
	switch (ch) {
	    case 'I':
		printf("include %s\r\n", optarg);
		push(&e, optarg);
		break;
	    case '?':
	    default:
		usage(argv[0]);
	}
    }
    argc -= optind;
    argv += optind;

    while (argc) {
	printf("parse %s\r\n", *argv);
	parse(&e, *argv);
	--argc;
	++argv;
    }
    return 0;
}

