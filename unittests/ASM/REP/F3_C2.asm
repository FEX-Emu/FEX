%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x51525354FFFFFFFF", "0x5152535440000000"],
    "XMM1": ["0x5152535400000000", "0x5152535440000000"],
    "XMM2": ["0x51525354FFFFFFFF", "0x5152535440000000"],
    "XMM3": ["0x0000000000000000", "0x7FC000007FC00000"],
    "XMM4": ["0x5152535400000000", "0x5152535440000000"],
    "XMM5": ["0x51525354FFFFFFFF", "0x5152535440000000"],
    "XMM6": ["0x5152535400000000", "0x5152535440000000"],
    "XMM7": ["0x00000000FFFFFFFF", "0x7FC000007FC00000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x515253543f800000
mov [rdx + 8 * 0], rax
mov rax, 0x5152535440000000
mov [rdx + 8 * 1], rax

mov rax, 0x515253543f800000
mov [rdx + 8 * 2], rax
mov rax, 0x5152535440800000
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 0]
movapd xmm4, [rdx + 8 * 0]
movapd xmm5, [rdx + 8 * 0]
movapd xmm6, [rdx + 8 * 0]
movapd xmm7, [rdx + 8 * 0]
movapd xmm8, [rdx + 8 * 2]

cmpss xmm0, xmm8, 0x00 ; EQ
cmpss xmm1, xmm8, 0x01 ; LT
cmpss xmm2, xmm8, 0x02 ; LTE
cmpss xmm4, xmm8, 0x04 ; NEQ
cmpss xmm5, xmm8, 0x05 ; NLT
cmpss xmm6, xmm8, 0x06 ; NLTE

; Unordered and Ordered tests need to be special cased
mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x7FC000007FC00000
mov [rdx + 8 * 1], rax

mov rax, 0x7FC0000000000000
mov [rdx + 8 * 2], rax
mov rax, 0x7FC0000000000000
mov [rdx + 8 * 3], rax

movapd xmm3, [rdx + 8 * 0]
movapd xmm7, [rdx + 8 * 0]
movapd xmm8, [rdx + 8 * 2]

; Unordered will return true when either input is nan
; [0.0, 0.0, nan, nan] unord [0.0, nan, 0.0, nan] = [0, 1, 1, 1]
cmpss xmm3, xmm8, 0x03 ; Unordered

; Ordered will return true when both inputs are NOT nan
; [0.0, 0.0, nan, nan] ord [0.0, nan, 0.0, nan] = [1, 0, 0, 0]
cmpss xmm7, xmm8, 0x07 ; Ordered

hlt
