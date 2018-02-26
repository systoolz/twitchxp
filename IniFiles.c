#include "IniFiles.h"

void IniInit(ini_data *id, TCHAR *s) { 
  if (id && s) {
    ZeroMemory(id, sizeof(id[0]));
    id->filename = s;
    for (; *s; s++); s++;
    id->section = s;
    for (; *s; s++); s++;
    id->current = s;
  }
}

inline BOOL IniTest(ini_data *id) {
  return(id && id->filename && id->section && id->current && *id->current);
}

inline void IniNext(ini_data *id) {
  for (; *id->current; id->current++); id->current++;
}

int IniGetInt(ini_data *id) {
int r;
  r = 0;
  if (IniTest(id)) {
    r = GetPrivateProfileInt(id->section, id->current, r, id->filename);
    IniNext(id);
  }
  return(r);
}

TCHAR *IniGetStr(ini_data *id) {
TCHAR *r, v[1024];
DWORD l;
  r = NULL;
  if (IniTest(id)) {
    *v = 0;
    l = 0;
    GetPrivateProfileString(id->section, id->current, (TCHAR *) &l, v, 1024, id->filename);
    IniNext(id);
    l = lstrlen(v);
    if (l) {
      l = (l + 1) * sizeof(r[0]);
      r = (TCHAR *) LocalAlloc(LMEM_FIXED, l);
      if (r) {
        CopyMemory(r, v, l);
      }
    }
  }
  return(r);
}

void IniPutStr(ini_data *id, TCHAR *value) {
  if (IniTest(id) && value) {
    WritePrivateProfileString(id->section, id->current, value, id->filename);
    IniNext(id);
  }
}

void IniPutInt(ini_data *id, int value) {
TCHAR v[16], f[3];
  if (IniTest(id)) {
    f[0] = TEXT('%');
    f[1] = TEXT('d');
    f[2] = 0;
    wsprintf(v, f, value);
    IniPutStr(id, v);
  }
}
