#include "SysToolX.h"
#include <wininet.h>

void FreeMem(void *block) {
  if (block) {
    LocalFree(block);
  }
}

void *GetMem(DWORD dwSize) {
  return((void *) LocalAlloc(LPTR, dwSize));
}

void *GrowMem(void *block, DWORD dwSize) {
  if (!dwSize) {
    if (block) {
      FreeMem(block);
    }
    block = NULL;
  } else {
    if (block) {
      block = (void *) LocalReAlloc(block, dwSize, LMEM_MOVEABLE | LMEM_ZEROINIT);
    } else {
      block = GetMem(dwSize);
    }
  }
  return(block);
}

inline BOOL IsEmptyChar(TCHAR c) {
  return(((c == TEXT(' ')) || (c == TEXT('\t')) || (c == TEXT('\r')) || (c == TEXT('\n'))) ? 1 : 0);
}

void StTrim(TCHAR *str) {
DWORD i, s, e;
  if (str) {
    s = 0;
    e = 0;
    for (i = 0; str[i]; i++) {
      if (!IsEmptyChar(str[i])) {
        if ((!s) && IsEmptyChar(str[s])) {
          s = i;
        }
        e = i + 1;
      }
      str[i - s] = str[i];
    }
    str[e - s] = 0;
  }
}

TCHAR *LangLoadString(UINT sid) {
TCHAR *res;
WORD *p;
HRSRC hr;
int i;
  res = NULL;
  hr = FindResource(NULL, MAKEINTRESOURCE(sid / 16 + 1), RT_STRING);
  p = hr ? (WORD *) LockResource(LoadResource(NULL, hr)) : NULL;
  if (p != NULL) {
    for (i = 0; i < (sid & 15); i++) {
      p += 1 + *p;
    }
    res = STR_ALLOC(*p);
    LoadString(NULL, sid, res, *p + 1);
  }
  return(res);
}

/* == FILE AND DISK ROUTINES =============================================== */

TCHAR *GetFullFilePath(TCHAR *filename) {
TCHAR *result, *np;
int sz;
  sz = GetFullPathName(filename, 0, NULL, &np);
  result = STR_ALLOC(sz);
  if (result) {
    GetFullPathName(filename, sz + 1, result, &np);
    result[sz] = 0;
  }
  return(result);
}

void SafeFileName(TCHAR *s) {
  if (s) {
    for (; *s; s++) {
      if (
        // \/:*?"<>|
        (*s == TEXT('"')) ||
        (*s == TEXT('*')) ||
        (*s == TEXT('/')) ||
        (*s == TEXT(':')) ||
        (*s == TEXT('<')) ||
        (*s == TEXT('>')) ||
        (*s == TEXT('?')) ||
        (*s == TEXT('\\')) ||
        (*s == TEXT('|'))
      ) {
        *s = TEXT('_');
      }
    }
  }
}

TCHAR *GetWndText(HWND wnd) {
TCHAR *result;
int sz;
  sz = GetWindowTextLength(wnd);
  result = STR_ALLOC(sz);
  if (result) {
    GetWindowText(wnd, result, sz + 1);
    result[sz] = 0;
  }
  return(result);
}

TCHAR *OpenSaveDialog(HWND wnd, TCHAR *filemask, TCHAR *defext, TCHAR *defname, int savedlg) {
OPENFILENAME ofn;
TCHAR filename[MAX_PATH], *result, sd[2];
  result = NULL;
  filename[0] = 0;
  if (defname) {
    lstrcpyn(filename, defname, MAX_PATH);
  }
  ZeroMemory(&ofn, sizeof(ofn));
  sd[0] = TEXT('.');
  sd[1] = 0;
  ofn.lpstrInitialDir = sd;
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner   = wnd;
  ofn.nMaxFile    = MAX_PATH;
  ofn.lpstrFile   = filename;
  ofn.Flags       = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR; // | OFN_EXPLORER
  ofn.Flags       |= savedlg ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST;
  /// OFN_ALLOWMULTISELECT
  // http://stackoverflow.com/questions/656655/getopenfilename-with-ofn-allowmultiselect-flag-set
  // http://support.microsoft.com/kb/131462
  ofn.lpstrFilter = filemask;
  ofn.lpstrDefExt = defext;
  if ((savedlg ? GetSaveFileName : GetOpenFileName)(&ofn)) {
    result = GetFullFilePath(ofn.lpstrFile);
  }
  return(result);
}

void URLOpenLink(HWND wnd, TCHAR *s) {
  CoInitialize(NULL);
  ShellExecute(wnd, NULL, s, NULL, NULL, SW_SHOWNORMAL);
  CoUninitialize();
}

BOOL WinExecFile(TCHAR *s, DWORD w) {
BOOL result;
PROCESS_INFORMATION cpi;
STARTUPINFO csi;
  ZeroMemory(&csi, sizeof(csi));
  csi.cb = sizeof(csi);
  csi.dwFlags = STARTF_USESHOWWINDOW;
  csi.wShowWindow = w;
  result = CreateProcess(NULL, s, NULL, NULL, FALSE, 0, NULL, NULL, &csi, &cpi);
  if (result) {
    // emulate original WinExec()
    WaitForInputIdle(cpi.hProcess, 30000);
    CloseHandle(cpi.hThread);
    CloseHandle(cpi.hProcess);
  }
  return(result);
}

int MsgBox(HWND wnd, TCHAR *lpText, UINT uType) {
int result;
TCHAR *s, *t;
  s = NULL;
  if (!HIWORD(lpText)) {
    s = LangLoadString(LOWORD(lpText));
  }
  t = GetWndText(wnd);
  result = MessageBox(wnd, s ? s : lpText, t, uType);
  if (t) { FreeMem(t); }
  if (s) { FreeMem(s); }
  return(result);
}

// http://blogs.msdn.com/b/oldnewthing/archive/2004/08/04/208005.aspx
void DialogEnableWindow(HWND hdlg, int idControl, BOOL bEnable) {
HWND hctl;
  hctl = GetDlgItem(hdlg, idControl);
  if ((bEnable == FALSE) && (hctl == GetFocus())) {
    SendMessage(hdlg, WM_NEXTDLGCTL, 0, FALSE);
  }
  EnableWindow(hctl, bEnable);
}

// PATCH: Windows 98 combobox fix (common controls version < 6)
// http://blogs.msdn.com/b/oldnewthing/archive/2006/03/10/548537.aspx
// ! http://vbnet.mvps.org/index.html?code/comboapi/comboheight.htm
void AdjustComboBoxHeight(HWND hWndCmbBox, DWORD MaxVisItems) {
RECT rc;
  GetWindowRect(hWndCmbBox, &rc);
  rc.right -= rc.left;
  ScreenToClient(GetParent(hWndCmbBox), (POINT *) &rc);
  rc.bottom = (MaxVisItems + 2) * SendMessage(hWndCmbBox, CB_GETITEMHEIGHT, 0, 0);
  MoveWindow(hWndCmbBox, rc.left, rc.top, rc.right, rc.bottom, TRUE);
  // PATCH: enable integral height, ComboBox must be created with CBS_NOINTEGRALHEIGHT
  SetWindowLong(hWndCmbBox, GWL_STYLE, (GetWindowLong(hWndCmbBox, GWL_STYLE) | CBS_NOINTEGRALHEIGHT) ^ CBS_NOINTEGRALHEIGHT);
}

// it's not very optimized code but good for small Internet pages
#define MAX_BLOCK_SIZE 1024
BYTE *HTTPGetContent(TCHAR *url, DWORD *len) {
HINTERNET hOpen, hConn, hReq;
URL_COMPONENTS uc;
BYTE *result, *buf, *r;
DWORD sz;
  // init result
  result = NULL;
  *len = 0;
  // prepare URL string
  ZeroMemory(&uc, sizeof(uc));
  uc.dwStructSize = sizeof(uc);
  sz = lstrlen(url);
  uc.lpszHostName     = STR_ALLOC(sz);
  uc.dwHostNameLength = sz;
  uc.lpszUrlPath      = STR_ALLOC(sz);
  uc.dwUrlPathLength  = sz;
  sz = InternetCrackUrl(url, lstrlen(url), 0, &uc);
  // sanity check
  if (sz && ((uc.nScheme == INTERNET_SCHEME_HTTPS) || (uc.nScheme == INTERNET_SCHEME_HTTP)) &&
      uc.lpszHostName && uc.lpszUrlPath && uc.lpszHostName[0] && uc.lpszUrlPath[0]
  ) {
    // open
    hOpen = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hOpen) {
      // connect
      sz = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
      hConn = InternetConnect(hOpen, uc.lpszHostName, sz, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
      if (hConn) {
        // request
        sz = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? (INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID) : 0;
        hReq = HttpOpenRequest(hConn, NULL, uc.lpszUrlPath, HTTP_VERSION, NULL, NULL,
          INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES | // INTERNET_FLAG_NO_AUTO_REDIRECT |
          INTERNET_FLAG_NO_CACHE_WRITE | sz, 0);
        if (hReq) {
          // HttpSendRequest() didn't work with -1 as dwHeadersLength in Unicode build:
          // GetLastError() == 12150 ERROR_HTTP_HEADER_NOT_FOUND
          // https://msdn.microsoft.com/en-us/library/windows/desktop/aa384247.aspx
          sz = HttpSendRequest(hReq, TEXT("Connection: Close"), 17, NULL, 0);
          if (sz) {
            // check for Content-Length
            sz = sizeof(len[0]);
            uc.dwStructSize = 0;
            if (HttpQueryInfo(hReq, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, len, &sz, &uc.dwStructSize)) {
              result = (BYTE *) GetMem(*len + 1);
              if (result) {
                sz = 0;
                InternetReadFile(hReq, result, *len, &sz);
              }
            } else {
              // length unknown
              buf = GetMem(MAX_BLOCK_SIZE);
              if (buf) {
                do {
                  sz = 0;
                  InternetReadFile(hReq, buf, MAX_BLOCK_SIZE, &sz);
                  if (sz) {
                    r = GrowMem(result, *len + sz + 1);
                    if (r) {
                      result = r;
                      CopyMemory(&result[*len], buf, sz);
                    } else {
                      // not enough memory
                      FreeMem(result);
                      result = NULL;
                      *len = 2;
                      break;
                    }
                    *len += sz;
                  }
                } while (sz);
                FreeMem(buf);
              }
            }
            // add 0 at the end for text buffer data
            if (result && *len) {
              result[*len] = 0;
            }
          } else {
            // v1.1
            // Windows XP, TLS not enabled in Internet Explorer
            if (uc.nScheme == INTERNET_SCHEME_HTTPS) {
              *len = GetLastError();
              *len = (
                (*len == ERROR_INTERNET_CANNOT_CONNECT) ||
                (*len == ERROR_INTERNET_CONNECTION_RESET)
              ) ? 1 : 0;
            }
          }
          InternetCloseHandle(hReq);
        }
        InternetCloseHandle(hConn);
      }
      InternetCloseHandle(hOpen);
    }
  }
  if (uc.lpszHostName) { FreeMem(uc.lpszHostName); }
  if (uc.lpszUrlPath)  { FreeMem(uc.lpszUrlPath);  }
  return(result);
}
