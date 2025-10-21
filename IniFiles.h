#ifndef __INIFILES_H
#define __INIFILES_H

#include <windows.h>

#pragma pack(push, 1)
typedef struct {
  const TCHAR *filename;
  const TCHAR *section;
  const TCHAR *current;
} ini_data;
#pragma pack(pop)

void IniInit(ini_data *id, const TCHAR *s);
int IniGetInt(ini_data *id);
TCHAR *IniGetStr(ini_data *id);
void IniPutStr(ini_data *id, TCHAR *value);
void IniPutInt(ini_data *id, int value);

#endif
