%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xffffffffffffff00",
    "RBX": "0xffffffffffff0000",
    "RCX": "0xffffffff00000000",
    "RDX": "0x0000000000000000",
    "R8" : "0x0000000000005400",
    "R9" : "0x0000000000005400",
    "R10": "0x0000000000005400",
    "R11": "0x0000000000005400"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0xffffffffffffffff
mov [r15 + 8 * 0], rax
mov [r15 + 8 * 1], rax
mov [r15 + 8 * 2], rax
mov [r15 + 8 * 3], rax

xor rax, rax

; Insure that inc overflow works and sets correct flags
inc  byte [r15 + 8 * 0 + 0]
lahf
mov r8, rax

inc  word [r15 + 8 * 1 + 0]
lahf
mov r9, rax

inc dword [r15 + 8 * 2 + 0]
lahf
mov r10, rax

inc qword [r15 + 8 * 3 + 0]
lahf
mov r11, rax


mov rax, [r15 + 8 * 0]
mov rbx, [r15 + 8 * 1]
mov rcx, [r15 + 8 * 2]
mov rdx, [r15 + 8 * 3]


; Mask flags we don't care about
and r8, 0xd400
and r9, 0xd400
and r10, 0xd400
and r11, 0xd400

hlt
