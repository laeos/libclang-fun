void dump_tokens(CXCursor cursor)
{
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = 0;
    unsigned int nTokens = 0;
    clang_tokenize(tu, range, &tokens, &nTokens);
    for (unsigned int i = 0; i < nTokens; i++)
    {
	CXString spelling = clang_getTokenSpelling(tu, tokens[i]);
	printf("token = %s\n", clang_getCString(spelling));
	clang_disposeString(spelling);
    }
    clang_disposeTokens(tu, tokens, nTokens);
}
