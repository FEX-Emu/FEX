%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x19",
    "RCX": "0x67E9",
    "RDX": "0x656667F9",
    "RBP": "0x61626364656667C9",
    "RDI": "0x81",
    "RSP": "0x8081",
    "R8":  "0x80808081",
    "R9":  "0xFFFFFFFFFFFFFFFE",
    "R10": "0x4142439545974799",
    "R11": "0x5152535455565789",
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
xor byte  [r15 + 8 * 0 + 0], al
xor word  [r15 + 8 * 0 + 2], ax
xor dword [r15 + 8 * 0 + 4], eax
xor qword [r15 + 8 * 1 + 0], rax

mov rbx, 0x71
mov rcx, 0x81
mov rdx, 0x91
mov rbp, 0xA1

xor bl,  byte  [r15 + 8 * 2]
xor cx,  word  [r15 + 8 * 2]
xor edx, dword [r15 + 8 * 2]
xor rbp, qword [r15 + 8 * 2]

mov rax, 0x01
xor al, 0x80
mov rdi, rax

mov rax, 0x01
xor ax, 0x8080
mov rsp, rax

mov rax, 0x01
xor eax, 0x80808080
mov r8, rax

mov rax, 0x01
xor rax, -1
mov r9, rax

mov r10, [r15 + 8 * 0]
mov r11, [r15 + 8 * 1]
mov r12, [r15 + 8 * 2]

hlt
