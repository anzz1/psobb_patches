/**
 * PSOBB Improve Framerate Patch
 *
 * PURPOSE:
 * This DLL hooks the Direct3D8 Present function to repeat frames.
 * PSOBB runs its internal logic and rendering at a hardcoded 30 FPS.
 *
 * On high-refresh rate monitors (60Hz, 120Hz, 144Hz+), sending only 30 frames
 * causes VSync stutter judgment or uneven frame pacing.
 *
 * By intercepting the Present call, we can "multiply" the 30 source frames
 * into a stream of 60, 90, 120, etc. identical frames.
 * This provides a compliant high-refresh signal, which is critical for:
 * 1. Smooth VSync operation (no tearing, no stutter).
 * 2. Enabling external AI Frame Generation tools (like Lossless Scaling)
 *    which require a stable high-framerate input to function correctly.
 */

#define WIN32_LEAN_AND_MEAN
#include "d3d8.hpp"
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================
static BOOL g_bFrameGenEnabled = FALSE; // Master switch
static DWORD g_dwTargetHz = 60;         // Target Refresh Rate
static BOOL g_bHooked = FALSE;          // Prevent double hooking
static FILE *g_pLogFile = nullptr;      // Log handle

// ============================================================================
// LOGGING
// ============================================================================
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

// ============================================================================
// CONFIGURATION
// ============================================================================
// Reads 'framegen.cfg' from the local directory.
// Expected Format:
// [General]
// FrameGen=1  (0 = Disable, 1 = Enable)
// TargetHz=60 (Target Monitor Refresh Rate)
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

  // Auto-generate default config if missing
  if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
    WritePrivateProfileStringA("General", "FrameGen", "0", path);
    WritePrivateProfileStringA("General", "TargetHz", "30", path);
  }
}

// ============================================================================
// ORIGINAL FUNCTION POINTERS
// ============================================================================
// We need to store the original addresses to call them after our hooks.
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

// ============================================================================
// FRAME REPETITION LOGIC
// ============================================================================
// Calculates how many times we present the SAME frame to match the target Hz.
// Base Game FPS = 30.
// Formula: Repeats = TargetHz / 30.
//
// Examples:
// 60Hz  -> 60/30 = 2 Repeats
// 144Hz -> 144/30 = 4.8 -> round to 5 Repeats (150 FPS effective)
// 165Hz -> 165/30 = 5.5 -> round to 6 Repeats (180 FPS effective)
// etc...
static int GetRepeatCount() {
  if (!g_bFrameGenEnabled || g_dwTargetHz <= 30)
    return 1;
  if (g_dwTargetHz >= 400)
    return 14; // 420 FPS (god, I hope this never happens)
  if (g_dwTargetHz >= 360)
    return 12; // 360 FPS
  if (g_dwTargetHz >= 240)
    return 8; // 240 FPS
  if (g_dwTargetHz >= 165)
    return 6; // 180 FPS
  if (g_dwTargetHz >= 144)
    return 5; // 150 FPS
  if (g_dwTargetHz >= 120)
    return 4; // 120 FPS
  if (g_dwTargetHz >= 90)
    return 3; // 90 FPS
  if (g_dwTargetHz >= 60)
    return 2; // 60 FPS
  return 1;
}

// ============================================================================
// HOOKS
// ============================================================================

// Hooked Present: This is where the magic happens.
// Instead of presenting once, we loop 'GetRepeatCount()' times.
// This floods the driver with identical frames, tricking the monitor/VSync.
static HRESULT __stdcall Hooked_Present(IDirect3DDevice8 *This,
                                        CONST RECT *pSourceRect,
                                        CONST RECT *pDestRect,
                                        HWND hDestWindowOverride,
                                        CONST RGNDATA *pDirtyRegion) {
  // If disabled, pass through immediately
  if (!g_bFrameGenEnabled)
    return g_pOriginalPresent(This, pSourceRect, pDestRect, hDestWindowOverride,
                              pDirtyRegion);

  int repeats = GetRepeatCount();
  HRESULT hr = S_OK;

  // Frame Repetition Loop
  for (int i = 0; i < repeats; i++) {
    // Call the REAL driver Present
    hr = g_pOriginalPresent(This, pSourceRect, pDestRect, hDestWindowOverride,
                            pDirtyRegion);

    // Stop if device is lost or error occurs
    if (FAILED(hr))
      break;
  }

  return hr;
}

// VTable Hook Helper: Replaces function pointer in the Interface's Virtual
// Function Table
static void HookVTable(void **vtable, int index, void *hook, void **original) {
  if (!vtable || !hook)
    return;
  DWORD oldProtect;
  // Unprotect memory to allow writing
  if (VirtualProtect(&vtable[index], sizeof(void *), PAGE_EXECUTE_READWRITE,
                     &oldProtect)) {
    if (original)
      *original = vtable[index]; // Backup original
    vtable[index] = hook;        // Overwrite with our hook
    VirtualProtect(&vtable[index], sizeof(void *), oldProtect,
                   &oldProtect); // Restore protection
  }
}

// Hooked CreateDevice: We need to catch the Device creation to find the Present
// function.
static HRESULT __stdcall
Hooked_CreateDevice(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType,
                    HWND hFocusWindow, DWORD BehaviorFlags,
                    D3DPRESENT_PARAMETERS *pPresentationParameters,
                    IDirect3DDevice8 **ppReturnedDeviceInterface) {

  // Call original to create the real device
  HRESULT hr = g_pOriginalCreateDevice(This, Adapter, DeviceType, hFocusWindow,
                                       BehaviorFlags, pPresentationParameters,
                                       ppReturnedDeviceInterface);

  // If successful, hook the returned Device's VTable
  if (SUCCEEDED(hr) && ppReturnedDeviceInterface &&
      *ppReturnedDeviceInterface) {
    Log("Hooking IDirect3DDevice8::Present (Index 15)");

    // IDirect3DDevice8 VTable layout: Present is usually at index 15
    void **vtable = *(void ***)(*ppReturnedDeviceInterface);
    HookVTable(vtable, 15, (void *)Hooked_Present,
               (void **)&g_pOriginalPresent);
  }
  return hr;
}

// Wrapper for Direct3DCreate8: Entry point hook.
// This is returned to the game when it asks for "Direct3DCreate8".
static IDirect3D8 *WINAPI MyDirect3DCreate8(UINT SDKVersion) {
  // Lazy resolution of original function
  if (!g_pOriginalDirect3DCreate8) {
    HMODULE hD3D8 = GetModuleHandleA("d3d8.dll");
    if (hD3D8)
      g_pOriginalDirect3DCreate8 =
          (Direct3DCreate8_t)GetProcAddress(hD3D8, "Direct3DCreate8");
  }

  if (!g_pOriginalDirect3DCreate8)
    return nullptr;

  // Create the main D3D8 object
  IDirect3D8 *pD3D8 = g_pOriginalDirect3DCreate8(SDKVersion);

  if (pD3D8) {
    // Hook IDirect3D8::CreateDevice (Index 15 in D3D8 VTable)
    void **vtable = *(void ***)pD3D8;
    HookVTable(vtable, 15, (void *)Hooked_CreateDevice,
               (void **)&g_pOriginalCreateDevice);
  }
  return pD3D8;
}

// ============================================================================
// IAT HOOKING SYSTEM
// ============================================================================
// We hook the Import Address Table of the GAME EXECUTABLE (loaded in memory).
// We look for imports of "d3d8.dll" and redirect "Direct3DCreate8" to our
// wrapper.
static void *InstallIATHook(const char *dllName, const char *funcName,
                            void *hookFunc) {
  HMODULE hModule = GetModuleHandleA(NULL); // Get Game Exe Module
  if (!hModule)
    return nullptr;

  PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
  if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    return nullptr;

  PIMAGE_NT_HEADERS pNtHeaders =
      (PIMAGE_NT_HEADERS)((BYTE *)hModule + pDosHeader->e_lfanew);

  // Find Import Directory
  PIMAGE_IMPORT_DESCRIPTOR pImportDesc =
      (PIMAGE_IMPORT_DESCRIPTOR)((BYTE *)hModule +
                                 pNtHeaders->OptionalHeader
                                     .DataDirectory
                                         [IMAGE_DIRECTORY_ENTRY_IMPORT]
                                     .VirtualAddress);

  if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
          .Size == 0)
    return nullptr;

  // Iterate all imported DLLs
  while (pImportDesc->Name) {
    const char *pszModuleName =
        (const char *)((BYTE *)hModule + pImportDesc->Name);

    // Check if this is d3d8.dll
    if (_stricmp(pszModuleName, dllName) == 0) {
      PIMAGE_THUNK_DATA pThunk =
          (PIMAGE_THUNK_DATA)((BYTE *)hModule + pImportDesc->FirstThunk);
      PIMAGE_THUNK_DATA pOrgThunk =
          (PIMAGE_THUNK_DATA)((BYTE *)hModule +
                              pImportDesc->OriginalFirstThunk);
      if (!pOrgThunk)
        pOrgThunk = pThunk;

      // Iterate functions imported from this DLL
      while (pThunk->u1.Function) {
        if (pOrgThunk->u1.AddressOfData &&
            !(pOrgThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
          PIMAGE_IMPORT_BY_NAME pImport =
              (PIMAGE_IMPORT_BY_NAME)((BYTE *)hModule +
                                      pOrgThunk->u1.AddressOfData);

          // Check if function name matches Direct3DCreate8
          if (strcmp(pImport->Name, funcName) == 0) {
            void *original = (void *)pThunk->u1.Function;
            DWORD oldProtect;
            // Unprotect IAT entry and overwrite it
            if (VirtualProtect(&pThunk->u1.Function, sizeof(ULONG_PTR),
                               PAGE_READWRITE, &oldProtect)) {
              pThunk->u1.Function = (ULONG_PTR)hookFunc;
              VirtualProtect(&pThunk->u1.Function, sizeof(ULONG_PTR),
                             oldProtect, &oldProtect);
              return original; // Return original address
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

// Installs the specific hook for PSOBB
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
