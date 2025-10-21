#include <windows.h>
#include <wininet.h>
#include <commctrl.h>
#include <shlwapi.h>
#include "resource/TwitchXP.h"
#include "SysToolX.h"
#include "M3U8JSON.h"
#include "IniFiles.h"

TCHAR *OpenSaveDialogEx(HWND wnd, DWORD msk, TCHAR *name, int savedlg) {
TCHAR buf[1025], *result, *s;
DWORD i, l;
  result = NULL;
  l = 0;
  buf[0] = 0;
  // only 4 bits
  msk &= 0xF;
  // any files always present
  msk |= 1 << (IDS_MSK_ANY - 1);
  // add strings
  for (i = 0; msk; msk >>= 1) {
    i++;
    // bit is set?
    if (msk & 1) {
      s = LangLoadString(i);
      if (s) {
        lstrcpyn(&buf[l], s, 1025 - l);
        l += lstrlen(s);
        FreeMem(s);
      }
    }
    // buf string too short
    if (l >= 1025) { break; }
  }
  // replace '|' with nulls
  for (s = buf; *s; s++) {
    if (*s == TEXT('|')) {
       *s = 0;
    }
  }
  // get default extension
  for (s = buf; *s; s++);
  for (s++; *s; s++) {
    // no multiextension
    if (*s == TEXT(';')) {
      s = NULL;
      break;
    }
  }
  if (s) {
    for (s--; *s; s--) {
      if (*s == TEXT('.')) {
        break;
      }
      if ((*s == TEXT('*')) || (*s == TEXT('?')) || (*s == TEXT(';'))) {
        s = NULL;
        break;
      }
    }
    if (s) { s++; }
  }
  result = OpenSaveDialog(wnd, buf, s, name, savedlg);
  return(result);
}

TCHAR *GetChannelName(HWND wnd) {
TCHAR *s, b[1025];
URL_COMPONENTS uc;
DWORD i;
  s = GetWndText(wnd);
  if (s) {
    // trim
    StTrim(s);
    // non-empty string
    if (*s) {
      // prepare URL string
      ZeroMemory(&uc, sizeof(uc));
      uc.dwStructSize = sizeof(uc);
      *b = 0;
      uc.lpszUrlPath = b;
      uc.dwUrlPathLength = 1025;
      // cracked successfully
      if (
        InternetCrackUrl(s, lstrlen(s), 0, &uc) && (uc.lpszUrlPath[0] == TEXT('/')) &&
        ((uc.nScheme == INTERNET_SCHEME_HTTPS) || (uc.nScheme == INTERNET_SCHEME_HTTP))
      ) {
        // remove slash
        b[0] = TEXT(' ');
        // remove parms (if any)
        for (i = 1; b[i]; i++) {
          if ((b[i] == TEXT('?')) || (b[i] == TEXT('/')) || (b[i] == TEXT('#'))) {
            b[i] = 0;
            break;
          }
        }
        // trim string
        StTrim(b);
        // safe since cracked string will be shorter than original
        CopyMemory(s, b, (lstrlen(b) + 1) * sizeof(b[0]));
      }
    }
    // still non-empty
    if (*s) {
      // set normalized text
      SetWindowText(wnd, s);
    } else {
      // empty string
      FreeMem(s);
      s = NULL;
    }
  }
  return(s);
}

// short and simply to understand code for Twitch.tv protocol can be found here:
// https://github.com/systoolz/miscsoft/blob/master/twitchtv.php
// max def stack size: 3984
CCHAR *TwitchPlayList(TCHAR *name) {
CCHAR *r, *chn, *buf, *s, *cid, *qry, *did;
DWORD i;
  r = NULL;
  // temporary values
  chn = NULL;
  buf = NULL;
  s = NULL;
  cid = NULL;
  qry = NULL;
  did = NULL;
  do {
    // sanity check
    if ((!name) || (!name[0])) { break; }
    // lazy Unicode to ANSI
    i = lstrlen(name);
    chn = (CCHAR *) GetMem(i + 1);
    if (!chn) { break; }
    chn[i] = 0;
    while (i) {
      i--;
      chn[i] = name[i];
    }
    buf = StrTplFmtA("https://www.twitch.tv/\x01", chn);
    if (!buf) { break; }
    s = (CCHAR *) HTTPGetContent(buf, &i, NULL, NULL);
    if (!s) {
      r = (CCHAR *) i; // v1.3
      break;
    }
    cid = GetWildMatchStr(s, "clientId = \"*\"");
    if (!cid) { break; }
    qry = GetWildMatchStr(s, "var query = '*'");
    if (!qry) { break; }
    DebugWnd(TEXT("clientId: %hs"), cid);
    DebugWnd(TEXT("query: %hs"), qry);
    // build deviceId (anything)
    did = (CCHAR *) GetMem(32 + 1);
    if (!did) { break; }
    for (i = 0; i < 32; i++) {
      did[i] = (GetTickCount() * i) & 0x0F;
      did[i] += (did[i] < 10) ? '0' : ('a' - 10);
      Sleep(1); // update GetTickCount()
    }
    did[32] = 0;
    // build headers
    FreeMem(buf);
    buf = StrTplFmtA("Client-ID: \x01\r\nDevice-ID: \x01\r\n", cid, did);
    // build POST data
    FreeMem(cid);
    cid = AddSlashesStr(qry);
    if (!cid) { break; }
    FreeMem(did);
    did = StrTplFmtA(
      "{\"operationName\":\"PlaybackAccessToken_Template\","
      "\"query\":\"\x01\","
      "\"variables\":{"
        "\"isLive\":true,"
        "\"login\":\"\x01\","
        "\"isVod\":false,"
        "\"vodID\":\"\","
        "\"playerType\":\"site\","
        "\"platform\":\"web\""
        "}"
      "}",
      cid, chn
    );
    if (!did) { break; }
    FreeMem(s);
    s = (CCHAR *) HTTPGetContent("https://gql.twitch.tv/gql", &i, buf, did);
    if (!s) {
      r = (CCHAR *) i; // v1.3
      break;
    }
    // get signature and token
    FreeMem(cid);
    cid = JSONParserStr(s, "\"data\"");
    if (!cid) { break; }
    DebugWnd(TEXT("data: %hs"), cid);
    FreeMem(buf);
    buf = JSONParserStr(cid, "\"streamPlaybackAccessToken\"");
    if (!buf) { break; }
    DebugWnd(TEXT("streamPlaybackAccessToken: %hs"), buf);
    FreeMem(cid);
    cid = JSONParserStr(buf, "\"signature\"");
    if (!cid) { break; }
    DebugWnd(TEXT("signature: %hs"), cid);
    FreeMem(did);
    did = JSONParserStr(buf, "\"value\"");
    if (!did) { break ;}
    DebugWnd(TEXT("value: %hs"), did);
    // old: usher.twitch.tv
    FreeMem(buf);
    buf = StrTplFmtA(
      "https://usher.ttvnw.net/api/channel/hls/\x01.m3u8"
      "?allow_source=true&fast_bread=true&p=\x02"
      "&playlist_include_framerate=true&reassignments_supported=true&sig=\x01"
      "&token=\x01",
      chn,
      GetTickCount(),
      cid,
      did
    );
    if (!buf) { break; }
    // get playlist
    FreeMem(s);
    s = (CCHAR *) HTTPGetContent(buf, &i, NULL, NULL);
    if (!s) {
      r = (CCHAR *) i; // v1.3
      break;
    }
    r = s;
  } while (0);
  // cleanup
  if (did) { FreeMem(did); }
  if (qry) { FreeMem(qry); }
  if (cid) { FreeMem(cid); }
  if ((!r) && (s)) { FreeMem(s); }
  if (buf) { FreeMem(buf); }
  if (chn) { FreeMem(chn); }
  return(r);
}

void FreeTwitchList(HWND wnd) {
int i, sz, k;
  sz = SendMessage(wnd, CB_GETCOUNT, 0, 0);
  for (i = 0; i < sz; i++) {
    k = SendMessage(wnd, CB_GETITEMDATA, i, 0);
    if (k && (k != CB_ERR)) {
      FreeMem((void *) k);
    }
  }
  SendMessage(wnd, CB_RESETCONTENT, 0, 0);
}

TCHAR *StDupConv(CCHAR *s) {
TCHAR *r;
DWORD i, sz;
  r = NULL;
  if (s) {
    sz = lstrlenA(s) + 1;
    r = (TCHAR *) GetMem(sz * sizeof(r[0]));
    if (r) {
      // lazy UTF-8 to ANSI/Unicode conversion
      for (i = 0; i < sz; i++) {
        r[i] = s[i];
      }
    }
  }
  return(r);
}

void PutWndData(HWND wnd, void *d) {
void *p;
  p = (void *) GetWindowLong(wnd, GWL_USERDATA);
  // free old text (if any)
  if (p) { FreeMem(p); }
  // save new text (if any)
  SetWindowLong(wnd, GWL_USERDATA, (LONG) d);
}

void *GetWndData(HWND wnd) {
  return((void *) GetWindowLong(wnd, GWL_USERDATA));
}

void SavePlayList(HWND wnd, TCHAR *name) {
CCHAR *r;
TCHAR *s;
HANDLE fl;
DWORD dw;
  // is playlist saved
  r = (CCHAR *) GetWndData(GetDlgItem(wnd, IDC_SAVE_IT));
  if (r) {
    // ask for filename
    s = OpenSaveDialogEx(wnd, IDS_MSK_M3U, name, 1);
    if (s) {
      // dump playlist to disk
      fl = CreateFile(s, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      if (fl != INVALID_HANDLE_VALUE) {
        WriteFile(fl, r, lstrlenA(r), &dw, NULL);
        CloseHandle(fl);
      }
      FreeMem(s);
    }
  }
}

DWORD LoadTwitchList(HWND wnd, TCHAR *s) {
CCHAR *r, *p, *b;
DWORD result, l;
int i;
  result = 1; // error load http data
  r = TwitchPlayList(s);
  // v1.1
  if (!HIWORD(r)) {
    result = LOWORD(r) ? LOWORD(r) : result; // v1.3
    r = NULL;
  }
  if (r) {
    result = 2; // invalid playlist format
    l = lstrlenA(r);
    b = (CCHAR *) GetMem((l + 1) * sizeof(b[0]));
    l /= 2;
    if (b) {
      p = r;
      *b = 0;
      b[10] = 0;
      p = M3U8Parser(p, NULL, b, 7+1);
      p = M3U8Parser(p, NULL, &b[10], 18+1);
      // format check
      if ((!lstrcmpA(b, "#EXTM3U")) && (!lstrcmpA(&b[10], "#EXT-X-TWITCH-INFO"))) {
        // save playlist in save button
        PutWndData(GetDlgItem(GetParent(wnd), IDC_SAVE_IT), r);
        FreeTwitchList(wnd);
        result = 0; // successful
        // fill in combobox
        while (*p) {
          p = M3U8Parser(p, "#EXT-X-MEDIA\0NAME", b, l);
          p = M3U8Parser(p, "#EXT-X-STREAM-INF\0RESOLUTION", &b[l], l);
          wsprintfA(b, "%s%c| %s", *b ? b : "???", b[l] ? ' ' : 0, &b[l]);
          i = SendMessageA(wnd, CB_ADDSTRING, 0, (LPARAM) b);
          p = M3U8Parser(p, NULL, b, l);
          if (i >= 0) {
            SendMessage(wnd, CB_SETITEMDATA, i, (LPARAM) StDupConv(b));
          }
        }
        // set default selection
        s = (TCHAR *) GetWndData(GetDlgItem(GetParent(wnd), IDC_QUALITY));
        SendMessage(wnd, CB_SETCURSEL, s ? SendMessage(wnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) s) : -1, 0);
      } else {
        DebugWnd(TEXT("server playlist reply: %hs"), r);
        // error - free buffer
        FreeMem(r);
      }
      FreeMem(b);
    }
  }
  return(result);
}

void PlayTwitchLink(HWND wnd) {
TCHAR *s, *l, *x;
DWORD k;
int i;
  // something actually selected
  l = NULL;
  i = SendMessage(GetDlgItem(wnd, IDC_QUALITY), CB_GETCURSEL, 0, 0);
  if (i >= 0) {
    i = SendMessage(GetDlgItem(wnd, IDC_QUALITY), CB_GETITEMDATA, i, 0);
    if (i && (i != CB_ERR)) {
      l = (TCHAR *) i;
    }
  } else {
    MsgBox(wnd, MAKEINTRESOURCE(IDS_MSG_NOTSLCT), MB_ICONINFORMATION | MB_OK);
    SetFocus(GetDlgItem(wnd, IDC_QUALITY));
  }
  if (l) {
    x = NULL;
    s = NULL;
    // custom or system video player
    if (IsDlgButtonChecked(wnd, IDC_PL_CUST) == BST_CHECKED) {
      // custom player
      s = GetWndText(GetDlgItem(wnd, IDC_PL_LINE));
      if (s) {
        x = StrTplFmt(TEXT("\"\x01\" \"\x01\""), s, l);
        if (x) {
          WinExecFile(x, (IsDlgButtonChecked(wnd, IDC_PL_HIDE) == BST_CHECKED) ? SW_HIDE : SW_SHOWNORMAL);
        }
      }
    } else {
      // system player
      k = 0;
      for (i = 0; i < 2; i++) {
        // PATCH: invalid headers of GCC 3.2 - first enum ASSOCSTR_COMMAND must be 1, not 0
        AssocQueryString(0, 2, TEXT(".AVI"), TEXT("open"), s, &k);
        if (!i) {
          s = (TCHAR *) GetMem((k + 1) * sizeof(x[0]));
          if (!s) { break; }
          *s = 0;
        }
      }
      if (s && *s) {
        x = StrTplFmt(TEXT("\"\x01\" \"\x01\""), s, l);
        if (x) {
          WinExecFile(x, SW_SHOWNORMAL);
        }
      } else {
        MsgBox(wnd, MAKEINTRESOURCE(IDS_MSG_NOTPLAY), MB_ICONERROR | MB_OK);
      }
    }
    if (s) { FreeMem(s); }
    if (x) { FreeMem(x); }
  }
}

const static TCHAR stConfig[] = TEXT(
  ".\\TwitchXP.ini\0"
  "main\0"
  "player\0"
  "hidden\0"
  "select\0"
  "custom\0"
  "recent\0"
  "\0"
);

void SettingsHandler(HWND wnd, BOOL bsave) {
ini_data id;
DWORD i;
TCHAR *s, z, *t;
  // init config
  IniInit(&id, stConfig);
  if (bsave) {
    z = 0;
    // player
    IniPutInt(&id, (IsDlgButtonChecked(wnd, IDC_PL_CUST) == BST_CHECKED) ? 1 : 0);
    //  hidden
    IniPutInt(&id, (IsDlgButtonChecked(wnd, IDC_PL_HIDE) == BST_CHECKED) ? 1 : 0);
    // select
    s = GetWndText(GetDlgItem(wnd, IDC_QUALITY));
    t = NULL;
    if (s && *s) {
      t = s;
    } else {
      t = GetWndData(GetDlgItem(wnd, IDC_QUALITY));
    }
    IniPutStr(&id, t ? t : &z);
    if (s) { FreeMem(s); }
    // custom
    s = GetWndText(GetDlgItem(wnd, IDC_PL_LINE));
    IniPutStr(&id, s ? s : &z);
    if (s) { FreeMem(s); }
    // recent
    s = GetWndText(GetDlgItem(wnd, IDC_CHANNEL));
    IniPutStr(&id, s ? s : &z);
    if (s) { FreeMem(s); }
  } else {
    // player
    i = IniGetInt(&id) ? IDC_PL_CUST : IDC_PL_SYST;
    CheckRadioButton(wnd, IDC_PL_SYST, IDC_PL_CUST, i);
    SendMessage(wnd, WM_COMMAND, (WPARAM)MAKELONG(i, BN_CLICKED), (LPARAM) GetDlgItem(wnd, i));
    // hidden
    CheckDlgButton(wnd, IDC_PL_HIDE, IniGetInt(&id) ? BST_CHECKED : BST_UNCHECKED);
    // select
    s = IniGetStr(&id);
    if (s) {
      PutWndData(GetDlgItem(wnd, IDC_QUALITY), (void *) s);
    }
    // custom
    s = IniGetStr(&id);
    if (s) {
      SetDlgItemText(wnd, IDC_PL_LINE, s);
      FreeMem(s);
    }
    // recent
    s = IniGetStr(&id);
    if (s) {
      SetDlgItemText(wnd, IDC_CHANNEL, s);
      FreeMem(s);
    }
  }
}

// https://msdn.microsoft.com/en-us/library/windows/desktop/bb760250.aspx
// https://msdn.microsoft.com/en-us/library/windows/desktop/bb760252.aspx
// https://msdn.microsoft.com/en-us/library/windows/desktop/hh298368.aspx
void InitToolTip(HWND wnd) {
HWND httw;
TOOLINFO ti;
DWORD i;
  if (IsWindow(wnd)) {
    // Init structure
    ZeroMemory(&ti, sizeof(ti));
    ti.hinst = GetModuleHandle(NULL);
    // Create the tooltip
    httw = CreateWindowEx(
      0, TOOLTIPS_CLASS, NULL,
      WS_POPUP | TTS_ALWAYSTIP,
      CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT,
      wnd, NULL, 
      ti.hinst, NULL
    );
    if (httw) {
      ti.cbSize = sizeof(ti);
      ti.hwnd = wnd;
      ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
      for (i = IDC_FIRSTID; GetDlgItem(wnd, i); i++) {
        ti.uId = (UINT_PTR) GetDlgItem(wnd, i);
        ti.lpszText = MAKEINTRESOURCE(i - IDC_FIRSTID + IDS_TTT_FIRSTID);
        SendMessage(httw, TTM_ADDTOOL, 0, (LPARAM) &ti);
      }
      // activate
      SendMessage(wnd, TTM_ACTIVATE, (WPARAM) TRUE, 0);
      // save handle
      SetWindowLong(wnd, GWL_USERDATA, (LONG) httw);
    }
  }
}

void FreeToolTip(HWND wnd) {
HWND httw;
  if (IsWindow(wnd)) {
    httw = (HWND) GetWindowLong(wnd, GWL_USERDATA);
    if (IsWindow(httw)) {
      // remove from userdata
      SetWindowLong(wnd, GWL_USERDATA, 0);
      // deactivate
      SendMessage(wnd, TTM_ACTIVATE, (WPARAM) FALSE, 0);
      // destroy
      SendMessage(wnd, WM_CLOSE, 0, 0);
    }
  }
}

// v1.2
void TryToAutoPlay(HWND wnd) {
HWND wh;
  wh = GetDlgItem(wnd, IDC_QUALITY);
  // window enabled
  if (IsWindowEnabled(wh)) {
    // something selected
    if (SendMessage(wh, CB_GETCURSEL, 0, 0) >= 0) {
      // click play
      PostMessage(wnd, WM_COMMAND, MAKELONG(IDC_PLAY_IT, BN_CLICKED), 0);
    } else {
      // else set focus here to select quality
      SetFocus(wh);
    }
  }
}

// v1.2
void ItemsState(HWND wnd, const WORD *items, BOOL state) {
  if (items) {
    for (; *items; items++) {
      DialogEnableWindow(wnd, *items, state);
    }
  }
}

// v1.2
const static WORD MainItems[] = {IDC_QUALITY, IDC_PLAY_IT, IDC_SAVE_IT, 0};
const static WORD PlayItems[] = {IDC_PL_LINE, IDC_PL_BRWS, IDC_PL_HIDE, 0};

BOOL CALLBACK DlgPrc(HWND wnd, UINT msg, WPARAM wparm, LPARAM lparm) {
DRAWITEMSTRUCT *dis;
BOOL result;
TCHAR *s, *d;
DWORD i;
  result = FALSE;
  switch (msg) {
    case WM_INITDIALOG:
      // add icons
      SendMessage(wnd, WM_SETICON, ICON_BIG  , (LPARAM) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICN)));
      SendMessage(wnd, WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICN)));
      // fix combobox height
      AdjustComboBoxHeight(GetDlgItem(wnd, IDC_QUALITY), 10);
      // disable controls
      ItemsState(wnd, MainItems, FALSE);
      // load settings (also sets radiogroup)
      SettingsHandler(wnd, FALSE);
      // create hints
      InitToolTip(wnd);
      // v1.8 init debug window
      DebugWnd(NULL, GetDlgItem(wnd, IDC_LOGINFO));
      // hide debug window
      ShowWindow(GetDlgItem(wnd, IDC_LOGINFO), SW_HIDE);
      // must be true
      result = TRUE;
      break;
    case WM_COMMAND:
      if (HIWORD(wparm) == BN_CLICKED) {
        switch (LOWORD(wparm)) {
          // v1.2
          case IDOK:
            // only if focus on the channel edit control
            if (GetFocus() == GetDlgItem(wnd, IDC_CHANNEL)) {
              // flag for autoplay
              PutWndData(wnd, (void *) 1);
              // click Get
              PostMessage(wnd, WM_COMMAND, MAKELONG(IDC_GET_LNK, BN_CLICKED), 0);
            } else {
              // try to play
              TryToAutoPlay(wnd);
            }
            break;
          // exit from program
          case IDCANCEL:
            // save settings
            SettingsHandler(wnd, TRUE);
            // cleanup
            FreeToolTip(wnd);
            FreeTwitchList(GetDlgItem(wnd, IDC_QUALITY));
            PutWndData(GetDlgItem(wnd, IDC_SAVE_IT), NULL);
            PutWndData(GetDlgItem(wnd, IDC_QUALITY), NULL);
            // end dialog
            EndDialog(wnd, 0);
            break;
        }
        // v1.2 - split switch into two parts to avoid generation of long and useless jump table
        switch (LOWORD(wparm)) {
          case IDC_PUT_LNK:
            // put link from the clipboard
            SendDlgItemMessage(wnd, IDC_CHANNEL, EM_SETSEL, 0, -1);
            SendDlgItemMessage(wnd, IDC_CHANNEL, WM_PASTE, 0, 0);
            break;
          case IDC_GET_LNK:
            // v1.2 disable buttons from previous run
            ItemsState(wnd, MainItems, FALSE);
            // get link playlist
            s = GetChannelName(GetDlgItem(wnd, IDC_CHANNEL));
            i = 1;
            if (s) {
              DebugWnd(TEXT("~channel: %s"), s);
              i = LoadTwitchList(GetDlgItem(wnd, IDC_QUALITY), s);
              if (!i) {
                // enable controls
                ItemsState(wnd, MainItems, TRUE);
                // v1.2 autoplay
                if ((DWORD)GetWndData(wnd) == 1) {
                  // remove flag
                  PutWndData(wnd, NULL);
                  // try to play
                  TryToAutoPlay(wnd);
                }
              } else {
                i += 1;
              }
              FreeMem(s);
            }
            // some error
            if (i) {
              MsgBox(wnd, MAKEINTRESOURCE(IDS_MSG_INVALID + i - 1), MB_ICONERROR);
              SetFocus(GetDlgItem(wnd, IDC_CHANNEL));
            }
            break;
          case IDC_PLAY_IT:
            PlayTwitchLink(wnd);
            break;
          case IDC_SAVE_IT:
            s = GetChannelName(GetDlgItem(wnd, IDC_CHANNEL));
            if (s) { SafeFileName(s); }
            SavePlayList(wnd, s);
            if (s) { FreeMem(s); }
            break;
          // radiogroup
          case IDC_PL_SYST:
          case IDC_PL_CUST:
            i = (LOWORD(wparm) == IDC_PL_CUST) ? TRUE : FALSE;
            ItemsState(wnd, PlayItems, i);
            break;
          case IDC_PL_BRWS:
            // select external player file
            s = OpenSaveDialogEx(wnd, IDS_MSK_EXE, NULL, 0);
            if (s) {
              SetDlgItemText(wnd, IDC_PL_LINE, s);
              // scroll text in edit box to the end, so user can see a filename
              SendDlgItemMessage(wnd, IDC_PL_LINE, EM_SETSEL, 0, -1);
              FreeMem(s);
            }
            break;
          case IDC_PL_HIDE:
            if (IsDlgButtonChecked(wnd, IDC_PL_HIDE) == BST_CHECKED) {
              if (MsgBox(wnd, MAKEINTRESOURCE(IDS_MSG_YOUSURE), MB_ICONWARNING | MB_YESNO) != IDYES) {
                CheckDlgButton(wnd, IDC_PL_HIDE, BST_UNCHECKED);
              }
            }
            break;
          case IDC_SITELNK:
            // get control text
            s = GetWndText(GetDlgItem(wnd, LOWORD(wparm)));
            if (s) {
              // save original pointer
              d = s;
              // find link splitter
              for (; *s; s++) {
                // found it
                if (*s == TEXT('|')) {
                  break;
                }
              }
              // found?
              if (*s == TEXT('|')) {
                // remove space if any
                for (s++; *s == TEXT(' '); s++);
              }
              // open link
              if (*s) {
                URLOpenLink(wnd, s);
              }
              // free memory
              FreeMem(d);
            }
            break;
        }
      }
      break;
    case WM_DRAWITEM:
      dis = (DRAWITEMSTRUCT *) lparm;
      if (dis && (dis->CtlID == IDC_SITELNK)) {
        s = GetWndText(dis->hwndItem);
        if (s) {
          SetTextColor(dis->hDC, RGB(0, 0, 0xFF));
          SetBkMode(dis->hDC, TRANSPARENT);
          DrawText(dis->hDC, s, -1, &dis->rcItem, DT_LEFT | DT_TOP | DT_SINGLELINE);
          FreeMem(s);
          result = TRUE;
        }
      }
      break;
    case WM_SETCURSOR:
      if (((HWND) wparm) == GetDlgItem(wnd, IDC_SITELNK)) {
        SetCursor(LoadCursor(NULL, IDC_HAND));
        SetWindowLongPtr(wnd, DWLP_MSGRESULT, TRUE);
        result = TRUE;
      }
      break;
    // v1.8 toggle debug window
    case WM_HELP:
      i = IsWindowVisible(GetDlgItem(wnd, IDC_LOGINFO));
      ShowWindow(GetDlgItem(wnd, IDC_LOGINFO), i ? SW_HIDE : SW_SHOW);
      ShowWindow(GetDlgItem(wnd, IDC_SITELNK), i ? SW_SHOW : SW_HIDE);
      ShowWindow(GetDlgItem(wnd, IDC_DETAILS), i ? SW_SHOW : SW_HIDE);
      break;
  }
  return(result);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  InitCommonControls();
  DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DLG), 0, &DlgPrc, 0);
  ExitProcess(0);
  return(0);
}
