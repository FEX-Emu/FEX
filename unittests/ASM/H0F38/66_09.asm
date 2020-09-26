%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0", "0x0"],
    "XMM1": ["0xFDFEFDFEFDFEFDFE", "0xFCFDFCFDFCFDFCFD"],
    "XMM2": ["0x0202020202020202", "0x0303030303030303"],
    "XMM3": ["0xFDFEFDFE00000000", "0x03030303FCFD0000"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov [rdx + 8 * 1], rax

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax

mov rax, 0x0001000100010001
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

mov rax, 0xFFFFFFFF00000000
mov [rdx + 8 * 6], rax
mov rax, 0x00010001FFFF0000
mov [rdx + 8 * 7], rax

mov rax, 0x0202020202020202
mov [rdx + 8 * 8], rax
mov rax, 0x0303030303030303
mov [rdx + 8 * 9], rax

movaps xmm0, [rdx + 8 * 8]
movaps xmm1, [rdx + 8 * 8]
movaps xmm2, [rdx + 8 * 8]
movaps xmm3, [rdx + 8 * 8]

; Test with full zero
psignw xmm0, [rdx + 8 * 0]

; Test with full negative
psignw xmm1, [rdx + 8 * 2]

; Test with full positive
psignw xmm2, [rdx + 8 * 4]

; Test a mix
psignw xmm3, [rdx + 8 * 6]

hlt
