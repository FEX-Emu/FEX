%ifdef CONFIG
{
  "RegData": {
    "R15": "0x414243444546FFFF",
    "R14": "0x00000000FFFFFFFF",
    "R13": "0xFFFFFFFFFFFFFFFF",
    "R12": "0x414243444546FFFF",
    "R11": "0x00000000FFFFFFFF",
    "R10": "0xFFFFFFFFFFFFFFFF"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 0], rax

mov r15, 0x4142434445464748
mov r14, 0x4142434445464748
mov r13, 0x4142434445464748
mov r12, 0x4142434445464748
mov r11, 0x4142434445464748
mov r10, 0x4142434445464748

movsx r15w, byte [rdx + 8 * 0]
movsx r14d, byte [rdx + 8 * 0]
movsx r13,  byte [rdx + 8 * 0]

movsx r12w, al
movsx r11d, al
movsx r10,  al

hlt
