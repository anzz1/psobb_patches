// util.h

#pragma once

#pragma function(memset)
void* __cdecl memset(void* dst, int val, size_t count) {
  void* start = dst;
  while (count--) {
    *(char*)dst = (char)val;
    dst = (char*)dst + 1;
  }
  return start;
}

__forceinline static DWORD calc_disp32(ULONG_PTR src, ULONG_PTR dst)
{
  // disp32 = target - disp32_ptr - 4
  return (DWORD)(dst-src-4);
}

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
