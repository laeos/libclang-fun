#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <map>
#include <unordered_set>
#include <string>
#include <queue>

#include "common.h"

using namespace std;

struct my_context {
    const char *fname;
};

struct expando {
    const char **args;
    size_t used;
    size_t alloced;
};

struct expando path_list = {0};
struct expando arg_list = {0};

std::map<std::string, std::unordered_set<std::string>> path_map;

struct parse_entry {
    CXTranslationUnit tu;
    const char *fname;
}; 

std::queue<struct parse_entry *> parse_queue;

static void grow(struct expando *e)
{
    if (e->alloced)
	e->alloced *= 2;
    else
	e->alloced = 10;

    e->args = (const char **)realloc(e->args, sizeof(char *) * e->alloced);
}

static void push(struct expando *e, const char *arg)
{
    if (e->used >= e->alloced)
	grow(e);
    e->args[e->used++] = arg;
}

static void push_path(const char *f)
{
    char *path = realpath(f, NULL);
    char buffer[1024];

    push(&path_list, f);

    snprintf(buffer, sizeof(buffer), "-I%s", path);
    push(&arg_list, strdup(buffer));
}

static const char *resolve_name(const char *name)
{
    size_t i;

    if (name[0] == '/')
	return realpath(name, NULL);


    for (i = 0; i < path_list.used; i++) {
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s/%s", path_list.args[i], name);
	char *happy = realpath(buf, NULL);
	if (!access(happy, R_OK))
	    return happy;
	free(happy);
    }
    fprintf(stderr, "could not resolve %s\r\n", name);
    exit(1);
}


static enum CXVisitorResult include_visit(void *context, CXCursor cursor, CXSourceRange range) {
  struct my_context *ctx = (struct my_context *)context;
  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
  CXFile file = clang_getIncludedFile(cursor);
  CXString filename = clang_getFileName(file);

  const char *s = clang_getCString(filename);

  if (s) {
      struct parse_entry *p = new parse_entry;
      const char *y = resolve_name(s);

      path_map[ctx->fname].insert(y);

      p->fname = s;
      p->tu = tu;
      parse_queue.push(p);
  }

  clang_disposeString(filename);

  return CXVisit_Continue;
}


static void dump_includes(CXTranslationUnit tu, const char *fname)
{
    if (fname) {
	const char *rs = resolve_name(fname);
	struct my_context ctx = {
	    .fname = rs,
	};
	CXCursorAndRangeVisitor visitor = {
	    .context = &ctx,
	    .visit = include_visit
	};
	if (path_map.count(rs)) {
	    return;
	}
	CXFile file = clang_getFile(tu, rs);
	clang_findIncludesInFile(tu, file, visitor);
    }
}

static void parse(struct expando *e, const char *fname)
{
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(index, fname, e->args, e->used, NULL, 0, CXTranslationUnit_DetailedPreprocessingRecord);

    dump_includes(tu, fname);

    while (!parse_queue.empty()) {
	struct parse_entry *p = parse_queue.front();
	dump_includes(p->tu, p->fname);
	parse_queue.pop();
    }

    clang_disposeTranslationUnit(tu);
}

static ostream *open_output(const char *output_file)
{

    if (output_file) {
	ofstream *output = new ofstream();
	output->open(output_file);
	return output;
    } else {
	return &std::cout;
    }
}

static void dump_map(const char *output_file)
{
    ostream *output = open_output(output_file);

    std::map<std::string, std::unordered_set<std::string>>::iterator it;

    *output << "digraph includes {" << "\n";

    for (it = path_map.begin(); it!=path_map.end(); ++it) {

	std::unordered_set<std::string>::iterator bt;

	for (bt = it->second.begin(); bt != it->second.end(); ++bt) {
	    *output << "\"" << it->first << "\"" << "->";
	    *output << "\"" << *bt << "\"" << ";\n";
	}
    }

    *output << "}" << "\n";

    if (output_file) {
	((ofstream *)output)->close();
	delete output;
    }
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-I<include> ...] <file> ...\n", prog);
    exit(1);
}


int main(int argc, char *argv[])
{
    char *output_file = NULL;
    int ch;

    while ((ch = getopt(argc, argv, "I:o:")) != -1) {
	switch (ch) {
	    case 'o':
		output_file = strdup(optarg);
		break;
	    case 'I':
		push_path(optarg);
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
	parse(&arg_list, *argv);
	--argc;
	++argv;
    }
    dump_map(output_file);
    return 0;
}
