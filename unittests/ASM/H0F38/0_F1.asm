%ifdef CONFIG
{
  "RegData": {
    "R15": "0x4142434445465857",
    "R14": "0x4142434458575655",
    "R13": "0x5857565554535251"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x5152535455565758
mov [rdx + 8 * 0], rax
mov rax, 0x4142434445464748
mov [rdx + 8 * 1], rax
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax

mov r15, [rdx + 8 * 0]
mov r14, [rdx + 8 * 0]
mov r13, [rdx + 8 * 0]

movbe word [rdx + 8 * 1], r15w
movbe dword [rdx + 8 * 2], r14d
movbe qword [rdx + 8 * 3], r13

mov r15, [rdx + 8 * 1]
mov r14, [rdx + 8 * 2]
mov r13, [rdx + 8 * 3]

hlt
