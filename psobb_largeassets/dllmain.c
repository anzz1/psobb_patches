// LargeAssets 1.25.13 (59NL)
// Credits to Solybum for the patch

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _NO_CRT_STDIO_INLINE
#include <windows.h>
#include <stdlib.h>
#include "util.h"

#define NEW_ASSET_SIZE_LIMIT  100000000

ULONG listAssetSizeLimit[] = {
  0x00800C32,
  0x005B7CFC,
  0x005B80F8,
  0x005B913F,
  0x005B7215,
  0x005B7937,
  0x005B97C2,
  0x005BA613,
  0x005BB405,
  0x005B77E3,
  0x005C74C1,
  0x0070EB5B,
  0x00800A34,
  0x005B7CFC,
  0x005B82AD,
  0x005BB40C,
  0x005E581E,
  0x007A6573,
  0x0081A965
};

static void patch_asset_limits(void) {
  if (*(DWORD*)0x00800C32 != 0x00090000 || *(DWORD*)0x005B7CFC != 0x00090000) // already patched
    return;

  for (int i = 0; i < _countof(listAssetSizeLimit); i++) {
    *(DWORD*)listAssetSizeLimit[i] = NEW_ASSET_SIZE_LIMIT;
  }
}

__declspec(dllexport) void __stdcall load(void) {
  if (GetImageSize(0) < 0x00762000 || *(DWORD*)0x00B613FA != 0x4C4E3935) { // 59NL
    MessageBoxA(0, "LargeAssets: Wrong client version, expected MTethVer12513 (1.25.13)", "Error", MB_ICONERROR);
    return;
  }

  patch_asset_limits();
}

int __stdcall DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH)
    DisableThreadLibraryCalls(hInstDLL);

  return TRUE;
}
