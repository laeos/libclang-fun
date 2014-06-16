#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>

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

static CXIndex clang_index;

static struct expando path_list = {0};
static struct expando arg_list = {0};

static std::unordered_set<std::string> path_seen;

struct parse_entry {
    CXTranslationUnit tu;
    const char *fname;
}; 

static std::queue<struct parse_entry *> parse_queue;

static ostream *output;

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

      *output << "\"" << ctx->fname << "\"->\"" << y << "\";\n";

      free((void *)y);

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
	if (path_seen.count(rs)) {
	    return;
	}
	path_seen.insert(rs);
	CXFile file = clang_getFile(tu, rs);
	clang_findIncludesInFile(tu, file, visitor);
    }
}






static void parse(struct expando *e, const char *fname)
{
    CXTranslationUnit tu = clang_parseTranslationUnit(clang_index, fname, e->args, e->used, NULL, 0, CXTranslationUnit_DetailedPreprocessingRecord);
    dump_includes(tu, fname);

    while (!parse_queue.empty()) {
	struct parse_entry *p = parse_queue.front();
	dump_includes(p->tu, p->fname);
	parse_queue.pop();
    }

    clang_disposeTranslationUnit(tu);
}

static int dir_callback(const char *fpath, const struct stat *sb, int typeflag)
{
    if (S_ISREG(sb->st_mode))
	parse(&arg_list, fpath);
    return 0;
}

static bool is_dir(const char *path)
{
    struct stat st;

    if (stat(path, &st) < 0) {
	fprintf(stderr, "stat %s: %s\n", path, strerror(errno));
	exit(1);
    }
    return S_ISDIR(st.st_mode);
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

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-I<include> ...] <file> ...\n", prog);
    exit(1);
}


int main(int argc, char *argv[])
{
    char *output_file = NULL;
    int ch;

    clang_index = clang_createIndex(0, 0);

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

    output = open_output(output_file);

    *output << "digraph includes {" << "\n";

    while (argc) {
	if (is_dir(*argv)) {
	    printf("recurse %s\r\n", *argv);
	    ftw(*argv, dir_callback, 100);
	} else {
	    printf("parse %s\r\n", *argv);
	    parse(&arg_list, *argv);
	}
	--argc;
	++argv;
    }

    *output << "}" << "\n";

    if (output_file) {
	((ofstream *)output)->close();
	delete output;
    }

    return 0;
}
