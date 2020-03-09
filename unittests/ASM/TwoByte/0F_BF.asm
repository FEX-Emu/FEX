%ifdef CONFIG
{
  "RegData": {
    "R14": "0x00000000FFFFFFFF",
    "R13": "0xFFFFFFFFFFFFFFFF",
    "R12": "0x00000000FFFFFFFF",
    "R11": "0xFFFFFFFFFFFFFFFF"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 0], rax

mov r14, 0x4142434445464748
mov r13, 0x4142434445464748
mov r12, 0x4142434445464748
mov r11, 0x4142434445464748

movsx r14d, word [rdx + 8 * 0]
movsx r13,  word [rdx + 8 * 0]

movsx r12d, ax
movsx r11,  ax

hlt
