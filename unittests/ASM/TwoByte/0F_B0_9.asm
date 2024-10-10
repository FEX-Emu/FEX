%ifdef CONFIG
{
  "RegData": {
    "R15": "0xffffffffffff9748",
    "R14": "0xffffffffffff4648",
    "R13": "0xffffffffffff8748",
    "R12": "0xffffffffffff4648",
    "R11": "0x0000000045468748",
    "R10": "0xffffffff45464648",
    "R9":  "0x4142434445468648",
    "R8":  "0x4142434445464648"
  }
}
%endif

mov rdx, 0xe0000000

; 8bit
mov rax, 0x4142434445466148
mov [rdx + 8 * 0], rax

; Not a match
mov rax, 0xFFFFFFFFFFFFFF47
mov rbx, 0
cmpxchg byte [rdx + 8 * 0], bl
lahf
mov r15, rax

; Match
mov rax, 0xFFFFFFFFFFFFFF48
mov rbx, 0
cmpxchg byte [rdx + 8 * 0], bl
lahf
mov r14, rax

; 16bit
mov rax, 0x4142434445466148
mov [rdx + 8 * 0], rax

; Not a match
mov rax, 0xFFFFFFFFFFFF4748
mov rbx, 0
cmpxchg word [rdx + 8 * 0], bx
lahf
mov r13, rax

; Match
mov rax, 0xFFFFFFFFFFFF6148
mov rbx, 0
cmpxchg word [rdx + 8 * 0], bx
lahf
mov r12, rax

; 32bit
mov rax, 0x4142434445466148
mov [rdx + 8 * 0], rax

; Not a match
mov rax, 0xFFFFFFFF45464748
mov rbx, 0
cmpxchg dword [rdx + 8 * 0], ebx
lahf
mov r11, rax

; Match
mov rax, 0xFFFFFFFF45466148
mov rbx, 0
cmpxchg dword [rdx + 8 * 0], ebx
lahf
mov r10, rax

; 64bit
mov rax, 0x4142434445466148
mov [rdx + 8 * 0], rax

; Not a match
mov rax, 0xFFFFFFFF45464748
mov rbx, 0
cmpxchg qword [rdx + 8 * 0], rbx
lahf
mov r9, rax

; Match
mov rax, 0x4142434445466148
mov rbx, 0
cmpxchg qword [rdx + 8 * 0], rbx
lahf
mov r8, rax

hlt
