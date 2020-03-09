%ifdef CONFIG
{
  "RegData": {
    "R15": "0xFFFFFFFFFFFF000F",
    "R14": "0x1F",
    "R13": "0x3F",
    "R12": "0xFFFFFFFFFFFF000C",
    "R11": "0x1C",
    "R10": "0x3C",
    "R9":  "0xFFFFFFFFFFFFFFFF",
    "R8":  "0xFFFFFFFFFFFFFFFF",
    "RSI": "0xFFFFFFFFFFFFFFFF"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 0], rax
mov rax, 0x1010101010101010
mov [rdx + 8 * 1], rax
mov rax, 0
mov [rdx + 8 * 2], rax

mov r15, -1
mov r14, -1
mov r13, -1
mov r12, -1
mov r11, -1
mov r10, -1
mov r9,  -1
mov r8,  -1
mov rsi, -1

bsr r15w, word  [rdx + 8 * 0]
bsr r14d, dword [rdx + 8 * 0]
bsr r13,  qword [rdx + 8 * 0]

bsr r12w, word  [rdx + 8 * 1]
bsr r11d, dword [rdx + 8 * 1]
bsr r10,  qword [rdx + 8 * 1]

bsr r9w, word  [rdx + 8 * 2]
bsr r8d, dword [rdx + 8 * 2]
bsr rsi, qword [rdx + 8 * 2]

hlt
