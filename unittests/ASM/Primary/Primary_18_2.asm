%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x08",
    "RCX": "0x9918",
    "RDX": "0x9A999928",
    "RBP": "0x9E9D9C9B9A999938",
    "RDI": "0x80",
    "RSP": "0x7F80",
    "R8":  "0x7F7F7F80",
    "R9":  "0x01",
    "R10": "0x4142427244744776",
    "R11": "0x5152535455565686",
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
sbb byte  [r15 + 8 * 0 + 0], al
stc
sbb word  [r15 + 8 * 0 + 2], ax
stc
sbb dword [r15 + 8 * 0 + 4], eax
stc
sbb qword [r15 + 8 * 1 + 0], rax

mov rbx, 0x71
mov rcx, 0x81
mov rdx, 0x91
mov rbp, 0xA1

stc
sbb bl,  byte  [r15 + 8 * 2]
stc
sbb cx,  word  [r15 + 8 * 2]
stc
sbb edx, dword [r15 + 8 * 2]
stc
sbb rbp, qword [r15 + 8 * 2]

mov rax, 0x01
stc
sbb al, 0x80
mov rdi, rax

mov rax, 0x01
stc
sbb ax, 0x8080
mov rsp, rax

mov rax, 0x01
stc
sbb eax, 0x80808080
mov r8, rax

mov rax, 0x01
stc
sbb rax, -1
mov r9, rax

mov r10, [r15 + 8 * 0]
mov r11, [r15 + 8 * 1]
mov r12, [r15 + 8 * 2]

hlt
