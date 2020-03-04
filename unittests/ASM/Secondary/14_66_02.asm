%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0000000041424344", "0x0000000051525354"],
    "XMM1": ["0x0000616263646566", "0x0000717273747576"],
    "XMM2": ["0x0041424344454647", "0x0051525354555657"],
    "XMM3": ["0x30B131B232B333B4", "0x38B939BA3ABB3BBC"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
movapd xmm1, [rdx + 16]
movapd xmm2, [rdx]
movapd xmm3, [rdx + 16]

psrlq xmm0, 32
psrlq xmm1, 16
psrlq xmm2, 8
psrlq xmm3, 1

hlt
