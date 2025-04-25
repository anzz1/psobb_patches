#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

static const char default_config[] =
"MSAA=1\r\n"
"SMAA=1\r\n"
"SSAO=1\r\n"
"CelShader=1\r\n"
"DOF=1\r\n"
"HDR=1\r\n";

BOOL g_bMSAA = 1;
BOOL g_bSMAA = 1;
BOOL g_bSSAO = 1;
BOOL g_bCelShader = 1;
BOOL g_bDOF = 1;
BOOL g_bHDR = 1;

static void write_default_config(const char* path) {
  HANDLE hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
  if (!hFile || hFile == INVALID_HANDLE_VALUE) return;
  DWORD w = 0;
  WriteFile(hFile, default_config, sizeof(default_config)-1, &w, NULL);
  CloseHandle(hFile);
}

// s2 should be in lowercase
__forceinline static char* __stristr(const char* s1, const char* s2) {
  unsigned int i;
  char *p;
  for (p = (char*)s1; *p != 0; p++) {
    i = 0;
    do {
      if (s2[i] == 0) return p;
      if (p[i] == 0) break;
      if (s2[i] != ((p[i]>64 && p[i]<91) ? (p[i]+32):p[i])) break;
    } while (++i);
  }
  return 0;
}

static BOOL parse_option(char* ptr, const char* option, BOOL* out) {
  char* p = ptr;
  if (__stristr(p, option)) {
    while (*p && *p != '=') p++;
    if (*p == '=') {
      p++;
      while (*p && (*p == ' ' || *p == '\t')) p++;
      if (*p) {
        if ((p[0] == 'o' || p[0] == 'O') &&
            (p[1] == 'n' || p[1] == 'N')) {
          *out = 1;
          return 1;
        }
        if ((p[0] == 't' || p[0] == 'T') &&
            (p[1] == 'r' || p[1] == 'R') &&
            (p[2] == 'u' || p[2] == 'U') &&
            (p[3] == 'e' || p[3] == 'E')) {
          *out = 1;
          return 1;
        }
        if (p[0] == '1') {
          *out = 1;
          return 1;
        }
        if ((p[0] == 'o' || p[0] == 'O') &&
            (p[1] == 'f' || p[1] == 'F') &&
            (p[2] == 'f' || p[2] == 'F')) {
          *out = 0;
          return 1;
        }
        if ((p[0] == 'f' || p[0] == 'F') &&
            (p[1] == 'a' || p[1] == 'A') &&
            (p[2] == 'l' || p[2] == 'L') &&
            (p[3] == 's' || p[3] == 'S') &&
            (p[4] == 'e' || p[4] == 'E')) {
          *out = 0;
          return 1;
        }
        if (p[0] == '0') {
          *out = 0;
          return 1;
        }
      }
    }
  }
  return 0;
}

static void parse_line(char* line) {
  char* p = line;
  while (*p && (*p == ' ' || *p == '\t')) p++;
  if (*p == 0 || *p == '#' || *p == ';' || *p == '/') return;
  if (parse_option(p, "msaa", &g_bMSAA)) return;
  if (parse_option(p, "smaa", &g_bSMAA)) return;
  if (parse_option(p, "ssao", &g_bSSAO)) return;
  if (parse_option(p, "celshader", &g_bCelShader)) return;
  if (parse_option(p, "dof", &g_bDOF)) return;
  if (parse_option(p, "hdr", &g_bHDR)) return;
}

static void load_config(void) {
  char *pos1, *pos2;
  char *data;
  char *data_end;
  DWORD dwFileSize = 0;
  DWORD dwBytesRead = 0;
  HANDLE hFile;
  char path[MAX_PATH+1];
  if (!GetModuleFileNameA(0, path, MAX_PATH)) return;
  path[MAX_PATH] = 0;
  char *p = strrchr(path, '\\');
  if (!p || p-path > MAX_PATH-16) return;
  strcpy(++p, "widescreen.cfg");

  hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
  if (!hFile || hFile == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      write_default_config(path);
    }
    return;
  }
  dwFileSize = GetFileSize(hFile, 0);
  if (dwFileSize > 0 && dwFileSize != INVALID_FILE_SIZE) {
    data = (char*)MALLOC(dwFileSize+1);
    if (data) {
      data[dwFileSize] = 0;
      if (ReadFile(hFile, data, dwFileSize, &dwBytesRead, 0)) {
        data_end = data+dwFileSize-1;
        pos1 = data;
        if (dwFileSize > 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
          pos1 += 3; // skip UTF-8 BOM
        }
        pos2 = pos1;
        while (pos2 <= data_end) {
          if (*pos2 == '\r' || *pos2 == '\n') {
            *pos2++ = 0;
            parse_line(pos1);
            while (*pos2 == '\r' || *pos2 == '\n') pos2++;
            pos1 = pos2;
          }
          else {
            pos2++;
          }
        }
        parse_line(pos1);
      }
      FREE(data);
    }
  }
  CloseHandle(hFile);
}

static void SetWorkingDir(void) {
  char path[MAX_PATH+1];
  if (!GetModuleFileNameA(0, path, MAX_PATH)) return;
  path[MAX_PATH] = 0;
  char *p = strrchr(path, '\\');
  if (!p) return;
  *p = 0;
  SetCurrentDirectoryA(path);
}

void load_options(void) {
  SetWorkingDir();
  load_config();
}
