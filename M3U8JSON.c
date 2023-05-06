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
      x -= ((x >= 'a') && (x <= 'z')) ? ('a' - 'A') : 0;
      y -= ((y >= 'a') && (y <= 'z')) ? ('a' - 'A') : 0;
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

// v1.6 rewrite
#define COPY2BUF if (d && sz) { *d = *s; d++; sz--; }
CCHAR *JSONParm(CCHAR *s, CCHAR *d, DWORD sz) {
BYTE t[256];
int i;
  if (s) {
    // build table
    ZeroMemory(t, 256);
    // stop characters
    t[0] = 1;
    t['\t'] = 1;
    t['\n'] = 1;
    t['\r'] = 1;
    t[' '] = 1;
    t[','] = 1;
    t[':'] = 1;
    t['{'] = 1;
    t['}'] = 1;
    do {
      // string
      if (*s == '"') {
        s++;
        while (*s && (*s != '"')) {
          // quoted character
          if (*s == '\\') {
            s++;
          }
          COPY2BUF;
          s++;
        }
        break;
      }
      // array
      if (*s == '{') {
        i = 0;
        do {
          // array
          if ((*s == '{') || (*s == '}')) {
            COPY2BUF;
            i += (*s == '{') ? 1 : (-1);
            s++;
            continue;
          }
          // string
          if (*s == '"') {
            COPY2BUF;
            s++;
            while (*s && (*s != '"')) {
              // quoted character
              if (*s == '\\') {
                COPY2BUF;
                s++;
              }
              COPY2BUF;
              s++;
            }
            if (*s) {
              COPY2BUF;
              s++;
            }
            continue;
          }
          // anything else
          COPY2BUF;
          s++;
        } while (*s && (i > 0));
        break;
      }
      // plain value
      while (!t[(BYTE) *s]) {
        COPY2BUF;
        s++;
      }
    } while (0); // v1.8 fix
    // tail zero
    if (d) {
      d -= sz ? 0 : 1;
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

// v1.6
void GetWildMatch(CCHAR *ps, CCHAR *pm, CCHAR *pv, DWORD sz) {
BYTE t[256], w[256], *s, *m, *v;
DWORD i, j, k;
  if (ps && pm && pv && sz) {
    s = (BYTE *) ps;
    m = (BYTE *) pm;
    v = (BYTE *) pv;
    // case-insensitive compare table
    for (i = 0; i < 256; i++) {
      t[i] = i;
      t[i] -= ((i >= 'a') && (i <= 'z')) ? ('a' - 'A') : 0;
    }
    // whitespace table
    ZeroMemory(w, sizeof(w));
    w[9] = 1;
    w[10] = 1;
    w[13] = 1;
    w[32] = 1;
    *v = 0;
    i = 0;
    j = 0;
    k = 0;
    while (*s) {
      // whitespace in mask
      if (w[m[i]]) {
        // skip in mask
        while (w[m[i]]) { i++; }
        // skip in string
        while (w[s[j]]) { j++; }
        // next character
        continue;
      }
      // wildchar
      if (m[i] == '*') {
        i++;
        // copy everything till next mask char or string end
        while ((s[j]) && ((t[s[j]] != t[m[i]]) || ((w[m[i]]) && (!w[s[j]]))) && (k < sz)) {
          v[k] = s[j];
          k++;
          j++;
        }
        if (w[m[i]]) { continue; }
        // mismatch
        if (t[s[j]] != t[m[i]]) {
          // move to next char
          s++;
          // reset
          i = 0;
          j = 0;
          k = 0;
          continue;
        }
        i++;
        // found
        if ((!m[i]) || (k >= sz)) { break; }
      }
      // compare characters
      while (m[i] && (!w[m[i]]) && (m[i] != '*') && (t[s[j]] == t[m[i]])) {
        i++;
        j++;
      }
      // wildchar or whitespace
      if ((m[i] == '*') || (w[m[i]])) { continue; }
      // found
      if (!m[i]) { break; }
      // mismatch
      if ((t[s[j]] != t[m[i]])) {
        // move to next char
        s++;
        // reset
        i = 0;
        j = 0;
        k = 0;
      }
    }
    // tail zero byte
    v[(k < sz) ? k : (sz - 1)] = 0;
  }
}
