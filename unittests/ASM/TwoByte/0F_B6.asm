%ifdef CONFIG
{
  "RegData": {
    "R15": "0x41424344454600FF",
    "R14": "0x00000000000000FF",
    "R13": "0x00000000000000FF",
    "R12": "0x41424344454600FF",
    "R11": "0x00000000000000FF",
    "R10": "0x00000000000000FF"
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

movzx r15w, byte [rdx + 8 * 0]
movzx r14d, byte [rdx + 8 * 0]
movzx r13,  byte [rdx + 8 * 0]

movzx r12w, al
movzx r11d, al
movzx r10,  al

hlt
