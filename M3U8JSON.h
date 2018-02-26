#ifndef __M3U8JSON_H
#define __M3U8JSON_H

#include <windows.h>

CCHAR *M3U8Parser(CCHAR *s, CCHAR *f, CCHAR *b, DWORD sz);
void JSONParser(CCHAR *s, CCHAR *f, CCHAR *b, DWORD sz);

#endif
