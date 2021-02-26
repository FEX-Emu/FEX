%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x8642434446464748",
    "RBX": "0x8242434445888a8c",
    "RCX": "0x41424344458c8e90",
    "RDX": "0x4142434445464748",
    "RSI": "0x8242434445464748",
    "RDI": "0x41424344458c8e90"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0x4142434445464748
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

mov rax, 1

; Test 3 byte offset across 4byte boundary
lock xadd dword [r15 + 8 * 0 + 3], eax

; Test 7 byte offset across 8byte boundary
lock xadd dword [r15 + 8 * 0 + 7], eax

; Test 15 byte offset across 16byte boundary
lock xadd dword [r15 + 8 * 0 + 15], eax

; Test 63 byte offset across cacheline boundary
lock xadd dword [r15 + 8 * 0 + 63], eax

mov rax, qword [r15 + 8 * 0]
mov rbx, qword [r15 + 8 * 1]
mov rcx, qword [r15 + 8 * 2]
mov rdx, qword [r15 + 8 * 3]
mov rsi, qword [r15 + 8 * 7]
mov rdi, qword [r15 + 8 * 8]

hlt
