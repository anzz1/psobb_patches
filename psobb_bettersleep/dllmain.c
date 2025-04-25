// BetterSleep 1.25.13 (59NL)

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _NO_CRT_STDIO_INLINE
#include <windows.h>
#include "util.h"

// DWORD __usercall mysleep@<ecx>(DWORD lowPart@<edx>, DWORD highPart@<eax>)
void __declspec(naked) mysleep()
{
  __asm {
    push    ebx
    push    esi
    push    edi
    mov     esi, dword ptr ds:[0x00ACBE50]
    mov     ebx, dword ptr ds:[0x00ACBE54]
    sub     edx, dword ptr ds:[0x00ACBE48]
    sbb     eax, dword ptr ds:[0x00ACBE4C]
    cmp     ebx, eax
    jg      _compare
    jl      _break_loop
    cmp     esi, edx
    jnb     _compare

    _break_loop:
    xor     ecx, ecx
    pop     edi
    pop     esi
    pop     ebx
    ret

    _compare:
    mov     edi, dword ptr ds:[0x00ACBE58]
    mov     ecx, dword ptr ds:[0x00ACBE5C]
    shld    ecx, edi, 1
    add     edi, edi
    sub     esi, edi
    sbb     ebx, ecx
    cmp     ebx, eax
    jl      _sleep_0
    jg      _sleep_1
    cmp     esi, edx
    jbe     _sleep_0

    _sleep_1:
    mov     ecx, 2
    pop     edi
    pop     esi
    pop     ebx
    ret

    _sleep_0:
    mov     ecx, 1
    pop     edi
    pop     esi
    pop     ebx
    ret
  }
}

static void patch_sleep(void) {
  if (*(ULONGLONG*)0x0082D1D0 != 0x15FF5568246C8B50 || *(ULONGLONG*)0x00482E20 != 0x000000A46C7205C7) // already patched
    return;

  *(DWORD*)0x0070CBAC = 0x90909090;     // timeBeginPeriod(1) -> nop
  *(DWORD*)0x0070CBB0 = 0x90909090;     // ^
  *(DWORD*)0x0070C25D = 0x90909090;     // timeEndPeriod(1) -> nop
  *(DWORD*)0x0070C261 = 0x90909090;     // ^
  ((UINT (__stdcall *)(UINT)) (void**)*((DWORD*)0x008F83BC))(1); // timeBeginPeriod(1);

  *(WORD*)0x0082C453 = 0x5250;          // push eax; push edx
  *(BYTE*)0x0082C455 = 0xE8;            // call
  *(ULONG_PTR*)(0x0082C456) = calc_disp32(0x0082C456, (ULONG_PTR)mysleep);
  memset((void*)0x0082C45A, 0x90, 23);  // nop x23
  *(WORD*)0x0082C471 = 0x585A;          // pop edx; pop eax
  *(DWORD*)0x0082C473 = 0x1174C985;     // test ecx, ecx; je 0x17
  *(WORD*)0x0082C477 = 0x5149;          // dec ecx; push ecx
  *(WORD*)0x0082C47E = 0x9090;          // nop x2
}

__declspec(dllexport) void __stdcall load(void) {
  if (GetImageSize(0) < 0x00762000 || *(DWORD*)0x00B613FA != 0x4C4E3935) { // 59NL
    MessageBox(0, "BetterSleep: Wrong client version, expected MTethVer12513 (1.25.13)", "Error", MB_ICONERROR);
    return;
  }

  patch_sleep();
}

int __stdcall DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH)
    DisableThreadLibraryCalls(hInstDLL);

  return TRUE;
}
