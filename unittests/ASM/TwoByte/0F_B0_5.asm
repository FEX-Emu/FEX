%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x5152535455FFFF58",
    "RCX": "0x616263FFFF666768",
    "RDX": "0xFF72737475767778",
    "RBP": "0x81828384858687FF",
    "RSI": "0xB1B2B3B4B5B6B7B8",
    "RDI": "0xC1C2C3C4C5C6C7C8",
    "RSP": "0xFF42434445464748",
    "R8":  "0x51525354555657FF",
    "R9":  "0x6465646556574647",
    "R11": "0x0000000000005841",
    "R12": "0xC8B1A89188718871"
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

; 16bit unaligned edges test
; Offsets   | Test                                |
; =============================================================
; 1         | Misaligned inside 32bit region      | 32bit CAS
; 3         | Misaligned through to 64bit region  | 64bit CAS
; 7         | Misaligned through to 128bit region | 128bit CAS
; 15        | Misaligned through to 256bit region | Dual 8bit/64bit CAS *CAN TEAR*
; 63        | Misaligned across 64byte cachelines | Dual 8bit/64bit CAS *CAN TEAR*

; Offset 1
; False
mov rax, 0
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 0 + 1], cx
mov [r10 + 8 * 9 + 0], ax

; True
mov rax, 0x5657
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 1 + 1], cx
mov [r10 + 8 * 9 + 2], ax

; Offset 3
; False
mov rax, 0
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 2 + 3], cx
mov [r10 + 8 * 9 + 4], ax

; True
mov rax, 0x6465
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 2 + 3], cx
mov [r10 + 8 * 9 + 6], ax

; Offset 7
; False
mov rax, 0
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 3 + 7], cx
mov [r10 + 8 * 10 + 0], ax

; True
mov rax, 0x8871
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 3 + 7], cx
mov [r10 + 8 * 10 + 2], ax

; Offset 15
; False
mov rax, 0
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 4 + 15], cx
mov [r10 + 8 * 10 + 4], ax

; True
mov rax, 0x8871
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 4 + 15], cx
mov [r10 + 8 * 10 + 6], ax

; Offset 63
; False
mov rax, 0
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 7 + 7], cx
mov [r10 + 8 * 10 + 6], ax

; True
mov rax, 0x5841
mov rcx, 0xFFFF
cmpxchg [r10 + 8 * 15 + 7], cx
mov [r10 + 8 * 11 + 0], ax

mov rax, [r10 + 8 * 0]
mov rbx, [r10 + 8 * 1]
mov rcx, [r10 + 8 * 2]
mov rdx, [r10 + 8 * 3]
mov rbp, [r10 + 8 * 4]

mov rsi, [r10 + 8 * 7]
mov rdi, [r10 + 8 * 8]

mov rsp, [r10 + 8 * 15]
mov r8, [r10 + 8 * 16]

mov r9, [r10 + 8 * 9]
mov r12, [r10 + 8 * 10]
mov r11, [r10 + 8 * 11]

hlt

