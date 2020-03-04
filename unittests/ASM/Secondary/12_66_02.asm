%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0000000000000000", "0x0000000000000000"],
    "XMM1": ["0x0000000000000000", "0x0000000000000000"],
    "XMM2": ["0x0041004300450047", "0x0051005300550057"],
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

psrlw xmm0, 32
psrlw xmm1, 16
psrlw xmm2, 8
psrlw xmm3, 1

hlt
