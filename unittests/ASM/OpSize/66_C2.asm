%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xFFFFFFFFFFFFFFFF", "0x0"],
    "XMM1": ["0x0", "0xFFFFFFFFFFFFFFFF"],
    "XMM2": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM3": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM4": ["0x0", "0xFFFFFFFFFFFFFFFF"],
    "XMM5": ["0xFFFFFFFFFFFFFFFF", "0x0"],
    "XMM6": ["0x0", "0x0"],
    "XMM7": ["0x0000000000000000", "0x0000000000000000"],
    "XMM8": ["0x0000000000000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM9": ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000
mov [rdx + 8 * 1], rax

mov rax, 0x3ff0000000000000
mov [rdx + 8 * 2], rax
mov rax, 0x4008000000000000
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 0]
movapd xmm3, [rdx + 8 * 0]
movapd xmm4, [rdx + 8 * 0]
movapd xmm5, [rdx + 8 * 0]
movapd xmm6, [rdx + 8 * 0]
movapd xmm7, [rdx + 8 * 0]
movapd xmm8, [rdx + 8 * 2]

cmppd xmm0, xmm8, 0x00 ; EQ
cmppd xmm1, xmm8, 0x01 ; LT
cmppd xmm2, xmm8, 0x02 ; LTE
cmppd xmm4, xmm8, 0x04 ; NEQ
cmppd xmm5, xmm8, 0x05 ; NLT
cmppd xmm6, xmm8, 0x06 ; NLTE

; Unordered and Ordered tests need to be special cased
mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x7FF8000000000000
mov [rdx + 8 * 1], rax

mov rax, 0x7FF8000000000000
mov [rdx + 8 * 2], rax
mov rax, 0x0000000000000000
mov [rdx + 8 * 3], rax

movapd xmm3, [rdx + 8 * 0]
movapd xmm7, [rdx + 8 * 0]
movapd xmm8, [rdx + 8 * 2]

; Unordered will return true when either input is nan
; [0.0, nan] unord [nan, 0.0] = [1, 1]
cmppd xmm3, xmm8, 0x03 ; Unordered

; Ordered will return true when both inputs are NOT nan
; [0.0, nan] ord [nan, 0.0] = [0, 0]
cmppd xmm7, xmm8, 0x07 ; Ordered

mov rax, 0x7FF8000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x0000000000000000
mov [rdx + 8 * 1], rax

movapd xmm8, [rdx + 8 * 0]
movapd xmm9, [rdx + 8 * 0]

; Ordered will return true when both inputs are NOT nan
; [nan, 0.0] ord [nan, 0.0] = [0, 1]
cmppd xmm8, xmm9, 0x07 ; Ordered

mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x7FF8000000000000
mov [rdx + 8 * 1], rax

movapd xmm9, [rdx + 8 * 0]
movapd xmm10, [rdx + 8 * 0]

; Ordered will return true when both inputs are NOT nan
; [0.0, nan] ord [0.0, nan] = [1, 0]
cmppd xmm9, xmm10, 0x07 ; Ordered

hlt
