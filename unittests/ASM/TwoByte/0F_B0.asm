%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414243444546FF48",
    "RBX": "0x51525354FFFF5758",
    "RCX": "0xFFFFFFFF65666768",
    "RDX": "0xFFFFFFFFFFFFFFFF",
    "RBP": "0x4748",
    "RSI": "0x55565758",
    "RDI": "0x6162636465666768",
    "RSP": "0x7172737475767778",
    "R8": "0x7172737475767778"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r10, 0xe0000000

mov rax, 0x4142434445464748
mov [r10 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r10 + 8 * 1], rax
mov rax, 0x6162636465666768
mov [r10 + 8 * 2], rax
mov rax, 0x7172737475767778
mov [r10 + 8 * 3], rax

mov rax, 0
mov [r10 + 8 * 4], rax
mov [r10 + 8 * 5], rax
mov [r10 + 8 * 6], rax
mov [r10 + 8 * 7], rax
mov [r10 + 8 * 8], rax

; False
mov rax, 0
mov rcx, 0xFF
cmpxchg [r10 + 8 * 0 + 0], cl
mov [r10 + 8 * 4 + 0], al

; True
mov rax, 0x47
mov rcx, 0xFF
cmpxchg [r10 + 8 * 0 + 1], cl
mov [r10 + 8 * 4 + 1], al

; False
mov rax, 0
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 1 + 0], cx
mov [r10 + 8 * 5 + 0], ax

; True
mov rax, 0x5556
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 1 + 2], cx
mov [r10 + 8 * 5 + 2], ax

; False
mov rax, 0
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 2 + 0], ecx
mov [r10 + 8 * 6 + 0], eax

; True
mov rax, 0x61626364
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 2 + 4], ecx
mov [r10 + 8 * 6 + 4], eax

; False
mov rax, 0
mov rcx, 0xFFFFFFFFFFFFFFFF
cmpxchg [r10 + 8 * 3 + 0], rcx
mov [r10 + 8 * 7 + 0], rax

; True
mov rax, 0x7172737475767778
mov rcx, 0xFFFFFFFFFFFFFFFF
cmpxchg [r10 + 8 * 3 + 0], rcx
mov [r10 + 8 * 8], rax

mov rax, [r10 + 8 * 0]
mov rbx, [r10 + 8 * 1]
mov rcx, [r10 + 8 * 2]
mov rdx, [r10 + 8 * 3]
mov rbp, [r10 + 8 * 4]
mov rsi, [r10 + 8 * 5]
mov rdi, [r10 + 8 * 6]
mov rsp, [r10 + 8 * 7]
mov r8, [r10 + 8 * 8]

hlt
