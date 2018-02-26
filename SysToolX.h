#ifndef __SYSTOOLX_H
#define __SYSTOOLX_H

#include <windows.h>

#define MEM_MOVE(x, y) (*((y *)x))
#define STR_SIZE(x) ((lstrlen(x) + 1) * sizeof(TCHAR))
#define STR_ALLOC(x) ((TCHAR *) GetMem((x + 1) * sizeof(TCHAR)))

void FreeMem(void *block);
void *GetMem(DWORD dwSize);
void *GrowMem(void *block, DWORD dwSize);

void StTrim(TCHAR *str);
TCHAR *LangLoadString(UINT sid);

TCHAR *GetFullFilePath(TCHAR *filename);
void SafeFileName(TCHAR *s);
TCHAR *GetWndText(HWND wnd);

TCHAR *OpenSaveDialog(HWND wnd, TCHAR *filemask, TCHAR *defext, TCHAR *defname, int savedlg);

void URLOpenLink(HWND wnd, TCHAR *s);
BOOL WinExecFile(TCHAR *s, DWORD w);

int MsgBox(HWND wnd, TCHAR *lpText, UINT uType);
void DialogEnableWindow(HWND hdlg, int idControl, BOOL bEnable);
void AdjustComboBoxHeight(HWND hWndCmbBox, DWORD MaxVisItems);

BYTE *HTTPGetContent(TCHAR *url, DWORD *len);

#endif
