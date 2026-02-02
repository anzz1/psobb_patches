#define WIN32_LEAN_AND_MEAN
#include "d3d8.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
// #include <d3d9.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Globals
static BOOL g_bFrameGenEnabled = FALSE;
static DWORD g_dwTargetHz = 60;
static BOOL g_bHooked = FALSE;
static FILE *g_pLogFile = nullptr;

static void Log(const char *format, ...) {
  if (!g_pLogFile) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char *p = strrchr(path, '\\');
    if (p)
      strcpy(p + 1, "framegen.log");
    else
      return;
    g_pLogFile = fopen(path, "a");
  }
  if (g_pLogFile) {
    va_list args;
    va_start(args, format);
    vfprintf(g_pLogFile, format, args);
    va_end(args);
    fprintf(g_pLogFile, "\n");
    fflush(g_pLogFile);
  }
}

// Config Loading
static void LoadConfig() {
  char path[MAX_PATH];
  GetModuleFileNameA(NULL, path, MAX_PATH);
  char *p = strrchr(path, '\\');
  if (p) {
    strcpy(p + 1, "framegen.cfg");
  } else {
    return; // Failed path
  }

  Log("Loading config from %s", path);
  g_bFrameGenEnabled = GetPrivateProfileIntA("General", "FrameGen", 0, path);
  g_dwTargetHz = GetPrivateProfileIntA("General", "TargetHz", 60, path);
  Log("Config: FrameGen=%d, TargetHz=%d", g_bFrameGenEnabled, g_dwTargetHz);

  if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
    WritePrivateProfileStringA("General", "FrameGen", "0", path);
    WritePrivateProfileStringA("General", "TargetHz", "60", path);
  }
}

// Function Pointers
typedef IDirect3D8 *(WINAPI *Direct3DCreate8_t)(UINT SDKVersion);
static Direct3DCreate8_t g_pOriginalDirect3DCreate8 = nullptr;

typedef HRESULT(__stdcall *CreateDevice_t)(
    IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
    DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters,
    IDirect3DDevice8 **ppReturnedDeviceInterface);
static CreateDevice_t g_pOriginalCreateDevice = nullptr;

typedef HRESULT(__stdcall *Present_t)(IDirect3DDevice8 *This,
                                      CONST RECT *pSourceRect,
                                      CONST RECT *pDestRect,
                                      HWND hDestWindowOverride,
                                      CONST RGNDATA *pDirtyRegion);
static Present_t g_pOriginalPresent = nullptr;

// Helpers
static int GetRepeatCount() {
  if (!g_bFrameGenEnabled || g_dwTargetHz <= 30)
    return 1;
  if (g_dwTargetHz >= 240)
    return 8;
  if (g_dwTargetHz >= 165)
    return 6;
  if (g_dwTargetHz >= 144)
    return 5;
  if (g_dwTargetHz >= 120)
    return 4;
  if (g_dwTargetHz >= 90)
    return 3;
  if (g_dwTargetHz >= 60)
    return 2;
  return 1;
}

// Hooked Present
static HRESULT __stdcall Hooked_Present(IDirect3DDevice8 *This,
                                        CONST RECT *pSourceRect,
                                        CONST RECT *pDestRect,
                                        HWND hDestWindowOverride,
                                        CONST RGNDATA *pDirtyRegion) {
  if (!g_bFrameGenEnabled)
    return g_pOriginalPresent(This, pSourceRect, pDestRect, hDestWindowOverride,
                              pDirtyRegion);

  int repeats = GetRepeatCount();
  HRESULT hr = S_OK;
  for (int i = 0; i < repeats; i++) {
    hr = g_pOriginalPresent(This, pSourceRect, pDestRect, hDestWindowOverride,
                            pDirtyRegion);
    if (FAILED(hr))
      break;
  }
  return hr;
}

// Hook Helper
static void HookVTable(void **vtable, int index, void *hook, void **original) {
  if (!vtable || !hook)
    return;
  DWORD oldProtect;
  if (VirtualProtect(&vtable[index], sizeof(void *), PAGE_EXECUTE_READWRITE,
                     &oldProtect)) {
    if (original)
      *original = vtable[index];
    vtable[index] = hook;
    VirtualProtect(&vtable[index], sizeof(void *), oldProtect, &oldProtect);
  }
}

static HRESULT __stdcall
Hooked_CreateDevice(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType,
                    HWND hFocusWindow, DWORD BehaviorFlags,
                    D3DPRESENT_PARAMETERS *pPresentationParameters,
                    IDirect3DDevice8 **ppReturnedDeviceInterface) {

  HRESULT hr = g_pOriginalCreateDevice(This, Adapter, DeviceType, hFocusWindow,
                                       BehaviorFlags, pPresentationParameters,
                                       ppReturnedDeviceInterface);

  if (SUCCEEDED(hr) && ppReturnedDeviceInterface &&
      *ppReturnedDeviceInterface) {
    Log("Hooking IDirect3DDevice8::Present (15)");
    void **vtable = *(void ***)(*ppReturnedDeviceInterface);
    HookVTable(vtable, 15, (void *)Hooked_Present,
               (void **)&g_pOriginalPresent);
  }
  return hr;
}

// Wrapper & IAT Hooking
static IDirect3D8 *WINAPI MyDirect3DCreate8(UINT SDKVersion) {
  if (!g_pOriginalDirect3DCreate8) {
    HMODULE hD3D8 = GetModuleHandleA("d3d8.dll");
    if (hD3D8)
      g_pOriginalDirect3DCreate8 =
          (Direct3DCreate8_t)GetProcAddress(hD3D8, "Direct3DCreate8");
  }
  if (!g_pOriginalDirect3DCreate8)
    return nullptr;
  IDirect3D8 *pD3D8 = g_pOriginalDirect3DCreate8(SDKVersion);
  if (pD3D8) {
    void **vtable = *(void ***)pD3D8;
    HookVTable(vtable, 15, (void *)Hooked_CreateDevice,
               (void **)&g_pOriginalCreateDevice);
  }
  return pD3D8;
}

// IAT Installation
static void *InstallIATHook(const char *dllName, const char *funcName,
                            void *hookFunc) {
  HMODULE hModule = GetModuleHandleA(NULL);
  if (!hModule)
    return nullptr;
  PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
  if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    return nullptr;
  PIMAGE_NT_HEADERS pNtHeaders =
      (PIMAGE_NT_HEADERS)((BYTE *)hModule + pDosHeader->e_lfanew);

  PIMAGE_IMPORT_DESCRIPTOR pImportDesc =
      (PIMAGE_IMPORT_DESCRIPTOR)((BYTE *)hModule +
                                 pNtHeaders->OptionalHeader
                                     .DataDirectory
                                         [IMAGE_DIRECTORY_ENTRY_IMPORT]
                                     .VirtualAddress);

  if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
          .Size == 0)
    return nullptr;

  while (pImportDesc->Name) {
    const char *pszModuleName =
        (const char *)((BYTE *)hModule + pImportDesc->Name);
    if (_stricmp(pszModuleName, dllName) == 0) {
      PIMAGE_THUNK_DATA pThunk =
          (PIMAGE_THUNK_DATA)((BYTE *)hModule + pImportDesc->FirstThunk);
      PIMAGE_THUNK_DATA pOrgThunk =
          (PIMAGE_THUNK_DATA)((BYTE *)hModule +
                              pImportDesc->OriginalFirstThunk);
      if (!pOrgThunk)
        pOrgThunk = pThunk;

      while (pThunk->u1.Function) {
        if (pOrgThunk->u1.AddressOfData &&
            !(pOrgThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
          PIMAGE_IMPORT_BY_NAME pImport =
              (PIMAGE_IMPORT_BY_NAME)((BYTE *)hModule +
                                      pOrgThunk->u1.AddressOfData);
          if (strcmp(pImport->Name, funcName) == 0) {
            void *original = (void *)pThunk->u1.Function;
            DWORD oldProtect;
            if (VirtualProtect(&pThunk->u1.Function, sizeof(ULONG_PTR),
                               PAGE_READWRITE, &oldProtect)) {
              pThunk->u1.Function = (ULONG_PTR)hookFunc;
              VirtualProtect(&pThunk->u1.Function, sizeof(ULONG_PTR),
                             oldProtect, &oldProtect);
              return original;
            }
          }
        }
        pThunk++;
        pOrgThunk++;
      }
    }
    pImportDesc++;
  }
  return nullptr;
}

static void InstallHook() {
  Log("InstallHook: Installing IAT Hook on Direct3DCreate8 (Standard Mode)...");
  g_pOriginalDirect3DCreate8 = (Direct3DCreate8_t)InstallIATHook(
      "d3d8.dll", "Direct3DCreate8", (void *)MyDirect3DCreate8);
  if (g_pOriginalDirect3DCreate8)
    g_bHooked = TRUE;
}

static DWORD WINAPI HookThread(LPVOID lpParam) {
  InstallHook();
  return 0;
}

extern "C" __declspec(dllexport) void __stdcall load(void) {
  LoadConfig();
  if (g_bFrameGenEnabled)
    CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH)
    DisableThreadLibraryCalls(hinstDLL);
  return TRUE;
}
