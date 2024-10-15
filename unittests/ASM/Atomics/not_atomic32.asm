%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xbebdbcbbba464748",
    "RBX": "0xbe42434445b9b8b7",
    "RCX": "0x4142434445b9b8b7",
    "RDX": "0x4142434445464748",
    "RSI": "0xbe42434445464748",
    "RDI": "0x4142434445b9b8b7"
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

; Test 3 byte offset across 4byte boundary
lock not dword [r15 + 8 * 0 + 3]

; Test 7 byte offset across 8byte boundary
lock not dword [r15 + 8 * 0 + 7]

; Test 15 byte offset across 16byte boundary
lock not dword [r15 + 8 * 0 + 15]

; Test 63 byte offset across cacheline boundary
lock not dword [r15 + 8 * 0 + 63]

mov rax, qword [r15 + 8 * 0]
mov rbx, qword [r15 + 8 * 1]
mov rcx, qword [r15 + 8 * 2]
mov rdx, qword [r15 + 8 * 3]
mov rsi, qword [r15 + 8 * 7]
mov rdi, qword [r15 + 8 * 8]

hlt
