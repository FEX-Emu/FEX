%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4042434445464748",
    "RBX": "0x4042434445464748",
    "RCX": "0x4142434445464748",
    "RDX": "0x4142434445464748",
    "RSI": "0x4042434445464748",
    "RDI": "0x4142434445464748"
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

; Test 7 byte offset across 8byte boundary
lock sbb qword [r15 + 8 * 0 + 7], rax

; Test 15 byte offset across 16byte boundary
lock sbb qword [r15 + 8 * 0 + 15], rax

; Test 63 byte offset across cacheline boundary
lock sbb qword [r15 + 8 * 0 + 63], rax

mov rax, qword [r15 + 8 * 0]
mov rbx, qword [r15 + 8 * 1]
mov rcx, qword [r15 + 8 * 2]
mov rdx, qword [r15 + 8 * 3]
mov rsi, qword [r15 + 8 * 7]
mov rdi, qword [r15 + 8 * 8]

hlt
