%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x3f8000003f800000", "0x3f8000003f800000"],
    "XMM1":  ["0x3e8000003e800000", "0x3e8000003e800000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3f8000003f800000 ; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x3f8000003f800000
mov [rdx + 8 * 1], rax

mov rax, 0x4080000040800000 ; 4.0
mov [rdx + 8 * 2], rax
mov rax, 0x4080000040800000
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx + 8 * 0]

rcpps xmm0, xmm0
rcpps xmm1, [rdx + 8 * 2]

hlt
