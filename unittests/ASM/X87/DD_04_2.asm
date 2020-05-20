%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xc90fdaa22168c235", "0x4000"],
    "XMM1": ["0x8000000000000000", "0x4005"],
    "XMM2": ["0x8000000000000000", "0x4004"],
    "XMM3": ["0x8000000000000000", "0x4003"],
    "XMM4": ["0x8000000000000000", "0x4002"],
    "XMM5": ["0x8000000000000000", "0x4001"],
    "XMM6": ["0x8000000000000000", "0x4000"],
    "XMM7": ["0x0000000000000000", "0x0000"],
    "MM0":  ["0xc90fdaa22168c235", "0x4000"],
    "MM1":  ["0x8000000000000000", "0x4005"],
    "MM2":  ["0x8000000000000000", "0x4004"],
    "MM3":  ["0x8000000000000000", "0x4003"],
    "MM4":  ["0x8000000000000000", "0x4002"],
    "MM5":  ["0x8000000000000000", "0x4001"],
    "MM6":  ["0x8000000000000000", "0x4000"],
    "MM7":  ["0x0000000000000000", "0x0000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 2
mov [rdx + 2 * 1], rax
mov rax, 4
mov [rdx + 2 * 2], rax
mov rax, 8
mov [rdx + 2 * 3], rax
mov rax, 16
mov [rdx + 2 * 4], rax
mov rax, 32
mov [rdx + 2 * 5], rax
mov rax, 64
mov [rdx + 2 * 6], rax

fldz
fild word [rdx + 2 * 1]
fild word [rdx + 2 * 2]
fild word [rdx + 2 * 3]
fild word [rdx + 2 * 4]
fild word [rdx + 2 * 5]
fild word [rdx + 2 * 6]
fldpi

o16 fnsave [rdx]

fldpi
fldpi
fldpi
fldpi
fldpi
fldpi
fldpi
fldpi

o16 frstor [rdx]

movups xmm0, [rdx + (0xE + 10 * 0)]
movups xmm1, [rdx + (0xE + 10 * 1)]
movups xmm2, [rdx + (0xE + 10 * 2)]
movups xmm3, [rdx + (0xE + 10 * 3)]
movups xmm4, [rdx + (0xE + 10 * 4)]
movups xmm5, [rdx + (0xE + 10 * 5)]
movups xmm6, [rdx + (0xE + 10 * 6)]
movups xmm7, [rdx + (0xE + 10 * 7)]

pslldq xmm0, 6
psrldq xmm0, 6

pslldq xmm1, 6
psrldq xmm1, 6

pslldq xmm2, 6
psrldq xmm2, 6

pslldq xmm3, 6
psrldq xmm3, 6

pslldq xmm4, 6
psrldq xmm4, 6

pslldq xmm5, 6
psrldq xmm5, 6

pslldq xmm6, 6
psrldq xmm6, 6

pslldq xmm7, 6
psrldq xmm7, 6

hlt
