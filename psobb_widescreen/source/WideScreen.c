// WideScreenPatch 1.25.13
// Credits to tofuman for the original patch

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

ULONG listHUDWidth[] = {
  0x004011D2,
  0x004011EF,
  0x004013A7,
  0x00409F4B,
  0x0040A02B,
  0x0040A04B,
  0x0040C445,
  0x0040C469,
  0x0040C48B,
  0x00453809,
  0x0045381D,
  0x00483278,
  0x004EB4AA,
  0x004EB4F0,
  0x004EC2D1,
  0x004EF592,
  0x004EF59C,
  0x00502D7D,
  0x005BC90E,
  0x005BD783,
  0x005BDB89,
  0x007126AC,
  0x00719AB1,
  0x00719ACE,
  0x00719C5C,
  0x00719C6B,
  0x00719D44,
  0x00719D53,
  0x00719E84,
  0x0071A21F,
  0x00721E6C,
  0x00721E7A,
  0x00721F2A,
  0x00721FC0,
  0x0077F2ED,
  0x0077F30D,
  0x0077F386,
  0x0077F3A7,
  0x00785641,
  0x007856B1,
  0x00785BD2,
  0x007888A6,
  0x007888C6,
  0x00788EA6,
  0x00788EC6,
  0x00804EE0,
  0x008051BA,
  0x008F8494,
  0x008F853C,
  0x008F854C,
  0x008F8EAC,
  0x008F8EBC,
  0x008F8F50,
  0x008F9A94,
  0x008F9AA4,
  0x008F9AC4,
  0x008F9D38,
  0x008F9D68,
  0x008F9E8C,
  0x008F9EF8,
  0x008F9F2C,
  0x008F9F34,
  0x008F9F58,
  0x008FA8A8,
  0x008FAE50,
  0x008FAE90,
  0x008FFB84,
  0x009007B8,
  0x00909168,
  0x00909D70,
  0x00909D84,
  0x00909DA8,
  0x0091D8BC,
  0x0091DA80,
  0x0091E19C,
  0x0091FBF0,
  0x0091FC54,
  0x0091FC74,
  0x0091FF94,
  0x00920108,
  0x00920164,
  0x0092B6EC,
  0x0092B71C,
  0x0092B740,
  0x0092B780,
  0x0096F5E0,
  0x0096F694,
  0x0096FE84,
  0x0096FF60,
  0x0096FF9C,
  0x0096FFC0,
  0x009701F0,
  0x00970640,
  0x009706C0,
  0x009707F8,
  0x0097080C,
  0x009796F4,
  0x00979A34,
  0x00979A7C,
  0x00979A9C,
  0x00979DC4,
  0x00979E00,
  0x0097A170,
  0x0097A1AC,
  0x0097A1B8,
  0x0097A6F4,
  0x0097A740,
  0x0097BB0C,
  0x0097BB58,
  0x0097BBB0,
  0x0097EBBC,
  0x0097EC1C,
  0x0097EC4C,
  0x0098A160,
  0x0098A4A8,
  0x0098A4B8,
  0x009A3468,
  0x009A3488,
  0x009A6900,
  0x009A6908,
  0x009B8DC4,
  0x009B8DDC
};
ULONG listHUDHeight[] = {
  0x004011C0,
  0x004011DD,
  0x004013AF,
  0x0040A016,
  0x0040A036,
  0x00453827,
  0x0045383B,
  0x004EB4B4,
  0x004EB4FA,
  0x004EC2E9,
  0x00502D85,
  0x007126A5,
  0x00719AD9,
  0x00719AF6,
  0x00719C08,
  0x00719C16,
  0x00719C79,
  0x00719C8A,
  0x0071A226,
  0x0077F3B2,
  0x0077F3D2,
  0x00785648,
  0x00785680,
  0x007856B8,
  0x00785705,
  0x00785BD9,
  0x00785C11,
  0x00785C49,
  0x007888D1,
  0x007888F1,
  0x00788ED1,
  0x00788EF1,
  0x00804EF4,
  0x008051CE,
  0x008F9A98,
  0x008F9AC8,
  0x008FFB80,
  0x00909164,
  0x0091D8B8,
  0x0091DA7C,
  0x0091FBEC,
  0x0091FC50,
  0x0091FC70,
  0x0091FF90,
  0x00920050,
  0x00920104,
  0x00920160,
  0x0096F5D8,
  0x0096F698,
  0x0096FF5C,
  0x0096FFA8,
  0x0096FFBC,
  0x009701EC,
  0x00970648,
  0x00970660,
  0x009706C8,
  0x009707F4,
  0x00970808,
  0x009796EC,
  0x00979A28,
  0x00979A70,
  0x00979A8C,
  0x00979DC8,
  0x00979DFC,
  0x0097A16C,
  0x0097A1A8,
  0x0097A1BC,
  0x0097A6F0,
  0x0097A73C,
  0x0097E9A8,
  0x0097E9D8,
  0x0097E9E4,
  0x0097E9F0,
  0x0097EA00,
  0x0097EBB8,
  0x0097EC18,
  0x0097EC48,
  0x0098A15C,
  0x0098A4A4,
  0x0098A4B4,
  0x009A690C,
  0x009A6914,
  0x009B8D0C,
  0x009B8D54,
  0x00A98498,
  0x00A984B4,
  0x00A984D0,
  0x00A984EC,
  0x00A98508,
  0x00A98524
};
ULONG listCenterAlignItems[] = {
  0x0066DFEF,
  0x00791C7A,
  0x00792366,
  0x00972000,
  0x00972040,
  0x00972048,
  0x00972060,
  0x009720A8,
  0x009720E0,
  0x009720E8,
  0x009720F0,
  0x009720F8,
  0x00972100,
  0x00972188,
  0x00972508,
  0x00972518,
  0x00972530,
  0x00972548,
  0x00972550,
  0x00972570,
  0x00972578,
  0x00972580,
  0x00972620,
  0x00972698,
  0x00974E3C,
  0x00974E44,
  0x009FF50C
};
ULONG listRightAlignItems[] = {
  0x00783A92,
  0x00972010,
  0x00972050,
  0x00972058,
  0x00972540,
  0x009720B0,
  0x009720B8,
  0x00972078,
  0x009725E8,
  0x009725D0,
  0x00972610,
  0x00A11324,
  0x00A11390,
  0x00A11398,
  0x00A113E8
};
ULONG listVerticalBottomAlignItems[] = {
  0x00407F9D,
  0x0071A8F8,
  0x0071A905,
  0x0071A988,
  0x0071A995,
  0x007403F6,
  0x0074467D,
  0x0074468D,
  0x007446D8,
  0x007446E8,
  0x00744723,
  0x00744733,
  0x0074477E,
  0x0074478E,
  0x007583C8,
  0x007583E3,
  0x007583FE,
  0x00758412,
  0x007584F5,
  0x0075850F,
  0x00758526,
  0x0075853F,
  0x00761CC7,
  0x00761CD9,
  0x00762367,
  0x00762602,
  0x007627DF,
  0x00783AAA,
  0x008F87D8,
  0x008F87E0,
  0x0096E534,
  0x0096E53C,
  0x0096E54C,
  0x0096E574,
  0x0096E5FC,
  0x0096E660,
  0x0096FC44,
  0x0096FC54,
  0x0097061C,
  0x00970638,
  0x0097066C,
  0x009710BC,
  0x00971F44,
  0x00971F4C,
  0x00971F54,
  0x00971F5C,
  0x00971F64,
  0x00971f6c,
  0x00971f74,
  0x00971F7C,
  0x00971F84,
  0x00971F8C,
  0x00971f94,
  0x00971f9c,
  0x00971FA4,
  0x00971fac,
  0x00971FBC,
  0x00971FC4,
  0x00971FCC,
  0x00971FD4,
  0x00971FDC,
  0x00971FE4,
  0x00971FEC,
  0x00971FF4,
  0x00971FFC,
  0x00972004,
  0x0097200C,
  0x00972014,
  0x00972024,
  0x0097206C,
  0x00972094,
  0x0097209C,
  0x009720A4,
  0x009720B4,
  0x009720CC,
  0x009720FC,
  0x00972224,
  0x0097222C,
  0x00972234,
  0x0097223C,
  0x00972244,
  0x0097224C,
  0x00972254,
  0x0097225C,
  0x00972264,
  0x0097227C,
  0x00972284,
  0x009722DC,
  0x009722E4,
  0x009722EC,
  0x009722F4,
  0x009722FC,
  0x00972304,
  0x0097230C,
  0x00972314,
  0x0097231C,
  0x00972324,
  0x0097232C,
  0x00972334,
  0x0097233C,
  0x00972344,
  0x0097234C,
  0x00972354,
  0x0097235C,
  0x00972364,
  0x0097236C,
  0x00972374,
  0x0097237C,
  0x00972384,
  0x0097238C,
  0x00972394,
  0x0097239C,
  0x009723A4,
  0x009723AC,
  0x009723B4,
  0x009723BC,
  0x009723C4,
  0x009723CC,
  0x009723D4,
  0x009723DC,
  0x009723E4,
  0x009723EC,
  0x009723F4,
  0x009723FC,
  0x00972404,
  0x0097240C,
  0x00972414,
  0x0097241C,
  0x00972424,
  0x0097242C,
  0x00972434,
  0x0097243C,
  0x00972444,
  0x0097244C,
  0x00972454,
  0x0097245C,
  0x00972464,
  0x0097246C,
  0x00972474,
  0x0097247C,
  0x00972484,
  0x0097248C,
  0x00972494,
  0x0097249C,
  0x009724A4,
  0x009724AC,
  0x009724B4,
  0x009724BC,
  0x009724C4,
  0x009724CC,
  0x009724D4,
  0x009724DC,
  0x009724E4,
  0x009724EC,
  0x009724F4,
  0x009724FC,
  0x00972504,
  0x00972554,
  0x0097255C,
  0x00972564,
  0x0097256C,
  0x0097258C,
  0x00972594,
  0x0097259C,
  0x009725A4,
  0x009725AC,
  0x009725B4,
  0x009725BC,
  0x009725C4,
  0x009725CC,
  0x009725D4,
  0x009725E4,
  0x009725EC,
  0x00972614,
  0x0097261C,
  0x00972700,
  0x00974E40,
  0x00974E48,
  0x009D0044,
  0x009F0A80,
  0x009F0A88,
  0x009FF084,
  0x009FF09C,
  0x009FF0B4,
  0x009FF0CC,
  0x009FF0E4,
  0x009FF0FC,
  0x009FF114,
  0x009FF12C,
  0x009FF1AC,
  0x00A1133C,
  0x00A113A4,
  0x00A113AC,
  0x00A113F4
};
ULONG listVerticalBottomAlignItemsMovs[] = {
  0x006F4D9F,
  0x006FFCB8,
  0x006FFCE0,
  0x006FFCF4,
  0x006FFD12,
  0x006FFD1C,
  0x006FFD58,
  0x006FFD76,
  0x0070D2CE,
  0x0070D2EC,
  0x0070D30A,
  0x0070D328,
  0x0070D346,
  0x0070D364,
  0x0070D382,
  0x0070D3A0,
  0x0070D3B4,
  0x0070D3D2,
  0x0070D3F0,
  0x0070D404
};
ULONG listVerticalBottomAlignItemsDelay[] = {
  0x009F0A90,
  0x009F0A98
};
ULONG listVerticalCenterAlignItems[] = {
  0x0066DFFA,
  0x00972054,
  0x0097205C,
  0x009720E4,
  0x009720EC,
  0x00972104,
  0x009720F4
};

RECT rectMon;

HWND __stdcall myCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y,
                                 int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
  X = rectMon.left;
  Y = rectMon.top;
  nWidth = rectMon.right - rectMon.left;
  nHeight = rectMon.bottom - rectMon.top;

  dwStyle = WS_POPUP;
  dwExStyle = WS_EX_APPWINDOW;
  HWND hWnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
  //SetWindowPos(hWnd, HWND_NOTOPMOST, X, Y, nWidth, nHeight, SWP_SHOWWINDOW);
  return hWnd;
}
ULONG_PTR pmyCreateWindowExA = (ULONG_PTR)myCreateWindowExA;

static DWORD GetImageSize(HMODULE hModule) {
  PIMAGE_DOS_HEADER img_dos_headers;
  PIMAGE_NT_HEADERS img_nt_headers;

  if (!hModule)
    hModule = GetModuleHandleA(0);

  img_dos_headers = (PIMAGE_DOS_HEADER)hModule;
  if (img_dos_headers->e_magic != IMAGE_DOS_SIGNATURE)
    return 0;
  img_nt_headers = (PIMAGE_NT_HEADERS)((size_t)img_dos_headers + img_dos_headers->e_lfanew);
  if (img_nt_headers->Signature != IMAGE_NT_SIGNATURE)
    return 0;
  if (img_nt_headers->FileHeader.SizeOfOptionalHeader < 60) // OptionalHeader.SizeOfImage
    return 0;

  return img_nt_headers->OptionalHeader.SizeOfImage;
}

void patch_widescreen(void) {
  if (GetImageSize(0) < 0x00762000 || *(DWORD*)0x00B613FA != 0x4C4E3935) { // 59NL
    MessageBox(0, "Wrong client version, expected MTethVer12513 (1.25.13)", "Error", MB_ICONERROR);
    ExitProcess(-1);
  }

  if (*(ULONGLONG*)0x0082D1D0 != 0x15FF5568246C8B50 || *(ULONGLONG*)0x00482E20 != 0x000000A46C7205C7) // already patched
    return;

  HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
  MONITORINFO info;
  memset(&info, 0, sizeof(MONITORINFO));
  info.cbSize = sizeof(MONITORINFO);
  GetMonitorInfoA(monitor, &info);
  memcpy(&rectMon, &info.rcMonitor, sizeof(RECT));

  memset((void*)0x00482E20, 0x90, 52); // WINDOW_MODE = 1
  *(ULONG_PTR*)0x0082D1D8 = (ULONG_PTR)&pmyCreateWindowExA;

  DWORD width = rectMon.right - rectMon.left;
  DWORD height = rectMon.bottom - rectMon.top;
  float AR = (float)width / (float)height;
  float A = (AR / (4.0f / 3.0f)) * 640.0f;
  float B = (AR / (4.0f / 3.0f)) * 128.0f;
  float C = 480.0f;
  DWORD D = 128;

#if HUD_SCALE
  A *= 1.5f;
  B *= 1.5f;
  C *= 1.5f;
  D = D * 3 / 2;
#endif

  for (int i = 0; i < 6; i++) {
    *(DWORD*)(0x009006F4 + i*8) = width;
    *(DWORD*)(0x009006F8 + i*8) = height;
  }

  *(DWORD*)(0x009A3840) = (DWORD)(B * 4.0f);
  *(DWORD*)(0x009A3844) = D * 3;
  *(DWORD*)(0x009A3848) = (DWORD)B;
  *(DWORD*)(0x009A384C) = D;
  *(DWORD*)(0x009A3854) = (DWORD)(B * 4.0f);
  *(DWORD*)(0x009A3858) = D * 2;
  *(DWORD*)(0x009A385C) = (DWORD)B;
  *(DWORD*)(0x009A3860) = D;
  *(DWORD*)(0x009A3868) = (DWORD)(B * 4.0f);
  *(DWORD*)(0x009A386C) = D;
  *(DWORD*)(0x009A3870) = (DWORD)B;
  *(DWORD*)(0x009A3874) = D;
  *(DWORD*)(0x009A387C) = (DWORD)(B * 4.0f);
  *(DWORD*)(0x009A3884) = (DWORD)B;
  *(DWORD*)(0x009A3888) = D;
  *(DWORD*)(0x009A3894) = D * 2;
  *(DWORD*)(0x009A3898) = (DWORD)(B * 2.0f);
  *(DWORD*)(0x009A389C) = D * 2;
  *(DWORD*)(0x009A38AC) = (DWORD)(B * 2.0f);
  *(DWORD*)(0x009A38B0) = D * 2;
  *(DWORD*)(0x009A38B8) = (DWORD)(B * 2.0f);
  *(DWORD*)(0x009A38BC) = D * 2;
  *(DWORD*)(0x009A38C0) = (DWORD)(B * 2.0f);
  *(DWORD*)(0x009A38C4) = D * 2;
  *(DWORD*)(0x009A38CC) = (DWORD)(B * 2.0f);
  *(DWORD*)(0x009A38D4) = (DWORD)(B * 2.0f);
  *(DWORD*)(0x009A38D8) = D * 2;

  *(float*)(0x009D0040) = (A / 2) - 128.0f;
  *(float*)(0x00761CCC) = (A / 2) - 110.0f;
  *(float*)(0x00761CDE) = (A / 2) - 110.0f;
  *(float*)(0x0076236C) = (A / 2) - 180.0f;
  *(float*)(0x007625F8) = (A / 2) - 4.0f;
  *(float*)(0x007627CF) = (A / 2) - 4.0f;
  *(float*)(0x008F87D4) = A - 63.0f;
  *(float*)(0x008F87E4) = A - 63.0f;
  *(float*)(0x009F0ADC) = A - 157.0f;
  *(float*)(0x009F0AF4) = A - 288.0f;
  *(float*)(0x009F0B0C) = A - 157.0f;
  *(float*)(0x009F0B24) = A - 288.0f;
  *(float*)(0x009F0B3C) = A - 128.0f;
  *(float*)(0x0070D350) = A - 384.0f;
  *(float*)(0x009F0B5C) = A - 128.0f;
  *(float*)(0x0070D2F6) = A - 384.0f;
  *(float*)(0x009FF080) = A - 144.0f;
  *(float*)(0x009FF098) = A - 144.0f;
  *(float*)(0x009FF0B0) = A - 16.0f;
  *(float*)(0x009FF0C8) = A - 16.0f;
  *(float*)(0x009FF0E0) = A - 272.0f;
  *(float*)(0x009FF0F8) = A - 272.0f;
  *(float*)(0x009FF110) = A - 143.0f;
  *(float*)(0x009FF128) = A - 143.0f;
  *(float*)(0x009FF1A8) = A - 144.0f;

  *(float*)(0x00489E53) = AR;
  *(float*)(0x0082EC74) = AR;
  *(float*)(0x0082ED43) = AR;
  *(float*)(0x0082EF4C) = AR;
  *(float*)(0x0082F018) = AR;
  *(float*)(0x0082F700) = AR;
  *(float*)(0x0082F7DB) = AR;
  *(float*)(0x00901288) = AR;
  *(float*)(0x0098A494) = AR;
  *(float*)(0x0098A4DC) = AR;
  *(float*)(0x0098A500) = AR;
  *(float*)(0x00A33CD0) = AR;

  for (int i = 0; i < _countof(listHUDWidth); i++) {
    *(float*)listHUDWidth[i] = A;
  }

  for (int i = 0; i < _countof(listHUDHeight); i++) {
    *(float*)listHUDHeight[i] = C;
  }

  for (int i = 0; i < _countof(listCenterAlignItems); i++) {
    *(float*)listCenterAlignItems[i] =*(float*)listCenterAlignItems[i] + ((A - 640.0f) / 2.0f);
  }

  for (int i = 0; i < _countof(listRightAlignItems); i++) {
    *(float*)listRightAlignItems[i] = *(float*)listRightAlignItems[i] + (A - 640.0f);
  }

  for (int i = 0; i < _countof(listVerticalBottomAlignItems); i++) {
    *(float*)listVerticalBottomAlignItems[i] = *(float*)listVerticalBottomAlignItems[i] + (C - 480.0f);
  }

  for (int i = 0; i < _countof(listVerticalBottomAlignItemsMovs); i++) {
    *(float*)listVerticalBottomAlignItemsMovs[i] = *(float*)listVerticalBottomAlignItemsMovs[i] + (C - 480.0f);
  }

  for (int i = 0; i < _countof(listVerticalBottomAlignItemsDelay); i++) {
    *(float*)listVerticalBottomAlignItemsDelay[i] = *(float*)listVerticalBottomAlignItemsDelay[i] + (C - 480.0f);
  }

  for (int i = 0; i < _countof(listVerticalCenterAlignItems); i++) {
    *(float*)listVerticalCenterAlignItems[i] = *(float*)listVerticalCenterAlignItems[i] + ((C / 2.0f) - 240.0f);
  }

  *(float*)(0x007584EE) = A + 10.0f;
  *(float*)(0x0075851F) = A + 10.0f;
  *(float*)(0x00978454) = A - 640.0f;
  *(float*)(0x00978458) = A - 640.0f - 220.0f;
  *(float*)(0x006F4922) = A / 2.0f;
  *(float*)(0x006F4936) = C / 2.0f;
  *(float*)(0x009712DC) = 240.0f - (C - 480.0f);
  *(float*)(0x007165B8) = A - 16.0f;
  *(float*)(0x006F4CF2) = 64.0f * (((B - 128.0f) / 128.0f) + 1.0f);
  *(float*)(0x006F4CFA) = 61.0f * ((((float)D - 128.0f) / 128.0f) + 1.0f);
  *(float*)(0x006F4D02) = 320.0f * (((B - 128.0f) / 128.0f) + 1.0f);
  *(float*)(0x006F4D0A) = 317.0f * ((((float)D - 128.0f) / 128.0f) + 1.0f);
  *(float*)(0x006F4D4E) = 320.0f * (((B - 128.0f) / 128.0f) + 1.0f);
  *(float*)(0x006F4D56) = 61.0f * ((((float)D - 128.0f) / 128.0f) + 1.0f);
  *(float*)(0x006F4D5E) = 576.0f * (((B - 128.0f) / 128.0f) + 1.0f);
  *(float*)(0x006F4D66) = 317.0f * ((((float)D - 128.0f) / 128.0f) + 1.0f);
}
