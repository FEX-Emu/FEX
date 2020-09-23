%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x0000000100000001", "0x0000000200000002"],
    "XMM1":  ["0x0000000400000004", "0x0000000800000008"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3fc000003f800000 ; [1.5, 1.0]
mov [rdx + 8 * 0], rax
mov rax, 0x4039999a40000000 ; [2.9, 2.0]
mov [rdx + 8 * 1], rax

mov rax, 0x4083333340800000 ; [4.1, 4.0]
mov [rdx + 8 * 2], rax
mov rax, 0x4108000041000000 ; [8.5, 8.0]
mov [rdx + 8 * 3], rax

mov rax, 0x4142434445464748
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

; Set up MXCSR to truncate
mov eax, 0x7F80
mov [rdx + 8 * 6], eax
ldmxcsr [rdx + 8 * 6]

movapd xmm0, [rdx + 8 * 4]
movapd xmm1, [rdx + 8 * 4]

movapd xmm2, [rdx + 8 * 0]

cvtps2dq xmm0, xmm2
cvtps2dq xmm1, [rdx + 8 * 2]

hlt
