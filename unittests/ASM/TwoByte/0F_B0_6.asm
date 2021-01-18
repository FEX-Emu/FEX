%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414243ffffffff48",
    "RBX": "0x5152535455565758",
    "RCX": "0xffffff6465666768",
    "RDX": "0x71727374757677ff",
    "RBP": "0xffffff8485868788",
    "RSI": "0xffffffb4b5b6b7b8",
    "RDI": "0xc1c2c3c4c5c6c7ff",
    "RSP": "0x4445464744454647",
    "R8":  "0x7861626378616263",
    "R9":  "0x9881828398818283",
    "R10": "0xc8b1b2b3c8b1b2b3"
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
mov rax, 0x8182838485868788
mov [r10 + 8 * 4], rax
mov rax, 0x9192939495969798
mov [r10 + 8 * 5], rax
mov rax, 0xA1A2A3A4A5A6A7A8
mov [r10 + 8 * 6], rax
mov rax, 0xB1B2B3B4B5B6B7B8
mov [r10 + 8 * 7], rax
mov rax, 0xC1C2C3C4C5C6C7C8
mov [r10 + 8 * 8], rax

mov rax, 0
mov [r10 + 8 * 9], rax
mov [r10 + 8 * 10], rax
mov [r10 + 8 * 11], rax
mov [r10 + 8 * 12], rax
mov [r10 + 8 * 13], rax

mov rax, 0x4142434445464748
mov [r10 + 8 * 15], rax
mov rax, 0x5152535455565758
mov [r10 + 8 * 16], rax

; 32bit unaligned edges test
; Offsets       | Test                                |
; =============================================================
; 1,2,3         | Misaligned through to 64bit region  | 64bit CAS
; 5,6,7,9,10,11 | Misaligned through to 128bit region | 128bit CAS
; 13,14,15      | Misaligned through to 256bit region | Dual 32bit/64bit CAS *CAN TEAR*
; 61,62,63      | Misaligned across 64byte cachelines | Dual 32bit/64bit CAS *CAN TEAR*

; Offset 1
; False
mov rax, 0
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 0 + 1], ecx
mov [r10 + 8 * 9 + 0], eax

; True
mov rax, 0x44454647
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 0 + 1], ecx
mov [r10 + 8 * 9 + 4], eax

; Offset 5
; False
mov rax, 0
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 2 + 5], ecx
mov [r10 + 8 * 10 + 0], eax

; True
mov rax, 0x78616263
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 2 + 5], ecx
mov [r10 + 8 * 10 + 4], eax

; Offset 13
; False
mov rax, 0
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 4 + 5], ecx
mov [r10 + 8 * 11 + 0], eax

; True
mov rax, 0x98818283
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 4 + 5], ecx
mov [r10 + 8 * 11 + 4], eax

; Offset 61
; False
mov rax, 0
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 7 + 5], ecx
mov [r10 + 8 * 12 + 0], eax

; Wrong
; True
mov rax, 0xC8B1B2B3
mov rcx, 0xFFFFFFFF
cmpxchg [r10 + 8 * 7 + 5], ecx
mov [r10 + 8 * 12 + 4], eax

mov rax, [r10 + 8 * 0]
mov rbx, [r10 + 8 * 1]
mov rcx, [r10 + 8 * 2]
mov rdx, [r10 + 8 * 3]
mov rbp, [r10 + 8 * 4]
mov rsi, [r10 + 8 * 7]
mov rdi, [r10 + 8 * 8]
mov rsp, [r10 + 8 * 9]

mov r8, [r10 + 8 * 10]
mov r9, [r10 + 8 * 11]
mov r10, [r10 + 8 * 12]

hlt

