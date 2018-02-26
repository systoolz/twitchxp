#include "M3U8JSON.h"

CCHAR *SkipWhiteSpaces(CCHAR *s) {
  if (s) {
    while (
      (*s == ' ') ||
      (*s == '\t') ||
      (*s == '\r') ||
      (*s == '\n')
    ) { s++; }
  }
  return(s);
}

CCHAR *SkipThisLine(CCHAR *s) {
  if (s) {
    for (; ((*s) && (*s != '\r') && (*s != '\n')); s++);
    s = SkipWhiteSpaces(s);
  }
  return(s);
}

CCHAR *SkipToChar(CCHAR *s, CCHAR c) {
  if (s) {
    s = SkipWhiteSpaces(s);
    if (*s == c) {
      s++;
      s = SkipWhiteSpaces(s);
    }
  }
  return(s);
}

BOOL StrBufMatch(CCHAR *s, CCHAR *b) {
BOOL result;
CCHAR x, y;
  // s == b == NULL or same pointer
  result = (s == b);
  if ((!result) && s && b) {
    while (*s) {
      x = *s;
      y = *b;
      x ^= ((x >= 'a') && (x <= 'y')) ? 0x20 : 0;
      y ^= ((y >= 'a') && (y <= 'y')) ? 0x20 : 0;
      if (x ^ y) { break; }
      s++;
      b++;
    }
    result = (!*s);
  }
  return(result);
}

CCHAR *GetPropFromString(CCHAR *n, CCHAR *s) {
CCHAR *r;
DWORD l;
  r = NULL;
  if (s && n) {
    l = lstrlenA(n);
    while (*s) {
      s = SkipWhiteSpaces(s);
      if (StrBufMatch(n, s)) {
        r = SkipWhiteSpaces(&s[l]);
        if (*r == '=') {
          r = SkipWhiteSpaces(&r[1]);
          break;
        }
        r = NULL;
      }
      s++;
    }
  }
  return(r);
}

void GetStrParm(CCHAR *s, CCHAR *u, DWORD sz, BOOL bline) {
CCHAR a, b;
  // sanity checks
  if (s && u && sz) {
    if (bline) {
      a = '\n';
      b = '\r';
    } else {
      a = (*s == '"') ? '"' : ',';
      s += (*s == '"') ? 1 : 0;
      b = 0;
    }
    // for zero tail char
    sz--;
    for (; ((sz) && (*s) && (*s != a) && (*s != b)); s++) {
      *u = *s;
      u++;
      sz--;
    }
    *u = 0;
  }
}

CCHAR *M3U8Parser(CCHAR *s, CCHAR *f, CCHAR *b, DWORD sz) {
  // sanity checks
  if (s && b && sz) {
    *b = 0;
    while (*s && sz) {
      s = SkipWhiteSpaces(s);
      // any prefix or params
      if (f) {
        // only if buffer matches (lazy)
        if (StrBufMatch(f, s)) {
          s = SkipToChar(s, ':');
          f = GetPropFromString(&f[lstrlenA(f) + 1], s);
          // string found
          if (f) {
            GetStrParm(f, b, sz, FALSE);
          }
          sz = 0;
        }
      } else {
        // get the whole line
        GetStrParm(s, b, sz, TRUE);
        sz = 0;
      }
      s = SkipThisLine(s);
    }
  }
  return(s);
}

CCHAR *JSONParm(CCHAR *s, CCHAR *d, DWORD sz) {
int q;
  if (s) {
    q = (*s == '"') ? 1 : 0;
    // skip double quote if necessary
    s += q;
    while (*s) {
      if (d && sz) {
        *d = *s;
      }
      if (q) {
        // quoted character
        if (*s == '\\') {
          s++;
          if (d && sz) {
            *d = *s;
          }
        } else {
          // value end
          if (*s == '"') { break; }
        }
      } else {
        // white space or delims
        if (
          (*s == '\t') ||
          (*s == '\r') ||
          (*s == '\n') ||
          (*s == ' ') ||
          (*s == ',') ||
          (*s == ':') ||
          (*s == '}')
        ) { break; }
      }
      s++;
      if (d && sz) {
        d++;
        sz--;
      }
    }
    // add zero terminator
    if (d) {
      *d = 0;
    }
  }
  return(s);
}

void JSONParser(CCHAR *s, CCHAR *f, CCHAR *b, DWORD sz) {
BOOL m;
  // sanity check
  if (s && f && b && sz) {
    *b = 0;
    s = SkipWhiteSpaces(s);
    // check JSON format
    if (*s == '{') {
      // start parsing parameters
      while (*s) {
        // skip char
        s = SkipWhiteSpaces(&s[1]);
        // name must start with double quote...
        if (*s != '"') { break; }
        // match name (lazy)
        m = StrBufMatch(f, s);
        // skip name
        s = JSONParm(s, NULL, 0);
        // ... and name must end with it
        if (*s != '"') { break; }
        s = SkipWhiteSpaces(&s[1]);
        // name:value separator char
        if (*s != ':') { break; }
        s = SkipWhiteSpaces(&s[1]);
        // get value or skip
        s = JSONParm(s, m ? b : NULL, m ? sz : 0);
        // value found
        if (m) { break; }
        // if not - go to the next name:value pair
        s = SkipWhiteSpaces((*s == '"') ? &s[1] : s);
        // parameters separator
        if (*s != ',') { break; }
      }
    }
  }
}
