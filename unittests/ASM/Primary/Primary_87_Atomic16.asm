%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0142430001000148",
    "RBX": "0x0142434445464700",
    "RCX": "0x4142434445464700",
    "RDX": "0x4142434445464748",
    "RSI": "0x0142434445464748",
    "RDI": "0x4142434445464700",
    "R14": "0x0000000000004647",
    "R13": "0x0000000000004445",
    "R12": "0x0000000000004841",
    "R11": "0x0000000000004841",
    "R10": "0x0000000000004841"
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

; Test 1 byte offset within 4byte boundary
mov rax, 1
xchg word [r15 + 8 * 0 + 1], ax
mov r14, rax

; Test 3 byte offset across 4byte boundary
mov rax, 1
xchg word [r15 + 8 * 0 + 3], ax
mov r13, rax

; Test 7 byte offset across 8byte boundary
mov rax, 1
xchg word [r15 + 8 * 0 + 7], ax
mov r12, rax

; Test 15 byte offset across 16byte boundary
mov rax, 1
xchg word [r15 + 8 * 0 + 15], ax
mov r11, rax

; Test 63 byte offset across cacheline boundary
mov rax, 1
xchg word [r15 + 8 * 0 + 63], ax
mov r10, rax

mov rax, qword [r15 + 8 * 0]
mov rbx, qword [r15 + 8 * 1]
mov rcx, qword [r15 + 8 * 2]
mov rdx, qword [r15 + 8 * 3]
mov rsi, qword [r15 + 8 * 7]
mov rdi, qword [r15 + 8 * 8]

hlt
