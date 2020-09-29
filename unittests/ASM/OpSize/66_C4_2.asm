%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x4142434445467778", "0x5152535455565758"],
    "XMM1":  ["0x4142434477784748", "0x5152535455565758"],
    "XMM2":  ["0x4142777845464748", "0x5152535455565758"],
    "XMM3":  ["0x7778434445464748", "0x5152535455565758"],
    "XMM4":  ["0x4142434445464748", "0x5152535455567778"],
    "XMM5":  ["0x4142434445464748", "0x5152535477785758"],
    "XMM6":  ["0x4142434445464748", "0x5152777855565758"],
    "XMM7":  ["0x4142434445464748", "0x7778535455565758"],
    "XMM8":  ["0x4142434445467778", "0x5152535455565758"],
    "XMM9":  ["0x4142434477784748", "0x5152535455565758"],
    "XMM10": ["0x4142777845464748", "0x5152535455565758"],
    "XMM11": ["0x7778434445464748", "0x5152535455565758"],
    "XMM12": ["0x4142434445464748", "0x5152535455567778"],
    "XMM13": ["0x4142434445464748", "0x5152535477785758"],
    "XMM14": ["0x4142434445464748", "0x5152777855565758"],
    "XMM15": ["0x4142434445464748", "0x7778535455565758"]
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

mov rax, 0x7172737475767778
mov [rdx + 8 * 2], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 0]
movapd xmm3, [rdx + 8 * 0]
movapd xmm4, [rdx + 8 * 0]
movapd xmm5, [rdx + 8 * 0]
movapd xmm6, [rdx + 8 * 0]
movapd xmm7, [rdx + 8 * 0]

movapd xmm8, [rdx + 8 * 0]
movapd xmm9, [rdx + 8 * 0]
movapd xmm10, [rdx + 8 * 0]
movapd xmm11, [rdx + 8 * 0]
movapd xmm12, [rdx + 8 * 0]
movapd xmm13, [rdx + 8 * 0]
movapd xmm14, [rdx + 8 * 0]
movapd xmm15, [rdx + 8 * 0]

pinsrw xmm0, rax, 0
pinsrw xmm1, rax, 1
pinsrw xmm2, rax, 2
pinsrw xmm3, rax, 3
pinsrw xmm4, rax, 4
pinsrw xmm5, rax, 5
pinsrw xmm6, rax, 6
pinsrw xmm7, rax, 7

pinsrw xmm8, rax, 8
pinsrw xmm9, rax, 9
pinsrw xmm10, rax, 10
pinsrw xmm11, rax, 11
pinsrw xmm12, rax, 12
pinsrw xmm13, rax, 13
pinsrw xmm14, rax, 14
pinsrw xmm15, rax, 15

hlt
