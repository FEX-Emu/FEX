%ifdef CONFIG
{
  "RegData": {
    "R15": "0x4142434445466148",
    "R14": "0x4142434445466100",
    "R13": "0x4142434445466148",
    "R12": "0x4142434445460000",
    "R11": "0x4142434445466148",
    "R10": "0x0000000000000000",
    "R9":  "0x4142434445466148",
    "R8":  "0x0000000000000000"
  }
}
%endif

; 8bit
mov rcx, 0x4142434445466148

; Not a match
mov rax, 0xFFFFFFFFFFFFFF47
mov rbx, 0
cmpxchg cl, bl
mov r15, rcx

; Match
mov rax, 0xFFFFFFFFFFFFFF48
mov rbx, 0
cmpxchg cl, bl
mov r14, rcx

; 16bit
mov rcx, 0x4142434445466148

; Not a match
mov rax, 0xFFFFFFFFFFFF4748
mov rbx, 0
cmpxchg cx, bx
mov r13, rcx

; Match
mov rax, 0xFFFFFFFFFFFF6148
mov rbx, 0
cmpxchg cx, bx
mov r12, rcx

; 32bit
mov rcx, 0x4142434445466148

; Not a match
mov rax, 0xFFFFFFFF45464748
mov rbx, 0
cmpxchg ecx, ebx
mov r11, rcx

; Match
mov rax, 0xFFFFFFFF45466148
mov rbx, 0
cmpxchg ecx, ebx
mov r10, rcx

; 64bit
mov rcx, 0x4142434445466148

; Not a match
mov rax, 0xFFFFFFFF45464748
mov rbx, 0
cmpxchg rcx, rbx
mov r9, rcx

; Match
mov rax, 0x4142434445466148
mov rbx, 0
cmpxchg rcx, rbx
mov r8, rcx

hlt
