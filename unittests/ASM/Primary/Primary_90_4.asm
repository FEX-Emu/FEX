%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFFFF",
    "RBX": "0x4242424242424242",
    "RCX": "0xFFFFFFFFFFFFFFFF",
    "RDX": "0x4242424242424242",
    "RBP": "0xFFFFFFFFFFFFFFFF",
    "RSI": "0x4242424242424242",
    "RDI": "0xFFFFFFFFFFFFFFFF",
    "RSP": "0x4242424242424242",
    "R8":  "0xFFFFFFFFFFFFFFFF",
    "R9":  "0x4242424242424242",
    "R10": "0xFFFFFFFFFFFFFFFF",
    "R11": "0x4242424242424242",
    "R12": "0xFFFFFFFFFFFFFFFF",
    "R13": "0x4242424242424242",
    "R14": "0xFFFFFFFFFFFFFFFF",
    "R15": "0x4242424242424242"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

%macro swap64 2
mov %1, -1
mov rax, 0x4242424242424242
xchg %1, rax

mov qword [r15 + 16 * %2 + 0], rax
mov qword [r15 + 16 * %2 + 8], %1

%endmacro

mov r15, 0xe0000000
mov rax, 0

mov [r15 + 8 * 0], rax
mov [r15 + 8 * 1], rax
mov [r15 + 8 * 2], rax
mov [r15 + 8 * 3], rax
mov [r15 + 8 * 4], rax
mov [r15 + 8 * 5], rax
mov [r15 + 8 * 6], rax
mov [r15 + 8 * 7], rax
mov [r15 + 8 * 8], rax
mov [r15 + 8 * 9], rax
mov [r15 + 8 * 10], rax
mov [r15 + 8 * 11], rax
mov [r15 + 8 * 12], rax
mov [r15 + 8 * 13], rax
mov [r15 + 8 * 14], rax
mov [r15 + 8 * 15], rax

swap64 r9, 0
swap64 r10, 1
swap64 r11, 2
swap64 r12, 3
swap64 r13, 4
swap64 r14, 5
swap64 r14, 6
swap64 rsp, 7

mov rax, [r15 + 16 * 0 + 0]
mov rbx, [r15 + 16 * 0 + 8]
mov rcx, [r15 + 16 * 1 + 0]
mov rdx, [r15 + 16 * 1 + 8]
mov rbp, [r15 + 16 * 2 + 0]
mov rsi, [r15 + 16 * 2 + 8]
mov rdi, [r15 + 16 * 3 + 0]
mov rsp, [r15 + 16 * 3 + 8]
mov r8,  [r15 + 16 * 4 + 0]
mov r9,  [r15 + 16 * 4 + 8]
mov r10, [r15 + 16 * 5 + 0]
mov r11, [r15 + 16 * 5 + 8]
mov r12, [r15 + 16 * 6 + 0]
mov r13, [r15 + 16 * 6 + 8]
mov r14, [r15 + 16 * 7 + 0]
mov r15, [r15 + 16 * 7 + 8]

hlt
