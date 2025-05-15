#ifndef __M3U8JSON_H
#define __M3U8JSON_H

#include <windows.h>

CCHAR *M3U8Parser(CCHAR *s, CCHAR *f, CCHAR *b, DWORD sz);
// v1.12
CCHAR *GetWildMatchStr(CCHAR *ps, CCHAR *pm);
CCHAR *AddSlashesStr(CCHAR *ps);
CCHAR *JSONParserStr(CCHAR *ps, CCHAR *pm);

#endif
