// MoreSaveSlots 1.25.13 (59NL)
// Credits to fuzziqersoftware for the patch

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _NO_CRT_STDIO_INLINE
#include <windows.h>
#include "util.h"

#define SLOT_COUNT 12                                      // 4 - 127

#if SLOT_COUNT < 4 || SLOT_COUNT > 127
#error SLOT_COUNT must be between 4 and 127
#endif

#define TOTAL_FILE_SIZE                 ((SLOT_COUNT * 0x2EA4) + 0x14)
#define ROUND2_SEED_OFFSET              (TOTAL_FILE_SIZE - 0x04)
#define SAVE_COUNT_OFFSET               (TOTAL_FILE_SIZE - 0x08)
#define BGM_TEST_SONGS_UNLOCKED_OFFSET  (TOTAL_FILE_SIZE - 0x10)

// This patch enables scrolling behavior within the character list
void __declspec(naked) enableScroll()
{
  __asm {
    mov       eax, dword ptr [edi + 0x28];                 // cursor = char_select_menu->cursor_obj (TAdSelectCurGC*)
    or        dword ptr [eax + 0x01F8], 3;                 // cursor->flags |= 3; Enable scrolling
    mov       eax, dword ptr ds:[0x00A3B050];              // scroll_bar = TAdScrollBarXb_objs[0]
    mov       ecx, [eax + 0xEC];                           // ecx = scroll_bar->client_id
    imul      ecx, ecx, 0x24;
    // Set up scroll bar graphics (in struct at scroll_bar + 0x1C)
    // TODO: Even though we set this up the same way PSO Xbox does, it still
    // doesn't render. Figure this out and fix it.
    mov       dword ptr [eax + ecx + 0x1C], 0x439D0000;
    mov       dword ptr [eax + ecx + 0x20], 0x43360000;
    mov       dword ptr [eax + ecx + 0x24], 0x439D0000;
    mov       dword ptr [eax + ecx + 0x28], 0x4392AB85;
    mov       dword ptr [eax + ecx + 0x2C], 0x40400000;
    mov       dword ptr [eax + ecx + 0x30], 0x425EA3D7;
    mov       dword ptr [eax + ecx + 0x34], 0x00000008;
    mov       dword ptr [eax + ecx + 0x38], 0x00000000;
    mov       dword ptr [eax + ecx + 0x2C], 0x00000000;
    or        dword ptr [eax + 0xF0], 1;                   // scroll_bar->flags |= 1
    mov       ecx, [eax + 0xEC];
    shl       ecx, 4;
    mov       dword ptr [eax + ecx + 0xAC], 0;             // scroll_bar->selection_state[client_id].scroll_offset = 0
    mov       dword ptr [eax + ecx + 0xB0], 0;             // scroll_bar->selection_state[client_id].selected_index = 0
    mov       dword ptr [eax + ecx + 0xB4], 4;             // scroll_bar->selection_state[client_id].num_items_in_view = 4
    mov       dword ptr [eax + ecx + 0xB8], (SLOT_COUNT-1);        // scroll_bar->selection_state[client_id].last_item_index = (slot count - 1)
    pop       edi
    ret
  }
}

// This patch fixes character selection cursor object so it will take the
// scroll offset into account
void __declspec(naked) fixScrollSelection()
{
  __asm {
    mov       edx, [edi + 0x28]                            // cursor = this->ad_select_cur_obj (TAdSelectCurGC*)
    mov       ebp, [edx + 0x44]                            // ebp = cursor->selected_index_within_view
    mov       eax, dword ptr ds:[0x00A3B050]               // scroll_bar = TAdScrollBarXb_objs[0]
    add       ebp, [eax + 0xAC]                            // ebp += scroll_bar->selection_state[0].scroll_offset
    ret
  }
}

// This patch changes the TAdSinglePlyChrSelectGC::selected_index_within_view
// to be the selected character's absolute index (including scroll_offset),
// not the index only within to the displayed four characters
void __declspec(naked) fixScrollIndex()
{
  __asm {
    mov       eax, dword ptr ds:[0x00A3B050]               // scroll_bar = TAdScrollBarXb_objs[0]
    mov       eax, [eax + 0xAC]                            // eax = scroll_bar->selection_state[0].scroll_offset
    mov       edx, [edi + 0x28]                            // cursor = this->ad_select_cur_obj (TAdSelectCurGC*)
    add       eax, [edx + 0x44]                            // eax += cursor->selected_index_within_view
    ret
  }
}

// This patch fixes the character file indexing so it will account for the
// scroll position
void __declspec(naked) fixCharacterFileIndex()
{
  __asm {
    mov       eax, dword ptr ds:[0x00A3B050]               // scroll_bar = TAdScrollBarXb_objs[0]
    mov       eax, [eax + 0xAC]                            // eax = TAdScrollBarXb_objs[0]->selection_state[0].scroll_offset
    add       ebp, eax                                     // arg0 += eax
    mov       [esp + 4], ebp
    mov       eax, 0x006C1A80
    jmp       eax                                          // set_current_char_slot
  }
}

// This patch fixes the preview display so it will show the correct section
// ID, etc.
void __declspec(naked) fixPreviewWindow()
{
  __asm {
    mov       eax, dword ptr ds:[0x00A3B050]               // scroll_bar = TAdScrollBarXb_objs[0]
    mov       eax, [eax + 0xAC]                            // eax = scroll_bar->selection_state[0].scroll_offset
    add       [esp + 4], eax
    mov       eax, 0x006C44D0                              // get_player_preview_info
    jmp       eax
  }
}


// Fix Signature check on all save files (rewritten as loop)
static BYTE fixSaveFileSignatureCheck[] = {
  0xBA, 0xB1, 0xD5, 0x7E, 0xC8, 0x05, 0xE8, 0x04, 0x00, 0x00, 0xB9, SLOT_COUNT, 0x00, 0x00, 0x00, 0x83,
  0x38, 0x00, 0x74, 0x04, 0x39, 0x10, 0x75, 0x0C, 0x05, 0xA4, 0x2E, 0x00, 0x00, 0x49, 0x75, 0xEF,
  0x31, 0xC0, 0xEB, 0x25, 0x31, 0xC0, 0x40, 0xEB, 0x20, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC
};
#if 0
void __declspec(naked) fixSaveFileSignatureCheck()
{
  __asm {
    mov      edx, 0xC87ED5B1                          // Expected signature value
    add      eax, 0x04E8                              // &char_file_list->chars[0].part2.signature
    mov      ecx, 0x0C                                // slot count
    again:
    cmp      dword ptr [eax], 0                       // signature == 0 (no char in slot)
    je       sig_ok
    cmp      dword ptr [eax], edx                     // signature == expected value
    jne      sig_bad
    sig_ok:
    add      eax, 0x2EA4                              // Advance to next slot
    dec      ecx
    jnz      again
    xor      eax, eax                                 // All signatures OK (eax = 0)
    jmp      sig_check_end
    sig_bad:
    xor      eax, eax                                 // Bad signature (eax = 1)
    inc      eax
    jmp      sig_check_end
    //data   CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    int 3
    sig_check_end:                                    // 006C1C76
  }
}
#endif


// Show slot number in each menu item
static BYTE addMenuSlotNumber[] = {
  0x8D, 0x94, 0x24, 0xC4, 0x02, 0x00, 0x00, 0x8B, 0x5B, 0x08, 0x43, 0x53, 0x89, 0xF1, 0x52, 0xB8,
  0x04, 0x26, 0x40, 0x00, 0xFF, 0xD0, 0x8D, 0x84, 0x24, 0xC4, 0x02, 0x00, 0x00, 0x66, 0x83, 0x38,
  0x00, 0x74, 0x05, 0x83, 0xC0, 0x02, 0xEB, 0xF5, 0x8B, 0x0D, 0x50, 0xB0, 0xA3, 0x00, 0x8B, 0x89,
  0xAC, 0x00, 0x00, 0x00, 0x8D, 0x4C, 0x29, 0x01, 0x51, 0xE8, 0x0E, 0x00, 0x00, 0x00, 0x20, 0x00,
  0x28, 0x00, 0x23, 0x00, 0x25, 0x00, 0x64, 0x00, 0x29, 0x00, 0x00, 0x00, 0x50, 0xB8, 0x29, 0x7E,
  0x85, 0x00, 0xFF, 0xD0, 0x83, 0xC4, 0x0C, 0xE9, 0x9A };
#if 0
void __declspec(naked) addMenuSlotNumber()
{
  __asm {
    // Original call (sprintf(line_buf, "LV%d", preview_info->visual.disp.level + 1))
    lea      edx, [esp + 0x02C4]
    mov      ebx, [ebx + 8]
    inc      ebx
    push     ebx
    mov      ecx, esi
    push     edx
    mov      eax, 0x00402604
    call     eax
    // Find the end of the string
    lea      eax, [esp + 0x02C4]
    show_slot_number_strend_again:
    cmp      word ptr [eax], 0
    je       show_slot_number_strend_done
    add      eax, 2
    jmp      show_slot_number_strend_again
    show_slot_number_strend_done:
    // Format the slot number and append it to the string
    mov      ecx, dword ptr ds:[0x00A3B050]           // scroll_bar = TAdScrollBarXb_objs[0]
    mov      ecx, [ecx + 0xAC]                        // ecx = scroll_bar->selection_state[0].scroll_offset
    lea      ecx, [ecx + ebp + 1]
    push     ecx                                      // Slot number (scroll_offset + z)
    call     get_show_slot_number_suffix_fmt
    //.binary  2000280023002500640029000000           // L" (#%d)"
    _emit    20
    _emit    00
    _emit    28
    _emit    00
    _emit    23
    _emit    00
    _emit    25
    _emit    00
    _emit    64
    _emit    00
    _emit    29
    _emit    00
    _emit    00
    _emit    00
    get_show_slot_number_suffix_fmt:
    push     eax                                      // Destination buffer
    mov      eax, 0x00857E29                          // _swprintf
    call     eax
    add      esp, 0x0C
    jmp      show_slot_number_end
    //.zero  0x98
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    _emit    0
    show_slot_number_end:                             // 00401E4D
  }
}
#endif

// These patches change various places where the character data size and slot
// count are referenced
void fix_references(void)
{
  *(BYTE*)(0x004751A4) = SLOT_COUNT;                       // slot count; TDataProtocol::handle_E5
  *(BYTE*)(0x0047525B) = SLOT_COUNT;                       // slot count; import_player_preview
  *(BYTE*)(0x004785E1) = SLOT_COUNT;                       // slot count; TDataProtocol::handle_E4
  *(DWORD*)(0x0048242D) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C17BF) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C1CCB) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C1CFE) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C1D1C) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C1DD7) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C222E) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C226D) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C228E) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C229E) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C24DB) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C2643) = SAVE_COUNT_OFFSET;               // save_count offset
  *(DWORD*)(0x006C264D) = SAVE_COUNT_OFFSET;               // save_count offset
  *(DWORD*)(0x006C26EF) = SAVE_COUNT_OFFSET;               // save_count offset
  *(DWORD*)(0x006C2705) = ROUND2_SEED_OFFSET;              // round2_seed offset
  *(DWORD*)(0x006C2793) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C286C) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C3113) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C353F) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C357E) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C35AA) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C35B7) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C35D2) = SAVE_COUNT_OFFSET;               // save_count offset
  *(DWORD*)(0x006C35DB) = SAVE_COUNT_OFFSET;               // save_count offset
  *(DWORD*)(0x006C36E0) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C3B1E) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C4209) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C47EF) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C4826) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C4962) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C4999) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C4A81) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C4ABA) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C4C9A) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C4CD1) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C4DB9) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C4DF2) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C4F58) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C4F94) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C5181) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C51BD) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C5332) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C536C) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C5501) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C553D) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C56B2) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C56EC) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C5872) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C58AC) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C5A41) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C5A7D) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C5B6E) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C5BA8) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C5D2E) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C5D68) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C5EEE) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C5F28) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C60AE) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C60E8) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C6303) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C633D) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C64C1) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C64FD) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C65EE) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C6628) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C67AE) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C67E8) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C696E) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C69A8) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C6B43) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C6B74) = 0x0000005D;                      // memcard block count
  *(DWORD*)(0x006C6BF6) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C6C30) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C6E3E) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C6E78) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C7075) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C70AF) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C7A02) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x006C7D22) = TOTAL_FILE_SIZE;                 // total file size
  *(BYTE*)(0x006C7D5E) = SLOT_COUNT;                       // slot count
  *(DWORD*)(0x006C7D7C) = TOTAL_FILE_SIZE;                 // total file size
  *(DWORD*)(0x0077BE92) = BGM_TEST_SONGS_UNLOCKED_OFFSET;  // bgm_test_songs_unlocked offset
}

void patch_moresaveslots(void) {
  if (*(ULONGLONG*)0x00413B7C != 0x909090C35F000000)       // already patched
    return;

  *(BYTE*)(0x00413B7F) = 0xE9; // jmp
  *(ULONG_PTR*)(0x00413B80) = calc_disp32(0x00413B80, (ULONG_PTR)enableScroll);

  *(BYTE*)(0x00413C38) = 0xE8; // call
  *(ULONG_PTR*)(0x00413C39) = calc_disp32(0x00413C39, (ULONG_PTR)fixScrollSelection);
  *(BYTE*)(0x00413C3D) = 0x90; // nop

  *(BYTE*)(0x00413CD8) = 0xE8; // call
  *(ULONG_PTR*)(0x00413CD9) = calc_disp32(0x00413CD9, (ULONG_PTR)fixScrollIndex);
  *(BYTE*)(0x00413CDD) = 0x90; // nop

  *(BYTE*)(0x00413CF0) = 0xE8; // call
  *(ULONG_PTR*)(0x00413CF1) = calc_disp32(0x00413CF1, (ULONG_PTR)fixCharacterFileIndex);

  *(BYTE*)(0x0040216C) = 0xE8; // call
  *(ULONG_PTR*)(0x0040216D) = calc_disp32(0x0040216D, (ULONG_PTR)fixPreviewWindow);

  *(BYTE*)(0x00401842) = 0xE8; // call
  *(ULONG_PTR*)(0x00401843) = calc_disp32(0x00401843, (ULONG_PTR)fixPreviewWindow);

  memcpy((void*)0x006C1C2D, fixSaveFileSignatureCheck, sizeof(fixSaveFileSignatureCheck));
  memcpy((void*)0x00401D57, addMenuSlotNumber, sizeof(addMenuSlotNumber));
  memset((void*)((DWORD)0x00401D57+sizeof(addMenuSlotNumber)), 0, 155);

  fix_references();
}

__declspec(dllexport) void __stdcall load(void) {
  if (GetImageSize(0) < 0x00762000 || *(DWORD*)0x00B613FA != 0x4C4E3935) { // 59NL
    MessageBox(0, "MoreSaveSlots: Wrong client version, expected MTethVer12513 (1.25.13)", "Error", MB_ICONERROR);
    return;
  }

  patch_moresaveslots();
}

int __stdcall DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH)
    DisableThreadLibraryCalls(hInstDLL);

  return TRUE;
}
