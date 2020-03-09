%ifdef CONFIG
{
  "RegData": {
    "R14": "0x000000000000FFFF",
    "R13": "0x000000000000FFFF",
    "R12": "0x000000000000FFFF",
    "R11": "0x000000000000FFFF"
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

movzx r14d, word [rdx + 8 * 0]
movzx r13,  word [rdx + 8 * 0]

movzx r12d, ax
movzx r11,  ax

hlt
