%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xffffffffffffff48",
    "RBX": "0xffffffffffffffff",
    "RCX": "0x61626364656667ff",
    "RDI": "0xffffffffffffffb8",
    "RSP": "0xc1c2c3c4c5c6c7ff",
    "R8":  "0x6851525354555657",
    "R9":  "0xc8b1b2b3b4b5b6b7",
    "R10": "0xc8b1b2b3b4b5b6b7"
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

; 64bit unaligned edges test
; Offsets       | Test                                |
; =============================================================
; [1,7]           | Misaligned through to 128bit region | 128bit CAS
; [9,15], [57,63] | Misaligned through to 256bit region | Dual 64bit CAS

; Offset 1
; False
mov rax, 0
mov rcx, 0xFFFFFFFFFFFFFFFF
cmpxchg [r10 + 8 * 0 + 1], rcx
mov [r10 + 8 * 9 + 0], rax

; True
mov rax, 0x5841424344454647
mov rcx, 0xFFFFFFFFFFFFFFFF
cmpxchg [r10 + 8 * 0 + 1], rcx
mov [r10 + 8 * 10], rax

; Offset 9
; False
mov rax, 0
mov rcx, 0xFFFFFFFFFFFFFFFF
cmpxchg [r10 + 8 * 1 + 1], rcx
mov [r10 + 8 * 9 + 0], rax

; True
mov rax, 0x6851525354555657
mov rcx, 0xFFFFFFFFFFFFFFFF
cmpxchg [r10 + 8 * 1 + 1], rcx
mov [r10 + 8 * 10], rax

; Offset 57
; False
mov rax, 0
mov rcx, 0xFFFFFFFFFFFFFFFF
cmpxchg [r10 + 8 * 7 + 1], rcx
mov [r10 + 8 * 11 + 0], rax

; True
mov rax, 0xC8B1B2B3B4B5B6B7
mov rcx, 0xFFFFFFFFFFFFFFFF
cmpxchg [r10 + 8 * 7 + 1], rcx
mov [r10 + 8 * 12], rax

mov rax, [r10 + 8 * 0]
mov rbx, [r10 + 8 * 1]
mov rcx, [r10 + 8 * 2]
mov rdi, [r10 + 8 * 7]
mov rsp, [r10 + 8 * 8]

mov r8, [r10 + 8 * 10]
mov r9, [r10 + 8 * 11]
mov r10, [r10 + 8 * 12]

hlt

