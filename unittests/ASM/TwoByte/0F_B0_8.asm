%ifdef CONFIG
{
  "RegData": {
    "R15": "0x4142434445469748",
    "R14": "0x4142434445464600",
    "R13": "0x4142434445468748",
    "R12": "0x4142434445464600",
    "R11": "0x4142434445468748",
    "R10": "0x4142434400004600",
    "R9":  "0x4142434445468748",
    "R8":  "0x0000000000004600"
  }
}
%endif

mov rdx, 0xe0000000

; 8bit
mov rax, 0x4142434445466148
mov [rdx + 8 * 0], rax

; Not a match
mov rax, 0x47
mov rbx, 0
cmpxchg byte [rdx + 8 * 0], bl
mov rax, [rdx + 8 * 0]
lahf
mov r15, rax

; Match
mov rax, 0x48
mov rbx, 0
cmpxchg byte [rdx + 8 * 0], bl
mov rax, [rdx + 8 * 0]
lahf
mov r14, rax

; 16bit
mov rax, 0x4142434445466148
mov [rdx + 8 * 0], rax

; Not a match
mov rax, 0x4748
mov rbx, 0
cmpxchg word [rdx + 8 * 0], bx
mov rax, [rdx + 8 * 0]
lahf
mov r13, rax

; Match
mov rax, 0x6148
mov rbx, 0
cmpxchg word [rdx + 8 * 0], bx
mov rax, [rdx + 8 * 0]
lahf
mov r12, rax

; 32bit
mov rax, 0x4142434445466148
mov [rdx + 8 * 0], rax

; Not a match
mov rax, 0x45464748
mov rbx, 0
cmpxchg dword [rdx + 8 * 0], ebx
mov rax, [rdx + 8 * 0]
lahf
mov r11, rax

; Match
mov rax, 0x45466148
mov rbx, 0
cmpxchg dword [rdx + 8 * 0], ebx
mov rax, [rdx + 8 * 0]
lahf
mov r10, rax

; 64bit
mov rax, 0x4142434445466148
mov [rdx + 8 * 0], rax

; Not a match
mov rax, 0x45464748
mov rbx, 0
cmpxchg qword [rdx + 8 * 0], rbx
mov rax, [rdx + 8 * 0]
lahf
mov r9, rax

; Match
mov rax, 0x4142434445466148
mov rbx, 0
cmpxchg qword [rdx + 8 * 0], rbx
mov rax, [rdx + 8 * 0]
lahf
mov r8, rax

hlt
