%ifdef CONFIG
{
  "RegData": {
    "RBX": "0xDA",
    "RCX": "0x67EA",
    "RDX": "0x656667FA",
    "RBP": "0x616263646566680A",
    "RDI": "0x82",
    "RSP": "0x8082",
    "R8":  "0x80808082",
    "R9":  "0x1",
    "R10": "0x414244164618471A",
    "R11": "0x515253545556582A",
    "R12": "0x6162636465666768"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0x4142434445464748
mov [r15 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 1], rax
mov rax, 0x6162636465666768
mov [r15 + 8 * 2], rax

mov rax, 0xD1
stc
adc byte  [r15 + 8 * 0 + 0], al
stc
adc word  [r15 + 8 * 0 + 2], ax
stc
adc dword [r15 + 8 * 0 + 4], eax
stc
adc qword [r15 + 8 * 1 + 0], rax

mov rbx, 0x71
mov rcx, 0x81
mov rdx, 0x91
mov rbp, 0xA1

stc
adc bl,  byte  [r15 + 8 * 2]
stc
adc cx,  word  [r15 + 8 * 2]
stc
adc edx, dword [r15 + 8 * 2]
stc
adc rbp, qword [r15 + 8 * 2]

mov rax, 0x01
stc
adc al, 0x80
mov rdi, rax

mov rax, 0x01
stc
adc ax, 0x8080
mov rsp, rax

mov rax, 0x01
stc
adc eax, 0x80808080
mov r8, rax

mov rax, 0x01
stc
adc rax, -1
mov r9, rax

mov r10, [r15 + 8 * 0]
mov r11, [r15 + 8 * 1]
mov r12, [r15 + 8 * 2]

hlt
